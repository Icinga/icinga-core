#!/usr/bin/perl
# 
# Taking a known nagios configuration directory, will check that the objects.cache is as expected

use warnings;
use strict;
use Test::More;
use FindBin qw($Bin);

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $icinga = "$topdir/base/icinga";
my $etc = "$Bin/etc";
my $precache = "$Bin/var/objects.precache";

plan tests => 2;

my $output = `$icinga -v "$etc/icinga.cfg"`;
if ($? == 0) {
	pass("Icinga validated test configuration successfully");
} else {
	fail("Icinga validation failed:\n$output");
}

system("$icinga -vp '$etc/icinga.cfg' > /dev/null") == 0 or die "Cannot create precached objects file";
system("grep -v 'Created:' $precache > '$precache.generated'");

my $diff = "diff -u $precache.expected $precache.generated";
system("$diff > /dev/null");
if ($? == 0) {
	pass( "Icinga precached objects file matches expected" );
} else {
	fail( "Icinga precached objects discrepency!!!\nTest with: $diff\nCopy with: cp $precache.generated $precache.expected" );
}	

