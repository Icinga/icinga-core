#!/usr/bin/perl
# 
# Local Checks for extinfo.cgi

use warnings;
use strict;
use Test::More;
use FindBin qw($Bin);

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $cgi_dir = "$topdir/cgi";
my $extinfo_cgi = "$cgi_dir/extinfo.cgi";

my $number_of_test_run=0;
my $type="";
my $query="";
my $output;
my $remote_user = "REMOTE_USER=icingaadmin";

$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg REQUEST_METHOD=GET $remote_user QUERY_STRING='nodaemoncheck&type=$type' $extinfo_cgi`;
subtest "extinfo.cgi without type" => sub {
	like( $output, "/Process Information/", "extinfo.cgi without params show the process information" );
	done_testing(1);
};
$number_of_test_run++;

$type="1";
$query="";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg REQUEST_METHOD=GET $remote_user QUERY_STRING='nodaemoncheck&type=$type' $extinfo_cgi`;
subtest "extinfo.cgi with $type$query" => sub{
	like( $output, "/It appears as though you do not have permission to view information for this host.../", "Type 1 without host shows an error" );
	done_testing(1);
};
$number_of_test_run++;

$type="1";
$query="&host=host1";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg REQUEST_METHOD=GET $remote_user QUERY_STRING='nodaemoncheck&type=$type$query' $extinfo_cgi`;
subtest "extinfo.cgi with $type$query" => sub{
	like( $output, "/Schedule downtime for this host and all services/", "extinfo.cgi allows us to set downtime for a host and all of his services" );
	done_testing(1);
};
$number_of_test_run++;


done_testing($number_of_test_run);
