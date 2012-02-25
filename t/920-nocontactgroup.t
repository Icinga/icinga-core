#!/usr/bin/perl
# 
# Check that no contactgroup gives the correct message

use warnings;
use strict;
use Icinga::Test qw ( run_cmd );
use Test::More;
use FindBin qw($Bin);

my $etc = "$Bin/etc";
my $topdir = "$Bin/..";
my $icinga = "$topdir/base/icinga";

plan tests => 2;

# icinga run without existing contactgroup should raise an exeception

my $output = run_cmd([$icinga, '-v', "$etc/icinga-no-contactgroup.cfg"]);
isnt($?, 0, "And get return code error" );
like( $output, "/Error: Could not find any contactgroup matching 'nonexistantone'/", "Correct error for no contactgroup" );
