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

$addr[0xf8] = "KEY1";
$addr[0xf9] = "KEY2";
$addr[0xfa] = "BLOCKID";
$addr[0xfb] = "POINTER";
$addr[0xfc] = "CLOCK";
$addr[0xfe] = "DELAY";

$op[0] = "SSC";
$op[0x30] = "HALT";
$op[0x40] = "NOP";
$op[0x51] = "MOV A,[expr]";
$op[0x60] = "MOV reg[expr],A";

$ssc[0] = "SWBootReset";
$ssc[1] = "ReadBlock";
$ssc[2] = "WriteBlock";
$ssc[3] = "EraseBlock";
$ssc[4] = "ProtectBlock";
$ssc[5] = "EraseAll";
$ssc[6] = "TableRead";
$ssc[7] = "Checksum";
$ssc[8] = "Calibrate0";
$ssc[9] = "Calibrate1";


sub decode
{
    return "<$op[$_[0]]>" if defined $op[$_[0]];
    return sprintf("0x%02x",$_[0]);
}


sub ssc
{
    return $ssc[$_[0]] if defined $ssc[$_[0]];
    return sprintf("0x%02x",$_[0]);
}


$v = join("",<>);
$v =~ s/\s*//g;
while (length $v) {
    die "remainder of vector is ".length($v)." bytes" unless length($v) >= 22;
    $f = substr($v,0,22,"");
    if ($f =~ /^100(........)(........)111$/) {
	print sprintf("1 00 $1 $2 111\tPOKE\t");
	($a,$d) = ($1,$2);
	if ($a =~ /^[01]+$/) {
	    $a = oct("0b$a");
	    if (defined $addr[$a]) {
		print $addr[$a];
	    }
	    else {
		print sprintf("0x%02x",$a);
	    }
	}
	else {
	    print "0b".$a;
	}
	print ",";
	if ($d =~ /^[01]+$/) {
	    print sprintf("0x%02x",oct("0b$d"));
	}
	else {
	    print "0b".$d;
	}
	print "\n";
    }
    elsif ($f =~ /^101(........)Z(........)Z1$/) {
	print sprintf("1 01 $1 Z $2 Z1\tPEEK\t");
	($a,$d) = ($1,$2);
	if ($a =~ /^[01]+$/) {
	    print sprintf("0x%02x",oct("0b$a"));
	}
	else {
	    print "0b".$a;
	}
	print "\t -> $d\n";
    }
    elsif ($f =~ /^110(........)(........)111$/) {
	print sprintf("1 10 $1 $2 111\t");
	($a,$d) = ($1,$2);
	die "invalid special code \"$a\"" unless $a =~ /^[01]+$/;
	$a = oct("0b$a");
	if ($d =~ /^[01]+$/) {
	    $d = oct("0b$d");
	}
	if ($a == 0xf0) {
	    print "LOAD\tA,".&ssc($d)."\n";
	}
	# 0xf4
	# 0xf5
	elsif ($a == 0xf6) {
	    print sprintf("LOAD\tSP,0x%02x\t; ?\n",$d);
	}
	elsif ($a == 0xf7) {
	    print sprintf("LOAD\tF,0x%02x\n",$d);
	}
	elsif ($a == 0xf8) {
	    print "LOAD\tCODE[0],".&decode($d)."\n";
	}
	elsif ($a == 0xf9) {
	    print "LOAD\tCODE[1],".&decode($d)."\n";
	}
	elsif ($a == 0xfa) {
	    print "LOAD\tCODE[2],".&decode($d)."\n";
	}
	elsif ($a == 0xff) {
	    print sprintf("MAGIC\t0x%02x\n",$d);
	}
	else {
	    if ($d =~ /^\d+$/) {
		print sprintf("0x%02x\t0x%02x\n",$a,$d);
	    }
	    else {
		print sprintf("0x%02x\t0b$d\n",$a);
	    }
	}
    }
    else {
	print "$f\n";
    }
}
