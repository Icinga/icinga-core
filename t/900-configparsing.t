#!/usr/bin/perl
#
# Taking a known icinga configuration directory, will check that the objects.cache is as expected

use warnings;
use strict;
use Test::Most;
use FindBin qw($Bin);
use Icinga::Test qw ( run_cmd slurp_file );

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $icinga = "$topdir/base/icinga";
my $etc = "$Bin/etc";
my $precache = "$Bin/var/objects.precache";

plan tests => 3;

my $output = run_cmd([$icinga, '-v', "$etc/icinga.cfg"]);
if ($? == 0) {
	pass("Icinga validated test configuration successfully");
} else {
	fail("Icinga validation failed:\n$output");
}

unlink $precache if -e $precache ;
$output = run_cmd ([ $icinga, '-vp', "$etc/icinga.cfg" ]);
if ($? == 0) {
	pass("Icinga precache generated successfully");
} else {
	fail("Could not create Icinga precache:\n$output");
}

system("grep -v 'Created:' $precache > '$precache.generated'");

my $generated = slurp_file("$precache.generated");
my $expected = slurp_file("$precache.expected");
eq_or_diff($generated, $expected,'Icinga precached objects file matches expected');

#cleanup
unlink("$precache.generated") if -e "$precache.generated";
unlink $precache if -e $precache;
