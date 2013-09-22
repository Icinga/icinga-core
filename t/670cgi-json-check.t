#!/usr/bin/perl

# Checks for status.cgi

use warnings;
use strict;
use Test::More;
use Icinga::Test qw ( run_cgi run_cgi_json );

plan tests => 37;

my $config = "etc/cgi-json-check.cfg";
my $test_passed = "OK: Test passed.";
my $output = "";
my $cgi_file = "";

my $numhosts;
my $expected_hosts = 2;


$cgi_file = "config.cgi";

print("Testing ". $cgi_file."\n");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=all', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get all config items");

$output = run_cgi_json('', 'icingaadmin', 'GET', 'type=all', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." without config file");

$output = run_cgi_json($config, 'icinga', 'GET', 'type=all', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." using unknown user");


$cgi_file = "extinfo.cgi";

print("Testing ". $cgi_file."\n");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=3', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get all comments");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=6', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get all downtimes");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=0', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get process info");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=4', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get performance info");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=7', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get scheduling queue");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=1&host=smtp.web.de', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get host info for \"smtp.web.de\"");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=2&host=smtp.gmail.com&service=NSClient%2B%2B', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get service info for \"NSClient++\" on \"smtp.we.de\"");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'type=2&host=smtp.gmail.com&service=SMTP', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get service info for \"SMTP\" on \"smtp.we.de\"");

$output = run_cgi_json('', 'icingaadmin', 'GET', 'type=7', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." without config file");

$output = run_cgi_json($config, 'icinga', 'GET', 'type=7', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." using unknown user");


$cgi_file = "notifications.cgi";

print("Testing ". $cgi_file."\n");

$output = run_cgi_json($cgi_file, 'icingaadmin', 'GET', 'host=all&ts_start=1379541600&ts_end=1379714399', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get notifications for all hosts");

$output = run_cgi_json('', 'icingaadmin', 'GET', 'host=all&ts_start=1379541600&ts_end=1379714399', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." without config file");

$output = run_cgi_json($config, 'icinga', 'GET', 'host=all&ts_start=1379541600&ts_end=1379714399', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." using unknown user");


$cgi_file = "outages.cgi";

print("Testing ". $cgi_file."\n");

$output = run_cgi_json($config, 'icingaadmin', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get current outages");

$output = run_cgi_json('', 'icingaadmin', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." without config file");

$output = run_cgi_json($config, 'icinga', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." using unknown user");


$cgi_file = "showlog.cgi";

print("Testing ". $cgi_file."\n");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'ts_start=1379541600&ts_end=1379688429', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get all log entries");

$output = run_cgi_json('', 'icingaadmin', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." without config file");

$output = run_cgi_json($config, 'icinga', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." using unknown user");


$cgi_file = "status.cgi";

print("Testing ". $cgi_file."\n");

$output = run_cgi_json($config, 'icingaadmin', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get only services");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'style=hostdetail', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get only hosts");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'style=hostservicedetail', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get all hosts and services together");


$output = run_cgi_json($config, 'icingaadmin', 'GET', 'hostgroup=all&style=overview', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." all hostgroups overview");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'hostgroup=all&style=summary', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." all hostgroups summary");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'hostgroup=all&style=grid', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." all hostgroups grid");


$output = run_cgi_json($config, 'icingaadmin', 'GET', 'servicegroup=all&style=overview', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." all servicegroup overview");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'servicegroup=all&style=summary', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." all servicegroup summary");

$output = run_cgi_json($config, 'icingaadmin', 'GET', 'servicegroup=all&style=grid', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." all servicegroup grid");

$output = run_cgi_json('', 'icingaadmin', 'GET', 'servicegroup=all&style=grid', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." missing config error");

$output = run_cgi_json('', 'icingaadmin', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." without config file");

$output = run_cgi_json($config, 'icinga', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." using unknown user");


$cgi_file = "tac.cgi";

print("Testing ". $cgi_file."\n");

$output = run_cgi_json($config, 'icingaadmin', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." get only services");

$output = run_cgi_json('', 'icingaadmin', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." without config file");

$output = run_cgi_json($config, 'icinga', 'GET', '', $cgi_file);
like( $output, "/".$test_passed."/", $cgi_file." using unknown user");


