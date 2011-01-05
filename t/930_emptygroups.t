#!/usr/bin/perl
#
# Check that empty host/service groups pass verfication.
# Likely error on non-patched version:
# "Error: Host 'r' specified in host group 'generic-pc' is not defined anywhere!"

use warnings;
use strict;
use Test::More;
use FindBin qw($Bin);

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $icinga = "$topdir/base/icinga";
my $etc = "$Bin/etc";

plan tests => 1;

my @output = `$icinga -v "$etc/icinga-empty-groups.cfg"`;
if ($? == 0) {
	pass("Icinga validated empty host/service-group successfully");
} else {
	@output = grep(/^Error: .+$/g, @output);
	fail("Icinga validation failed:\n@output");
}


