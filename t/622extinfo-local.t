#!/usr/bin/perl
#
# Local Checks for extinfo.cgi

use warnings;
use strict;
use Test::Most;
use Icinga::Test qw ( run_cgi );

plan tests => 2;

my $output = run_cgi('etc/cgi.cfg', 'GET', '', 'extinfo.cgi');
like( $output, "/Process Information/", "extinfo.cgi without params show the process information" );

$output = run_cgi('etc/cgi.cfg', 'GET', 'type=1&host=host1', 'extinfo.cgi');
like( $output, "/Schedule downtime for this host and all services/", "extinfo.cgi allows us to set downtime for a host and all of his services" );
