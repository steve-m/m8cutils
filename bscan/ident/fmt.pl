#!/usr/bin/perl
#
# fmt.pl - Formatter for automated probes
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

$W = 4;		# column width

while (<>) {
    chop;
    if (/^-D(SINGLE|DUAL)=(\S+)/) {
	$t = $2;
	if (!defined $n{$t}) {
	    $n{$t} = $n++;
	    push(@t,$t);
	}
	$test{$curr} |= 1 << $n{$t};
	if (/YES/) {
	    $res{$curr} |= 1 << $n{$t};
	}
    }
    else {
	$curr = $_;
        push(@c,$_);
    }
}

# analyze widths

for (@t) {
    $l = length $_ if length $_ > $l;
}
$d = int(($l+$W)/$W);
$l = 0;
for (@c) {
    $l = length $_ if length $_ > $l;
}

# print header

$x = $l+$W;
for ($i = 0; $i != $d; $i++) {
    print " " x $x;
    print (("|".(" " x ($W-1))) x $i);
    for ($j = $i; $j < @t; $j += $d) {
	print $t[$j];
	if (@t >= $j+$d) {
	    $x0 = $x+$j*$W+length $t[$j];
	    $x1 = $x+($j+$d)*$W;
	    for ($k = $x0; $k != $x1; $k++) {
		last if $k > $x+$W*(@t-1);
		if (($k-$x) % $W || $k < $x+$d*$W) {
		    print " ";
		}
		else {
		    print "|";
		}
	    }
	}
    }
    print "\n";
}

# continue header for one more line

print " " x ($x-($W-1)),((" " x ($W-1))."|") x @t,"\n";

# print the results

for (@c) {
    print sprintf("%-*s",$l+1,$_);
    for ($i = 0; $i != $n; $i++) {
	print " " x ($W-1);
	print "",
	  (($test{$_} >> $i) & 1 ? ($res{$_} >> $i) & 1 ? "Y" : "." : "-");
    }
    print "\n";
}
