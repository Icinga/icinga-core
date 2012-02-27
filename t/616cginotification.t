#!/usr/bin/perl

# Checks for notifications.cgi
# 

use warnings;
use strict;
use Test::More;
use Icinga::Test qw ( run_cgi );

plan tests => 5;

my $output = run_cgi('etc/cgi.cfg', 'GET', 'nodaemoncheck',
    'notifications.cgi');
like( $output, "/<input type='hidden' name='host' value='all'>/", "Host value set to all if nothing set" );

$output = run_cgi('etc/cgi.cfg', 'GET', 'nodaemoncheck&host=all',
    'notifications.cgi');
like( $output, "/<input type='hidden' name='host' value='all'>/", "Host value set to all if host=all set" );

$output = run_cgi('etc/cgi.cfg', 'GET', 'nodaemoncheck&host=hostA',
    'notifications.cgi');
like( $output, "/<input type='hidden' name='host' value='hostA'>/", "Host value set to host in query string" );

$output = run_cgi('etc/cgi.cfg', 'GET', 'nodaemoncheck&contact=all',
    'notifications.cgi');
like( $output, "/<input type='hidden' name='contact' value='all'>/", "Contact value set to all from query string" );

$output = run_cgi('etc/cgi.cfg', 'GET', 'nodaemoncheck&contact=fred',
    'notifications.cgi');
like( $output, "/<input type='hidden' name='contact' value='fred'>/", "Contact value set to fred from query string" );
