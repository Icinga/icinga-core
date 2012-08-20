#!/usr/bin/perl

# Checks for status.cgi

use warnings;
use strict;
use Test::More;
use Icinga::Test qw ( run_cgi );

plan tests => 8;

my $expected_hosts = 2;
my $numhosts;

# pass host=all and checks if its included in the output
my $output = run_cgi('etc/cgi.cfg', 'GET', 'host=all', 'status.cgi');

like( $output, '/status.cgi\?host=all/', "Host value should be set to all if host=all passed in" );

# count the number of hosts included, we don't care about mouseovers
$numhosts = grep /extinfo.cgi\?type=1&host=(?!.*onMouseOver)/, split("\n", $output);

ok( $numhosts == $expected_hosts, "Expected 2 hosts, but we got $numhosts");

# run with set host argument
$output = run_cgi('etc/cgi.cfg', 'GET', 'host=host1', 'status.cgi');

# check if setting the filter works
like( $output, '/status.cgi\?host=host1/', "Host value should be set to specific host if passed in" );
like( $output, '/1 of 1 Matching Services/', "Found the one host" );

# run with empty host argument and see if nothing matches
$output = run_cgi('etc/cgi.cfg', 'GET', 'host=', 'status.cgi');

like( $output, '/status.cgi\?host=&/', "Host value kept as blank if set to blank" );
like( $output, '/0 of 0 Matching Services/', "Got no hosts because looking for a blank name" );

$output = run_cgi('etc/cgi.cfg', 'GET', '', 'status.cgi');
like( $output, '/status.cgi\?host=all&/', "Host value should be set to all if nothing set initially" );

my $hosts_found = grep /extinfo.cgi\?type=1&host=(?!.*onMouseOver)/, split("\n", $output);

# we expect the same number of hosts as with hosts=all
is( $hosts_found, $numhosts, "Same number of hosts as with hosts=all" );
