#!/usr/bin/perl
#
# mapconv.pl - Convert the mapping between vectors to C data structures
#
# Written 2006 by Werner Almesberger
# Copyright 2006 Werner Almesberger
#

if ($ARGV[0] eq "-m") {
    $data = 0;
}
elsif ($ARGV[0] eq "-d") {
    $data = 1;
}
else {
    print STDERR "usage: $0 (-d|-m) [file]\n";
    exit(1);
}

shift(@ARGV);

$n = 0;
while (<>) {
    s/\s*#.*//;
    next if /^\s*$/;
    die unless /^("[^"]+")\s+(\S+)\s+(\S+)\s+(\S+)\s*$/;
    if ($data) {
	print "static const uint32_t v_${n}[] = { $3 END_VECTOR };\n";
    }
    else {
	print "{ $1, ".($2 eq "*" ? "NULL" : "\"$2\"").", v_$n, $4 },\n";
    }
    $n++;
}
