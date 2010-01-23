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
my $local_cgi = "$cgi_dir/histogram.cgi";

my $output;

my $input = '';
my $remote_user = 'REMOTE_USER=icingaadmin';

#plan tests => 49;

my $number_of_tests_run = 0;


$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck' $local_cgi`;
subtest "No Args" => sub {
	like( $output, "/Step 1: Select Report Type/", "$local_cgi without params asks for report type" );
	like( $output, "/<option value=gethost>Host/", "$local_cgi shows Host in dropdown list" );
	like( $output, "/<option value=getservice>Service/", "$local_cgi shows Service in dropdown list" );
	like( $output, "/<input type='submit' value='Continue to Step 2'>/", "$local_cgi shows Continue to Step 2 Button" );
	done_testing(4);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$input="gethost";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&input=$input' $local_cgi`;
subtest "Input $input" => sub {
        like( $output, "/<DIV CLASS='reportSelectTitle'>Step 2: Select Host</DIV>/", "$local_cgi with input=$input shows right header" );
        like( $output, "/<tr><td class='reportSelectSubTitle' valign=center>Host:</td>/", "$local_cgi with input=$input shows Host Name colum" );
	like( $output, "/<option value='host1'>host1/", "$local_cgi shows example Host in dropdown list" );
	like( $output, "/<input type='submit' value='Continue to Step 3'>/", "$local_cgi '$input' shows Continue to Step 3 Button" );
        done_testing(4);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$input="getoptions&host=host1";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&input=$input' $local_cgi`;
subtest "Input $input" => sub {
        like( $output, "/<DIV CLASS='reportSelectTitle'>Step 3: Select Report Options</DIV>/", "$local_cgi with input=$input shows right header" );
        like( $output, "/Report Period:/", "$local_cgi with input=$input shows Report Period Dropdown Box " );
        like( $output, "/Start Date \\(Inclusive\\)/", "$local_cgi with input=$input shows Start Date (Inclusive)" );
        like( $output, "/End Date \\(Inclusive\\):/", "$local_cgi with input=$input shows End Date (Inclusive)" );
        like( $output, "/Statistics Breakdown/", "$local_cgi with input=$input shows Statistics Breakdown" );
        like( $output, "/Events To Graph/", "$local_cgi with input=$input shows Events to Graph" );
        like( $output, "/State Types To Graph/", "$local_cgi with input=$input shows State Types to Graph" );
        like( $output, "/Assume State Retention:/", "$local_cgi with input=$input shows Assume State Retention" );
        like( $output, "/Initial States Logged/", "$local_cgi with input=$input shows Initial States Logged" );
        like( $output, "/Ignore Repeated States/", "$local_cgi with input=$input shows Ignore Repeated States" );
	like( $output, "/<input type='submit' value='Create Report'>/", "$local_cgi '$input' shows Continue to Step 3 Button" );
        done_testing(11);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$input="getservice";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&input=$input' $local_cgi`;
subtest "Input $input" => sub {
        like( $output, "/<DIV CLASS='reportSelectTitle'>Step 2: Select Service</DIV>/", "$local_cgi with input=$input shows right header" );
        like( $output, "/<tr><td class='reportSelectSubTitle'>Service:</td>/", "$local_cgi with input=$input shows Host Name colum" );
	like( $output, "/<option value='Dummy service'>host1;Dummy service/", "$local_cgi shows example Service in dropdown list" );
	like( $output, "/<input type='submit' value='Continue to Step 3'>/", "$local_cgi shows Continue to Step 3 Button" );
        done_testing(4);
};
$number_of_tests_run++;

# Tests against type 'hosts'
$input="getoptions&host=host1&service=Dummy+service";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg $remote_user REQUEST_METHOD=GET QUERY_STRING='nodaemoncheck&input=$input' $local_cgi`;
subtest "Input $input" => sub {
        like( $output, "/<DIV CLASS='reportSelectTitle'>Step 3: Select Report Options</DIV>/", "$local_cgi with input=$input shows right header" );
        like( $output, "/Report Period:/", "$local_cgi with input=$input shows Report Period Dropdown Box " );
        like( $output, "/Start Date \\(Inclusive\\)/", "$local_cgi with input=$input shows Start Date (Inclusive)" );
        like( $output, "/End Date \\(Inclusive\\):/", "$local_cgi with input=$input shows End Date (Inclusive)" );
        like( $output, "/Statistics Breakdown/", "$local_cgi with input=$input shows Statistics Breakdown" );
        like( $output, "/Events To Graph/", "$local_cgi with input=$input shows Events to Graph" );
        like( $output, "/State Types To Graph/", "$local_cgi with input=$input shows State Types to Graph" );
        like( $output, "/Assume State Retention:/", "$local_cgi with input=$input shows Assume State Retention" );
        like( $output, "/Initial States Logged/", "$local_cgi with input=$input shows Initial States Logged" );
        like( $output, "/Ignore Repeated States/", "$local_cgi with input=$input shows Ignore Repeated States" );
	like( $output, "/<input type='submit' value='Create Report'>/", "$local_cgi '$input' shows Continue to Step 3 Button" );
        done_testing(11);
};
$number_of_tests_run++;

$input="host=host1&timeperiod=last7days&smon=1&sday=1&syear=2010&shour=0&smin=0&ssec=0&emon=1&eday=23&eyear=2010&ehour=24&emin=0&esec=0&breakdown=dayofmonth&graphevents=120&graphstatetypes=3&assumestateretention=yes&initialstateslogged=no&newstatesonly=no";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg REMOTE_USER=icingaadmin REQUEST_METHOD=GET QUERY_STRING="nodaemoncheck&$input" $local_cgi`;
like( $output, "/<IMG SRC='histogram.cgi\\?createimage/", "$local_cgi for host shows an IMG SRC" );
$number_of_tests_run++;

$input="host=host1&service=Dummy+service&timeperiod=last7days&smon=1&sday=1&syear=2010&shour=0&smin=0&ssec=0&emon=1&eday=23&eyear=2010&ehour=24&emin=0&esec=0&breakdown=dayofmonth&graphevents=120&graphstatetypes=3&assumestateretention=yes&initialstateslogged=no&newstatesonly=no";
$output = `NAGIOS_CGI_CONFIG=etc/cgi.cfg REMOTE_USER=icingaadmin REQUEST_METHOD=GET QUERY_STRING="nodaemoncheck&$input" $local_cgi`;
like( $output, "/<IMG SRC='histogram.cgi\\?createimage/", "$local_cgi for service shows an IMG SRC" );
$number_of_tests_run++;

done_testing($number_of_tests_run);

