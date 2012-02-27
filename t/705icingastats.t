#!/usr/bin/perl

# Checks icingastats

use warnings;
use strict;
use Test::More;
use Icinga::Test qw ( run_cmd );
use FindBin qw($Bin);

my $topdir = "$Bin/..";
my $icingastats = "$topdir/base/icingastats";
my $etc = "$Bin/etc";

plan tests => 9;

my $output = run_cmd([$icingastats, '-c',  "$etc/icnga-does-not-exit.cfg"]);
isnt( $?, 0, "Bad return code with no config file" );
like( $output, "/Error processing config file/", "No config file" );

$output = run_cmd([$icingastats, '-c', "$etc/icinga-no-status.cfg"]);
isnt( $?, 0, "Bad return code with no status file" );
like( $output, "/Error reading status file 'var/status.dat.no.such.file': No such file or directory/", "No config file" );

$output = run_cmd([$icingastats, '-c', "$etc/icinga-no-status.cfg", '-m', 'NUMHSTUP']);
isnt( $?, 0, "Bad return code with no status file in MRTG mode" );

$output = run_cmd([$icingastats, '-c', "$etc/icinga.cfg"]);
is($?, 0, "No error with working config");

like($output, qr/Icinga PID.*48451\s/, 'Should include icinga pid');

like($output, qr/Total Hosts.*2\s/, 'We have two hosts in this config');
like($output, qr/Total Services.*2\s/, 'We have two services in this config');


