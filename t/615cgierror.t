#!/usr/bin/perl

# Check against all cgis that you get an appropriate error is raised when the CGI config file does not exist


use warnings;
use strict;
use Test::More;
use Icinga::Test qw ( run_cgi );

use FindBin qw($Bin);

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $cgi_dir = "$topdir/cgi";

# get all cgis from our bindir
opendir(my $dh, $cgi_dir) or die "Cannot opendir $cgi_dir: $!";
my %cgis = map { ( $_ => 1 ) } grep /\.cgi$/, readdir $dh;
closedir $dh;


plan tests => scalar keys %cgis;

# loop over all cgis and see if they return an error
foreach my $cgi (sort keys %cgis) {
	my $output = `ICINGA_CGI_CONFIG=etc/cgi.nonexistant REQUEST_METHOD=GET $cgi_dir/$cgi`;
	like( $output, "/Error: Could not open CGI (config|configuration) file 'etc/cgi.nonexistant' for reading/", "Found error for $cgi" );
}

