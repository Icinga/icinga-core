#!/usr/bin/perl

# host urgency related checks for status.cgi
# sorting by host urgency is a feature introduced in #1452

# Introducing a new status.cgi sort mode (sortoption=16384) where hosts
# are sorted by state, but with the order being based on "urgency"
# (down>unreachable>pending>ok) instead of the historically assigned (?)
# state IDs (unreachable>down>up>pending).

# cgi-hosturgencies.cfg includes 4 host in all states
# (down,unreachable,up,pending)

use warnings;
use strict;
use Test::More;
use Icinga::Test qw ( run_cgi );

plan tests => 3;


# sorting by names, so we expect host1,host2,host3,host4
my $output = run_cgi('etc/cgi-hosturgencies.cfg', 'GET', 'hostgroup=all&style=hostdetail', 'status.cgi');
like( $output, '/host1.*host2.*host3.*host4/msx', "List of hosts sorted by ascending name" );

# sorting by by ascending status (see above), so we epext 4,1,2,3
$output = run_cgi('etc/cgi-hosturgencies.cfg', 'GET',
    'hostgroup=all&style=hostdetail&sortobject=hosts&sorttype=1&sortoption=8', 'status.cgi');
like( $output, '/host4.*host1.*host2.*host3/mxs', "List of hosts sorted by ascending status" );

# sorting the new way, so we expect 2,3,4,1
$output = run_cgi('etc/cgi-hosturgencies.cfg', 'GET',
    'hostgroup=all&style=hostdetail&sortobject=hosts&sorttype=2&sortoption=9', 'status.cgi');
like( $output, '/host2.*host3.*host4.*host1/mxs', "List of hosts sorted by descending urgency" );
