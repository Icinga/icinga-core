#!/usr/bin/perl
#
# Local Checks for extinfo.cgi

use warnings;
use strict;
use Test::Most;
use Icinga::Test qw ( run_cgi get_body );

my $tests = 4;

my $output = run_cgi('etc/cgi.cfg', 'GET', '', 'extinfo.cgi');
like( $output, "/Process Information/", "extinfo.cgi without params show the process information" );

$output = run_cgi('etc/cgi.cfg', 'GET', 'type=1&host=host1', 'extinfo.cgi');
like( $output, "/Schedule downtime for this host and all services/", "extinfo.cgi allows us to set downtime for a host and all of his services" );

# test the json exporter
SKIP: {
    eval { require Test::JSON };
    skip "Test::JSON not installed", 2 if $@;

    use Test::JSON;
    $output = run_cgi('etc/cgi.cfg', 'GET', 'jsonoutput', 'extinfo.cgi');

    is_valid_json(get_body($output), 'json output should be well formed');

    use JSON;
    my $data = decode_json(get_body($output));
    cmp_deeply($data->{extinfo}->{process_info}, superhashof({
                'icinga_pid' => 48451,
                'program_start_time' => '05-15-2009 00:56:31',
      }), 'Check some contents of json output');
}

done_testing($tests);
