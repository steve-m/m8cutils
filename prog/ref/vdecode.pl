#!/usr/bin/perl
#
# vdecode.pl - Programming vector decoder
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

#
# This script is an ad hoc hack to decode programming vectors. It's neither
# meant to be pretty nor particularly accurate.
#

$addr{0xf8} = "KEY1";
$addr{0xf9} = "KEY2";
$addr{0xfa} = "BLOCKID";
$addr{0xfb} = "POINTER";
$addr{0xfc} = "CLOCK";
$addr{0xfd} = "RESERVED_FD";
$addr{0xfe} = "DELAY";
$addr{0xff} = "RESERVED_FF";

$data{"0xF8:0x3A"} = "KEY1_MAGIC";

$reg{0xe0} = "OSC_CR0";
$reg{0xf0} = "CPU_A";
$reg{0xf3} = "CPU_X";
$reg{0xf4} = "CPU_PCL";
$reg{0xf5} = "CPU_PCH";
$reg{0xf6} = "CPU_SP";
$reg{0xf7} = "CPU_F";
$reg{0xf8} = "CPU_CODE0";
$reg{0xf9} = "CPU_CODE1";
$reg{0xfa} = "CPU_CODE2";
$reg{0xfe} = "CPU_SCR1";
$reg{0xff} = "CPU_SCR0";

$op{0} = "<SSC>";
$op{0x30} = "<HALT>";
$op{0x40} = "<NOP>";
$op{0x51} = "<MOV A,[expr]>";
$op{0x60} = "<MOV reg[expr],A>";
$op{0xe8} = "IMO_TR";
$op{0xea} = "BDG_TR";

$f{0x10} = "CPU_F_XIO";

$osc_cr0{2} = "Speed_12MHz";

$ssc{0} = "SSC_SWBootReset";
$ssc{1} = "SSC_ReadBlock";
$ssc{2} = "SSC_WriteBlock";
$ssc{3} = "SSC_EraseBlock";
$ssc{4} = "SSC_ProtectBlock";
$ssc{5} = "SSC_EraseAll";
$ssc{6} = "SSC_TableRead";
$ssc{7} = "SSC_Checksum";
$ssc{8} = "SSC_Calibrate0";
$ssc{9} = "SSC_Calibrate1";
$ssc{16} = "SSC_ReadProtection";


sub addr
{
    local ($addr) = @_;

    return "0b$addr" unless $addr =~ /^[01]+$/;
    $addr = oct("0b".$addr);
    return sprintf("0x%02x",$addr) unless defined $addr{$addr};
    push(@explain,"$addr{$addr} = ".sprintf("0x%02x",$addr));
    return $addr{$addr};
}


sub data
{
    local ($addr,$data) = @_;

    return "0b$data" unless $data =~ /^[01]+$/;
    $addr = sprintf("0x%02X",oct("0b".$addr));
    $data = sprintf("0x%02X",oct("0b".$data));
    return $data unless defined $data{"$addr:$data"};
    push(@explain,$data{"$addr:$data"}." = $data");
    return $data{"$addr:$data"};
}


sub reg
{
    local ($addr) = @_;

    return "0b$addr" unless $addr =~ /^[01]+$/;
    $addr = oct("0b".$addr);
    return sprintf("0x%02X",$addr) unless defined $reg{$addr};
    push(@explain,"$reg{$addr} = ".sprintf("0x%02x",$addr));
    return $reg{$addr};
}


sub value
{
    local ($addr,$data) = @_;
    local ($txt) = undef;

    return "0b$data" unless $data =~ /^[01]+$/;
    $addr = oct("0b".$addr);
    $data = oct("0b".$data);
    $txt = $ssc{$data} if $addr == 0xf0;			# CPU_A
    $txt = $f{$data} if $addr == 0xf7;				# CPU_F
    $txt = $op{$data} if $addr >= 0xf8 && $addr <= 0xfa;	# CPU_CODEx
    $txt = "MAGIC_EXEC" if $addr == 0xff;			# CPU_SCR0
    $txt = $osc_cr0{$data} if $addr == 0xe0;			# OSC_CR0 
    return sprintf("0x%02X",$data) unless defined $txt;
    push(@explain,"$txt = $data");
    return $txt;
}


sub tab
{
    local ($s,$tabs) = @_;

    return "$s " if length $s >= $tabs*8;
    return $s.("\t" x int(($tabs*8-length($s)+7)/8));
}


sub explain
{
    if (!$explain || !@explain) {
	print "\n";
	return;
    }
    print "// ",join(", ",@explain),"\n";
    @explain = ();
}


while (@ARGV) {
    if ($ARGV[0] =~ /-s/) {
	$short = 1;
    }
    elsif ($ARGV[0] =~ /-e/) {
	$explain = 1;
    }
    elsif ($ARGV[0] =~ /-/) {
	print STDERR "usage: $0 [-e] [-s] [file ...]\n";
	print STDERR "  -e  explain; print values of symbolic names\n";
	print STDERR "  -s  short; don't print bit strings\n";
	exit(1);
    }
    else {
	last;
    }
    shift(@ARGV);
}

$v = join("",<>);
$v =~ s/\s*//g;
while (length $v) {
    die "remainder of vector is ".length($v)." bytes" unless length($v) >= 22;
    $f = substr($v,0,22,"");
    if ($f =~ /^100(........)(........)(...)$/) {
	print "100 $1 $2 $3\t" unless $short;
	print &tab("[".&addr($1)."]",2),&tab("= ".&data($1,$2),2);
        &explain;
    }
    elsif ($f =~ /^101(........).(........).(.)$/) {
	print "101 $1 Z $2 Z $3\t" unless $short;
	print &tab("[".&addr($1)."]",2),&tab("-> ".&data($1,$2),2);
	&explain;

    }
    elsif ($f =~ /^110(........)(........)(...)$/) {
	print "110 $1 $2 $3\t" unless $short;
	print &tab("reg[".&reg($1)."]",2),&tab("= ".&value($1,$2),2);
        &explain;
    }
    elsif ($f =~ /^111(........).(........).(.)$/) {
	print "111 $1 $2 $3\t" unless $short;
	print &tab("reg[".&reg($1)."]",2),&tab("-> ".&value($1,$2),2);
        &explain;
    }
    else {
	print "$f\n";
    }
}
