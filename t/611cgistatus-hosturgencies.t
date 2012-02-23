#!/usr/bin/perl

# host urgency related checks for status.cgi

use warnings;
use strict;
use Test::More;
use Icinga::Test qw ( run_cgi );

plan tests => 1;


my $output = run_cgi('etc/cgi-hosturgencies.cfg', 'GET', 'hostgroup=all&style=hostdetail', 'status.cgi');
like( $output, '/host1.*host2.*host3.*host4/msx', "List of hosts sorted by ascending name" );

open (my $fh, '>', '/tmp/test.html');
print $fh $output;
