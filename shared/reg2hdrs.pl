#!/usr/bin/perl
#
# reg2hdrs.pl - Convert "m8c-registers" to various headers used by m8cutils
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#


sub pad
{
    local ($text,$tabs) = @_;

    die "text \"$text\" with $tabs tabs" if length $text >= $tabs*8;
    return $text.("\t" x ($tabs-int(length($text)/8)));
}


sub process
{
    $line =~ s/#.*//;
    return if $line =~ /^\s*$/;
    die "syntax error "
      unless $line =~ /^0x([0-9a-fA-F]+)\s+([A-Za-z_][A-Z-a-z0-9_]*)\s*/;
    $name = $2;
    die "duplicate definition of $name"
      if defined $reg{$name} && $reg{$name} != hex $1;
    $reg{$name} = hex $1;
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
	next if $1 eq "_";
	@fv = ();
	$values = $5;
	if ($mode eq "sim") {
	    push(@fv,"define\t".&pad("  ".$name."_".$1,3).
	      sprintf("%s[%d:%d]\n",$name,$bits+$size-1,$bits));
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
		else {
		    push(@fv,"#define\t".&pad("  ".$name."_".$1."_".$_,4).
		      sprintf("0x%x\n",$n << $bits));
		}
	    }
	    $n++;
	}
	unshift(@f,@fv);
    }
    die "$name: field syntax error" if $line !~ /^\s*$/;
    die "$name: $bits bits left in byte" if $bits;
    $fields{$name} .= join("",@f);
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

if ($mode eq "sim") {
    $name = "DEFAULT_M8CSIM";
    print "// MACHINE-GENERATED. DO NOT EDIT !\n\n" || die "print: $!";
}
elsif ($mode eq "asm") {
    $name = "M8C_INC";
    print "/* MACHINE-GENERATED. DO NOT EDIT ! */\n\n" || die "print: $!";
    print "#ifndef $name\n#define $name\n\n" || die "print: $!";
}
elsif ($mode eq "c") {
    $name = "M8C_H";
    print "/* MACHINE-GENERATED. DO NOT EDIT ! */\n\n" || die "print: $!";
    print "#ifndef $name\n#define $name\n\n" || die "print: $!";
}
else {
    die "unknown mode \"$mode\"";
}
for (sort keys %reg) {
    if ($mode eq "sim") {
	print "define\t".&pad($_,3).sprintf("reg[0x%03x]\n",$reg{$_}) ||
	  die "print: $!";
    }
    else {
	print "#define\t".&pad($_,3).sprintf("0x%03x\n",$reg{$_}) ||
	  die "print: $!";
    }
    print $fields{$_} || die "print: $!" if defined $fields{$_};
}
if ($mode eq "c") {
    print "\n#define\tREGISTER_NAMES_INIT \\\n";
    for (sort { $reg{$a} <=> $reg{$b} } keys %reg) {
	print sprintf("    [0x%03x] = \"$_\", \\\n",$reg{$_}) ||
	  die "print: $!";
    }
}
if ($mode ne "sim") {
    print "\n#endif /* !$name */\n" || die "print: $!";
}
