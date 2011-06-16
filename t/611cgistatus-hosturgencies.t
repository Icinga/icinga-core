#!/usr/bin/perl
# 
# Checks for status.cgi

use warnings;
use strict;
use Test::More;
use FindBin qw($Bin);

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $cgi_dir = "$topdir/cgi";
my $cgi = "$cgi_dir/status.cgi";

my $output;

#plan tests => 3;
plan tests => 2;

my $numhosts;

# Host list sorted by name
$output = `ICINGA_CGI_CONFIG=etc/cgi-hosturgencies.cfg REMOTE_USER=icingaadmin REQUEST_METHOD=GET QUERY_STRING='hostgroup=all&style=hostdetail' $cgi`;
like( $output, '/>host1<.*>host2<.*>host3<.*>host4</s', "List of hosts sorted by ascending name" );

# Host list sorted by (traditional) status
#$output = `ICINGA_CGI_CONFIG=etc/cgi-hosturgencies.cfg REMOTE_USER=icingaadmin REQUEST_METHOD=GET QUERY_STRING='hostgroup=all&style=hostdetail&sorttype=1&sortoption=8' $cgi`;
#like( $output, '/>host4<.*>host1<.*>host2<.*>host3</s', "List of hosts sorted by host status (ascending)" );

# Host list sorted by urgency
$output = `ICINGA_CGI_CONFIG=etc/cgi-hosturgencies.cfg REMOTE_USER=icingaadmin REQUEST_METHOD=GET QUERY_STRING='hostgroup=all&style=hostdetail&sorttype=2&sortoption=9' $cgi`;
like( $output, '/>host2<.*>host3<.*>host4<.*>host1</s', "List of hosts sorted by descending urgency" );
