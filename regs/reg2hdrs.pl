#!/usr/bin/perl
#
# reg2hdrs.pl - Convert "m8c-registers" to various headers used by m8cutils
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

#
# This badly wants rewriting:
#
# - expressions should be evaluated only once and for the whole bit vector.
#   The caches help a little, but that's just a kludge.
#
# - should do all this in C, LEX, and YACC
#
# - if we had a proper scanner, nothing like the process/process_one hack would
#   be needed
#


sub pad
{
    local ($text,$tabs) = @_;

#    die "text \"$text\" with $tabs tabs" if length $text >= $tabs*8;
    return $text." " if length $text >= $tabs*8;
    return $text.("\t" x ($tabs-int(length($text)/8)));
}


sub process_one
{
#print STDERR "process_one: '$line'\n";
    $line =~ s/#.*//;
    return if $line =~ /^\s*$/;

    # chip declaration
    if ($line =~
      /^chip\s+([A-Za-z_][A-Z-a-z0-9_]*)(\s+([A-Za-z_][A-Z-a-z0-9_]*))?\s*$/) {
	die "chip $1 is already defined" if defined $chip{$1};
	$chip{$1} = $chips++;
	push(@chip,$1);
	if (defined $2) {
	    die "alias $3 is already defined" if defined $alias{$3};
	    $alias{$3} = $1;
	}
	return;
    }

    # conditional attribute
    if ($line =~ /^([A-Za-z_][A-Z-a-z0-9_]*)\s*=\s*/) {
	local $id;
	die "attribute \"$1\" is already defined" if defined $attr{$1};
	$id = $1;
	($tmp = $') =~ s/\s+//g;
	$attr{$id} = $tmp;
	return;
    }

    # if
    if ($line =~ /^if\s+/) {
	($tmp = $') =~ s/\s+//g;
	push(@ifs,"(".$tmp.")");
	return;
    }

    # endif
    if ($line =~ /^endif\s*$/) {
	die "\"endif\" without \"if\"" unless @ifs;
	pop(@ifs);
	return;
    }

    # register definition
    die "syntax error "
      unless $line =~
	/^([01xX]),([0-9a-fA-F]{2})[Hh]\s+([A-Za-z_][A-Z-a-z0-9_]*)\s*/;
    $name = $3;
    if ($1 eq "x" || $1 eq "X") {
	if ($mode eq "c") {
	    $addr = 0x200 | hex $2;	# PSOC_REG_ADDR_X
	}
	else {
	    $addr = hex $2;
	}
    }
    else {
	$addr = ($1 << 8) | hex $2;
    }
    $defs{$name}++;
    $index = $name."#".$defs{$name};
    $reg{$index} = $addr;
    $cond{$index} = join("&&",@ifs);
    return unless length $';
    $bits = 8;
    @f = ();
    $line = $';
    while (1) {
	last unless $line =~
	  /^(_|[A-Za-z0-9_]+)(\[(\d+)\])?(\s*{\s*(([A-Za-z0-9_]+\s*)*)})?\s*/;
	$line = $';
	$size = defined $2 ? $3 : 1;
	die "$name: exceeding byte" if $size > $bits;
	$bits -= $size;
	if ($1 eq "_") {
	    if ($mode eq "c") {
		push(@f,"\n\t{ NULL, $size, NULL },");
	    }
	    next;
	}
	@fv = ();
	$values = $5;
	if ($mode eq "sim") {
	    push(@fv,"define\t".&pad("  ".$name."_".$1,3).
	      sprintf("%s[%d:%d]\n",$name,$bits+$size-1,$bits));
	}
	elsif ($mode eq "c") {
	    push(@f,"\n\t{ \"$1\", $size, ");
	    if (defined $values) {
		push(@f,"(const struct psoc_regdef_value []) {");
	    }
	    else {
		push(@f,"NULL");
	    }
	}
	else {
	    push(@fv,"#define\t".&pad("  ".$name."_".$1,3).
	      sprintf("0x%02x\n",((1 << $size)-1) << $bits));
	}
	$n = 0;
	for (split(/\s+/,$values)) {
	    next if $_ eq "";
	    die "${name}_$1 overflow on $_" if $n == 1 << $size;
	    if ($_ ne "_") {
		if ($mode eq "sim") {
		    push(@fv,"define\t".&pad("    ".$name."_".$1."_".$_,4).
		      sprintf("0x%x\n",$n));
		}
		elsif ($mode eq "c") {
		    push(@f,"\n\t    { \"$_\", ".sprintf("0x%x",$n)." },");
		}
		else {
		    push(@fv,"#define\t".&pad("  ".$name."_".$1."_".$_,4).
		      sprintf("0x%x\n",$n << $bits));
		}
	    }
	    $n++;
	}
	if ($mode eq "c") {
	    if (defined $values) {
		push(@f,"\n\t    { NULL } }");
	    }
	    push(@f," },");
	}
	unshift(@f,@fv);
    }
    die "$name: field syntax error" if $line !~ /^\s*$/;
    die "$name: $bits bits left in byte" if $bits;
    $fields{$index} .= join("",@f);
}


sub process
{
    local $left;

    while ($line =~
      /^(..*?)\b(chip|[01xX],[0-9a-fA-F][0-9a-fA-F][hH]|if|endif|[_A-Za-z][_A-Za-z0-9]*\s*=)/) {
	$left = $2.$';
	$line = $1;
	&process_one;
	$line = $left;
    }
    &process_one;
}


sub context
{
    return join("/",@context).": ";
}


sub token
{
    return undef if $expr eq "";
    die &context."syntax error in \"$expr\""
      unless $expr =~ /^(&&|\|\||!|\(|\)|0|1|[A-Za-z_][A-Za-z0-9_]*)/;
    $expr = $';
    return $1;
}


sub item
{
    local ($chip) = @_;
    local ($token,$value,$tmp);

    $token = &token;
    if ($token eq "!") {
	return !&item($chip);
    }
    elsif ($token eq "(") {
	$value = &expr($chip);
	die &context."missing )" unless &token eq ")";
	return $value;
    }
    elsif (defined $attr{$token}) {
	return $cache{"$token $chip"} if defined $cache{"$token $chip"};
	$tmp = $expr;
	$expr = $attr{$token};
	push(@context,$token);
	$value = &expr($chip);
	die &context."syntax error" unless $expr eq "";
	pop(@context);
	$expr = $tmp;
	$value = $value || $token eq $chip;
	$cache{"$token $chip"} = $value;
	return $value;
    }
    elsif ($token eq $chip) {
	return 1;
    }
    elsif (defined $chip{$token}) {
	return 0;
    }
    elsif ($token eq "0") {
	return 0;
    }
    elsif ($token eq "1") {
	return 1;
    }
    else {
	die &context."unknown/invalid token \"$token\"";
    }
}


sub expr
{
    local ($chip) = @_;
    local ($value,$token,$tmp);

    $value = &item($chip);
    while (1) {
	$token = &token;
	if ($token eq "&&") {
	    $tmp = &item($chip);
	    $value = $value && $tmp;
	}
	elsif ($token eq "||") {
	    $tmp = &item($chip);
	    $value = $value || $tmp;
	}
	else {
	    $expr = $token.$expr;
	    last;
	}
    }
    return $value;
}


sub eval_if
{
    local ($res,$i);

    return $cache{$_[0]} if defined $cache{$_[0]};
    $res = "";
    for ($i = 0; $i != @chip; $i++) {
	$expr = $_[0]; # global !
	if ($expr eq "") {
	    $res .= "1";
	}
	else {
	    $res .= &expr($chip[$i]) ? "1" : "0";
	    die &context."syntax error in \"$_[0]\"" unless $expr eq "";
	}
    }
    $cache{$_[0]} = $res;
    return $res;
}


$mode = shift(@ARGV);
while (<>) {
    if (/^\s/) {
	$line .= $_;
	next;
    }
    else {
	&process;
	$line = $_;
    }
}
&process;
die "if without endif: $ifs[$#ifs]" if @ifs;

foreach (sort keys %defs) {
    $present = "0" x $chips;
    for ($i = 1; $i <= $defs{$_}; $i++) {
	$index = $_."#".$i;
	@context = ($index);
	$chips{$index} = &eval_if($cond{$index});
	for ($j = 0; $j != $chips; $j++) {
	    if (substr($chips{$index},$j,1) eq "1") {
		die "multiple definitions of $_ selected" if
		  substr($present,$j,1) eq "1";
		substr($present,$j,1) = "1";
	    }
	}
    }
}

if ($mode eq "sim") {
    $name = "DEFAULT_M8CSIM";
    print "// MACHINE-GENERATED. DO NOT EDIT !\n\n" || die "print: $!";
}
elsif ($mode eq "asm") {
    $name = "M8C_INC";
    print "/* MACHINE-GENERATED. DO NOT EDIT ! */\n\n" || die "print: $!";
    print "#ifndef $name\n#define $name\n\n" || die "print: $!";
}
elsif ($mode eq "h") {
    $name = "M8C_H";
    print "/* MACHINE-GENERATED. DO NOT EDIT ! */\n\n" || die "print: $!";
    print "#ifndef $name\n#define $name\n\n" || die "print: $!";
}
elsif ($mode eq "c") {
    $name = undef;
    print "/* MACHINE-GENERATED. DO NOT EDIT ! */\n\n" || die "print: $!";
    print "#include <stddef.h>\n\n#include \"regdef.h\"\n\n\n" ||
      die "print: $!";
    print "const struct psoc_regdef_register psoc_regdef[] = {" ||
      die "print: $!";
}
else {
    die "unknown mode \"$mode\"";
}

if ($mode ne "c") {
    foreach (sort keys %alias) {
	print "#ifdef $_\n#define $alias{$_}\n#endif\n";
    }
    print "\n" if keys %alias;
}

$all = "1" x $chips;
$curr_if = $all;
for (sort keys %reg) {
    ($name = $_) =~ s/#.*//;
    if ($mode ne "c" && $curr_if != $chips{$_}) {
	print "#endif\n" unless $curr_if == $all;
	$curr_if = $chips{$_};
	if ($curr_if != $all) {
	    print "#if";
	    $first = 1;
	    for ($i = 0; $i != $chips; $i++) {
		next unless substr($curr_if,$i,1) eq "1";
		print " ||" if !$first;
		print " defined(".$chip[$i].")";
		$first = 0;
	    }
	    print " 0" if $first;
	    print "\n";
	}
    }
    if ($mode eq "sim") {
	print "define\t".&pad($name,3).sprintf("reg[0x%03x]\n",$reg{$_}) ||
	  die "print: $!";
    }
    elsif ($mode eq "c") {
	print "\n    { \"$name\", ".sprintf("0x%03x",$reg{$_}).", \"".
          $chips{$_}."\"," ||
	  die "print: $!";
	if (defined $fields{$_}) {
	    print "\n      (const struct psoc_regdef_field []) { $fields{$_} }"
	      || die "print: $!";
	}
	else {
	    print " NULL" || die "print: $!";
	}
	print " }," || die "print: $!";
    }
    else {
	print "#define\t".&pad($name,3).sprintf("0x%03x\n",$reg{$_}) ||
	  die "print: $!";
    }
    print $fields{$_} || die "print: $!" if
      defined $fields{$_} && $mode ne "c";
}

if ($mode eq "h") {
    print "#endif\n\n" if $curr_if != $all;
    print "\n#define\tREGISTER_NAMES_INIT \\\n";
    for (sort { $reg{"$a#1"} <=> $reg{"$b#1"} } keys %defs) {
	next if $seen{$reg{"$_#1"}};
	$seen{$reg{"$_#1"}} = 1;
	print sprintf("    [0x%03x] = \"$_\", \\\n",$reg{"$_#1"}) ||
	  die "print: $!";
    }
}
if ($mode eq "c") {
    print "\n    { NULL }\n};\n";
}
else {
    print "#endif\n\n" if $curr_if != $all;
    if ($mode ne "sim") {
	print "\n#endif /* !$name */\n" || die "print: $!";
    }
}

if ($mode eq "c") {
    print "\nconst struct psoc_regdef_chip proc_regdef_chips[] = {\n";
    foreach (sort(@chip,keys %alias)) {
	print "    { \"$_\",\t";
	if (defined $chip{$_}) {
	    print $chip{$_};
	}
	else {
	    print $chip{$alias{$_}};
	}
	print " },\n";
    }
    print "    { NULL,\t0 }\n};\n";
}
