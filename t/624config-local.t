#!/usr/bin/perl
# 
# Local Checks for cmd.cgi

use warnings;
use strict;
use Test::More;
use FindBin qw($Bin);

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $cgi_dir = "$topdir/cgi";
my $local_cgi = "$cgi_dir/config.cgi";

my $output;

my $type = '';
my $remote_user = 'REMOTE_USER=icingaadmin';

#plan tests => 49;

my $number_of_tests_run = 0;

$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET $local_cgi`;
subtest "Type 'none'" => sub {
	like( $output, "/Select Type of Config Data You Wish To View/", "$local_cgi without params asks for config data" );
	#like( $output, "/<P><DIV CLASS='errorMessage'>Error: No command was specified</DIV></P>/", "$local_cgi without params shows an error" );
	done_testing(1);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="hosts";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Hosts</DIV></P>/", "$local_cgi with type=$type shows host configuration" );
        like( $output, "/<TH CLASS='data'>Host Name</TH>/", "$local_cgi with type=$type shows Host Name colum" );
        like( $output, "/<TH CLASS='data'>Alias/Description</TH>/", "$local_cgi with type=$type shows Alias/Description colum" );
        like( $output, "/<TH CLASS='data'>Address</TH>/", "$local_cgi with type=$type shows Address colum" );
        like( $output, "/<TH CLASS='data'>Parent Hosts</TH>/", "$local_cgi with type=$type shows Parent Hosts colum" );
        like( $output, "/<TH CLASS='data'>Max. Check Attempts</TH>/", "$local_cgi with type=$type shows Max. Check Attempts colum" );
        like( $output, "/<TH CLASS='data'>Check Interval</TH>/", "$local_cgi with type=$type shows Check Interval colum" );
        like( $output, "/<TH CLASS='data'>Retry Interval</TH>/", "$local_cgi with type=$type shows Retry Interval colum" );
        like( $output, "/<TH CLASS='data'>Host Check Command</TH>/", "$local_cgi with type=$type shows Host Check Command colum" );
        like( $output, "/<TH CLASS='data'>Check Period</TH>/", "$local_cgi with type=$type shows Check Period colum" );
        like( $output, "/<TH CLASS='data'>Obsess Over</TH>/", "$local_cgi with type=$type shows Obsess Over colum" );
        like( $output, "/<TH CLASS='data'>Enable Active Checks</TH>/", "$local_cgi with type=$type shows Enable Active Checks colum" );
        like( $output, "/<TH CLASS='data'>Enable Passive Checks</TH>/", "$local_cgi with type=$type shows Enable Passive Checks colum" );
        like( $output, "/<TH CLASS='data'>Check Freshness</TH>/", "$local_cgi with type=$type shows Check Freshness colum" );
        like( $output, "/<TH CLASS='data'>Freshness Threshold</TH>/", "$local_cgi with type=$type shows Freshness Threshold colum" );
        like( $output, "/<TH CLASS='data'>Default Contacts/Groups</TH>/", "$local_cgi with type=$type shows Default Contacts/Groups colum" );
        like( $output, "/<TH CLASS='data'>Notification Interval</TH>/", "$local_cgi with type=$type shows Notification Interval colum" );
        like( $output, "/<TH CLASS='data'>First Notification Delay</TH>/", "$local_cgi with type=$type shows First Notification Delay colum" );
        like( $output, "/<TH CLASS='data'>Notification Options</TH>/", "$local_cgi with type=$type shows Notification Options colum" );
        like( $output, "/<TH CLASS='data'>Notification Period</TH>/", "$local_cgi with type=$type shows Notification Period colum" );
        like( $output, "/<TH CLASS='data'>Event Handler</TH>/", "$local_cgi with type=$type shows Event Handler colum" );
        like( $output, "/<TH CLASS='data'>Enable Event Handler</TH>/", "$local_cgi with type=$type shows Enable Event Handler colum" );
        like( $output, "/<TH CLASS='data'>Stalking Options</TH>/", "$local_cgi with type=$type shows Stalking Options colum" );
        like( $output, "/<TH CLASS='data'>Enable Flap Detection</TH>/", "$local_cgi with type=$type shows Enable Flap Detection colum" );
        like( $output, "/<TH CLASS='data'>Low Flap Threshold</TH>/", "$local_cgi with type=$type shows Low Flap Threshold colum" );
        like( $output, "/<TH CLASS='data'>High Flap Threshold</TH>/", "$local_cgi with type=$type shows High Flap Threshold colum" );
        like( $output, "/<TH CLASS='data'>Flap Detection Options</TH>/", "$local_cgi with type=$type shows Flap Detection Options colum" );
        like( $output, "/<TH CLASS='data'>Process Performance Data</TH>/", "$local_cgi with type=$type shows Process Performance Data colum" );
        like( $output, "/<TH CLASS='data'>Enable Failure Prediction</TH>/", "$local_cgi with type=$type shows Enable Failure Prediction colum" );
        like( $output, "/<TH CLASS='data'>Failure Prediction Options</TH>/", "$local_cgi with type=$type shows Failure Prediction Options colum" );
        like( $output, "/<TH CLASS='data'>Notes</TH>/", "$local_cgi with type=$type shows Notes colum" );
        like( $output, "/<TH CLASS='data'>Notes URL</TH>/", "$local_cgi with type=$type shows Notes URL colum" );
        like( $output, "/<TH CLASS='data'>Action URL</TH>/", "$local_cgi with type=$type shows Action URL colum" );
        like( $output, "/<TH CLASS='data'>2-D Coords</TH>/", "$local_cgi with type=$type shows 2-D Coords colum" );
        like( $output, "/<TH CLASS='data'>3-D Coords</TH>/", "$local_cgi with type=$type shows 3-D Coords colum" );
        like( $output, "/<TH CLASS='data'>Statusmap Image</TH>/", "$local_cgi with type=$type shows Statusmap Image colum" );
        like( $output, "/<TH CLASS='data'>VRML Image</TH>/", "$local_cgi with type=$type shows VRML Image colum" );
        like( $output, "/<TH CLASS='data'>Logo Image</TH>/", "$local_cgi with type=$type shows Logo Image colum" );
        like( $output, "/<TH CLASS='data'>Image Alt</TH>/", "$local_cgi with type=$type shows Image Alt colum" );
        like( $output, "/<TH CLASS='data'>Retention Options</TH>/", "$local_cgi with type=$type shows Retention Options colum" );
        done_testing(40);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="hostdependencies";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Host Dependencies</DIV></P>/", "$local_cgi with type=$type shows hostdependency configuration" );
        like( $output, "/<TH CLASS='data'>Dependent Host</TH>/", "$local_cgi with type=$type shows Dependent Host colum" );
        like( $output, "/<TH CLASS='data'>Master Host</TH>/", "$local_cgi with type=$type shows Master Host colum" );
        like( $output, "/<TH CLASS='data'>Dependency Type</TH>/", "$local_cgi with type=$type shows Dependency Type colum" );
        like( $output, "/<TH CLASS='data'>Dependency Period</TH>/", "$local_cgi with type=$type shows Dependency Period colum" );
        like( $output, "/<TH CLASS='data'>Dependency Failure Options</TH>/", "$local_cgi with type=$type shows Dependency Failure Options colum" );
        done_testing(6);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="hostescalations";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Host Escalations</DIV></P>/", "$local_cgi with type=$type shows hostescalations configuration" );
        like( $output, "/<TH CLASS='data'>Host</TH>/", "$local_cgi with type=$type shows Host colum" );
        like( $output, "/<TH CLASS='data'>Contacts/Groups</TH>/", "$local_cgi with type=$type shows Contacts/Groups colum" );
        like( $output, "/<TH CLASS='data'>First Notification</TH>/", "$local_cgi with type=$type shows First Notification colum" );
        like( $output, "/<TH CLASS='data'>Last Notification</TH>/", "$local_cgi with type=$type shows Last Notification colum" );
        like( $output, "/<TH CLASS='data'>Notification Interval</TH>/", "$local_cgi with type=$type shows Notification Interval colum" );
        like( $output, "/<TH CLASS='data'>Escalation Period</TH>/", "$local_cgi with type=$type shows Escalation Period colum" );
        like( $output, "/<TH CLASS='data'>Escalation Options</TH>/", "$local_cgi with type=$type shows Escalation Options colum" );
        done_testing(8);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="hostgroups";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Host Groups</DIV></P>/", "$local_cgi with type=$type shows Hostgroups configuration" );
        like( $output, "/<TH CLASS='data'>Group Name</TH>/", "$local_cgi with type=$type shows Group Name colum" );
        like( $output, "/<TH CLASS='data'>Description</TH>/", "$local_cgi with type=$type shows Description colum" );
        like( $output, "/<TH CLASS='data'>Host Members</TH>/", "$local_cgi with type=$type shows Host Members colum" );
        like( $output, "/<TH CLASS='data'>Notes</TH>/", "$local_cgi with type=$type shows Notes colum" );
        like( $output, "/<TH CLASS='data'>Notes URL</TH>/", "$local_cgi with type=$type shows Notes URL colum" );
        like( $output, "/<TH CLASS='data'>Action URL</TH>/", "$local_cgi with type=$type shows Action URL colum" );
        done_testing(7);
};
$number_of_tests_run++;

# Tests against type 'services'
$type="services";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Services</DIV></P>/", "$local_cgi with type=$type shows Service configuration" );
        like( $output, "/<TH CLASS='data'>Host</TH>/", "$local_cgi with type=$type shows Host colum" );
        like( $output, "/<TH CLASS='data'>Description</TH>/", "$local_cgi with type=$type shows Description colum" );
        like( $output, "/<TH CLASS='data'>Max. Check Attempts</TH>/", "$local_cgi with type=$type shows Max. Check Attempts colum" );
        like( $output, "/<TH CLASS='data'>Normal Check Interval</TH>/", "$local_cgi with type=$type shows Normal Check Interval colum" );
        like( $output, "/<TH CLASS='data'>Retry Check Interval</TH>/", "$local_cgi with type=$type shows Retry Check Interval colum" );
        like( $output, "/<TH CLASS='data'>Check Command</TH>/", "$local_cgi with type=$type shows Check Command colum" );
        like( $output, "/<TH CLASS='data'>Check Period</TH>/", "$local_cgi with type=$type shows Check Period colum" );
        like( $output, "/<TH CLASS='data'>Parallelize</TH>/", "$local_cgi with type=$type shows Parallelize colum" );
        like( $output, "/<TH CLASS='data'>Volatile</TH>/", "$local_cgi with type=$type shows Volatile colum" );
        like( $output, "/<TH CLASS='data'>Obsess Over</TH>/", "$local_cgi with type=$type shows Obsess Over colum" );
        like( $output, "/<TH CLASS='data'>Enable Active Checks</TH>/", "$local_cgi with type=$type shows Enable Active Checks colum" );
        like( $output, "/<TH CLASS='data'>Enable Passive Checks</TH>/", "$local_cgi with type=$type shows Enable Passive Checks colum" );
        like( $output, "/<TH CLASS='data'>Check Freshness</TH>/", "$local_cgi with type=$type shows Check Freshness colum" );
        like( $output, "/<TH CLASS='data'>Freshness Threshold</TH>/", "$local_cgi with type=$type shows Freshness Threshold colum" );
        like( $output, "/<TH CLASS='data'>Default Contacts/Groups</TH>/", "$local_cgi with type=$type shows Default Contacts/Groups colum" );
        like( $output, "/<TH CLASS='data'>Enable Notifications</TH>/", "$local_cgi with type=$type shows Enable Notifications colum" );
        like( $output, "/<TH CLASS='data'>Notification Interval</TH>/", "$local_cgi with type=$type shows Notifivation Interval colum" );
        like( $output, "/<TH CLASS='data'>First Notification Delay</TH>/", "$local_cgi with type=$type shows First Notification Delay colum" );
        like( $output, "/<TH CLASS='data'>Notification Options</TH>/", "$local_cgi with type=$type shows Notification Options colum" );
        like( $output, "/<TH CLASS='data'>Notification Period</TH>/", "$local_cgi with type=$type shows Notification Period colum" );
        like( $output, "/<TH CLASS='data'>Event Handler</TH>/", "$local_cgi with type=$type shows Event Handler colum" );
        like( $output, "/<TH CLASS='data'>Stalking Options</TH>/", "$local_cgi with type=$type shows Stalking Options colum" );
        like( $output, "/<TH CLASS='data'>Enable Flap Detection</TH>/", "$local_cgi with type=$type shows Enable Flap Detection colum" );
        like( $output, "/<TH CLASS='data'>Low Flap Threshold</TH>/", "$local_cgi with type=$type shows Low Flap Threshold colum" );
        like( $output, "/<TH CLASS='data'>High Flap Threshold</TH>/", "$local_cgi with type=$type shows High Flap Threshold colum" );
        like( $output, "/<TH CLASS='data'>Flap Detection Options</TH>/", "$local_cgi with type=$type shows Flap Detection Options colum" );
        like( $output, "/<TH CLASS='data'>Process Performance Data</TH>/", "$local_cgi with type=$type shows Process Performance Data colum" );
        like( $output, "/<TH CLASS='data'>Enable Failure Prediction</TH>/", "$local_cgi with type=$type shows Enable Failure Prediction colum" );
        like( $output, "/<TH CLASS='data'>Notes</TH>/", "$local_cgi with type=$type shows Notes colum" );
        like( $output, "/<TH CLASS='data'>Notes URL</TH>/", "$local_cgi with type=$type shows Notes URL colum" );
        like( $output, "/<TH CLASS='data'>Action URL</TH>/", "$local_cgi with type=$type shows Action URL colum" );
        like( $output, "/<TH CLASS='data'>Logo Image</TH>/", "$local_cgi with type=$type shows Logo Image colum" );
        like( $output, "/<TH CLASS='data'>Image Alt</TH>/", "$local_cgi with type=$type shows Image Alt colum" );
        like( $output, "/<TH CLASS='data'>Retention Options</TH>/", "$local_cgi with type=$type shows Retention Options colum" );
        done_testing(35);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="servicegroups";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Service Groups</DIV></P>/", "$local_cgi with type=$type shows Hostgroups configuration" );
        like( $output, "/<TH CLASS='data'>Group Name</TH>/", "$local_cgi with type=$type shows Group Name colum" );
        like( $output, "/<TH CLASS='data'>Description</TH>/", "$local_cgi with type=$type shows Description colum" );
        like( $output, "/<TH CLASS='data'>Service Members</TH>/", "$local_cgi with type=$type shows Service Members colum" );
        like( $output, "/<TH CLASS='data'>Notes</TH>/", "$local_cgi with type=$type shows Notes colum" );
        like( $output, "/<TH CLASS='data'>Notes URL</TH>/", "$local_cgi with type=$type shows Notes URL colum" );
        like( $output, "/<TH CLASS='data'>Action URL</TH>/", "$local_cgi with type=$type shows Action URL colum" );
        done_testing(7);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="servicedependencies";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Service Dependencies</DIV></P>/", "$local_cgi with type=$type shows hostdependency configuration" );
        like( $output, "/<TH CLASS='data' COLSPAN=2>Dependent Service</TH>/", "$local_cgi with type=$type shows Dependent Service colum" );
        like( $output, "/<TH CLASS='data' COLSPAN=2>Master Service</TH>/", "$local_cgi with type=$type shows Master Service colum" );
        like( $output, "/<TH CLASS='data'>Host</TH>/", "$local_cgi with type=$type shows Master Service colum" );
        like( $output, "/<TH CLASS='data'>Service</TH>/", "$local_cgi with type=$type shows Master Service colum" );
        like( $output, "/<TH CLASS='data'>Dependency Type</TH>/", "$local_cgi with type=$type shows Dependency Type colum" );
        like( $output, "/<TH CLASS='data'>Dependency Period</TH>/", "$local_cgi with type=$type shows Dependency Period colum" );
        like( $output, "/<TH CLASS='data'>Dependency Failure Options</TH>/", "$local_cgi with type=$type shows Dependency Failure Options colum" );
        done_testing(8);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="serviceescalations";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Service Escalations</DIV></P>/", "$local_cgi with type=$type shows hostescalations configuration" );
        like( $output, "/<TH CLASS='data' COLSPAN=2>Service</TH>/", "$local_cgi with type=$type shows Service colum" );
        like( $output, "/<TH CLASS='data'>Host</TH>/", "$local_cgi with type=$type shows Host colum" );
        like( $output, "/<TH CLASS='data'>Description</TH>/", "$local_cgi with type=$type shows Description colum" );
        like( $output, "/<TH CLASS='data'>Contacts/Groups</TH>/", "$local_cgi with type=$type shows Contacts/Groups colum" );
        like( $output, "/<TH CLASS='data'>First Notification</TH>/", "$local_cgi with type=$type shows First Notification colum" );
        like( $output, "/<TH CLASS='data'>Last Notification</TH>/", "$local_cgi with type=$type shows Last Notification colum" );
        like( $output, "/<TH CLASS='data'>Notification Interval</TH>/", "$local_cgi with type=$type shows Notification Interval colum" );
        like( $output, "/<TH CLASS='data'>Escalation Period</TH>/", "$local_cgi with type=$type shows Escalation Period colum" );
        like( $output, "/<TH CLASS='data'>Escalation Options</TH>/", "$local_cgi with type=$type shows Escalation Options colum" );
        done_testing(10);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="contacts";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Contacts</DIV></P>/", "$local_cgi with type=$type shows Contacts configuration" );
        like( $output, "/<TH CLASS='data'>Contact Name</TH>/", "$local_cgi with type=$type shows Contact Name colum" );
        like( $output, "/<TH CLASS='data'>Alias</TH>/", "$local_cgi with type=$type shows Alias colum" );
        like( $output, "/<TH CLASS='data'>Email Address</TH>/", "$local_cgi with type=$type shows Email Address colum" );
        like( $output, "/<TH CLASS='data'>Pager Address/Number</TH>/", "$local_cgi with type=$type shows Pager Address/Number colum" );
        like( $output, "/<TH CLASS='data'>Service Notification Options</TH>/", "$local_cgi with type=$type shows Service Notification Options colum" );
        like( $output, "/<TH CLASS='data'>Host Notification Options</TH>/", "$local_cgi with type=$type shows Host Notification Options colum" );
        like( $output, "/<TH CLASS='data'>Service Notification Period</TH>/", "$local_cgi with type=$type shows Service Notification Period colum" );
        like( $output, "/<TH CLASS='data'>Host Notification Period</TH>/", "$local_cgi with type=$type shows Host Notification Period colum" );
        like( $output, "/<TH CLASS='data'>Service Notification Commands</TH>/", "$local_cgi with type=$type shows Service Notification Command colum" );
        like( $output, "/<TH CLASS='data'>Host Notification Commands</TH>/", "$local_cgi with type=$type shows Host Notification Command colum" );
        like( $output, "/<TH CLASS='data'>Retention Options</TH>/", "$local_cgi with type=$type shows Retention Options colum" );
        done_testing(12);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="contactgroups";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Contact Groups</DIV></P>/", "$local_cgi with type=$type shows Contact Groups configuration" );
        like( $output, "/<TH CLASS='data'>Group Name</TH>/", "$local_cgi with type=$type shows Group Name colum" );
        like( $output, "/<TH CLASS='data'>Description</TH>/", "$local_cgi with type=$type shows Description colum" );
        like( $output, "/<TH CLASS='data'>Contact Members</TH>/", "$local_cgi with type=$type shows Contact Members colum" );
        done_testing(4);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="timeperiods";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Time Periods</DIV></P>/", "$local_cgi with type=$type shows Time Periods configuration" );
        like( $output, "/<TH CLASS='data'>Name</TH>/", "$local_cgi with type=$type shows Name colum" );
        like( $output, "/<TH CLASS='data'>Alias/Description</TH>/", "$local_cgi with type=$type shows Alias/Description colum" );
        like( $output, "/<TH CLASS='data'>Exclusions</TH>/", "$local_cgi with type=$type shows Exclusions colum" );
        like( $output, "/<TH CLASS='data'>Days/Dates</TH>/", "$local_cgi with type=$type shows Days/Dates colum" );
        like( $output, "/<TH CLASS='data'>Times</TH>/", "$local_cgi with type=$type shows Times colum" );
        done_testing(6);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$type="commands";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&type=$type' $local_cgi`;
subtest "Type $type" => sub {
        like( $output, "/<P><DIV ALIGN=CENTER CLASS='dataTitle'>Commands</DIV></P>/", "$local_cgi with type=$type shows Commands configuration" );
        like( $output, "/<TH CLASS='data'>Command Name</TH>/", "$local_cgi with type=$type shows Command Name colum" );
        like( $output, "/<TH CLASS='data'>Command Line</TH>/", "$local_cgi with type=$type shows Command Line colum" );
        done_testing(3);
};
$number_of_tests_run++;


done_testing($number_of_tests_run);

