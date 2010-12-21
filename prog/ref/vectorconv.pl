#!/usr/bin/perl
#
# vectorconv.pl - Converter for programming vectors
#
# Written 2005, 2006 by Werner Almesberger
# Copyright 2005, 2006 Werner Almesberger
#
# The vectors are published in a PDF document (AN2026A and AN2026B), in a
# format that requires heavy post-processing after copy-and-paste. This script
# does this conversion.
#


sub out
{
    local $version = "";
    local $i = 0;

    return unless defined $name;
    if ($name =~ /\s*\((.*)\)1?$/) {
	$name = $`;
	$version = $1;
    }
    print "    { \"$name\", ";
    if ($version) {
	print "\"$version\",";
    }
    else {
	print "NULL,";
    }
    while ($s = substr($vector,0,8,"")) {
	print "\n      " unless $i++ % 6;
	print "\"$s\" ";
    }
    print "},\n";
}


while (<>) {
    chop;
    s/#.*//;
    s/^\s*//;
    s/where.*//;
    next if /^$/;
    if (/^([-A-Z()].*?)(\s+([01][01]\S*))?\s*$/) {
	($n,$b) = ($1,$3);
	if ($paren || $n =~ /^[-(]/ || $name =~ /-$/ ) {
	    $name .= $n;
	    $vector .= $b;
	    $paren = 1 if $n =~ /\(/;
	    undef $paren if $n =~ /\)/;
	}
	else {
	    &out;
	    $name = $n;
	    $vector = $b;
	}
    }
    elsif (/^[01][01]\S*/) {
	$vector .= $&;
    }
    else {
	die;
    }
}
&out;
