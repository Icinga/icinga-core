#!/usr/bin/perl

package Icinga::Test;

use strict;
use warnings;

use Exporter 'import';
our @EXPORT_OK = qw( run_cgi);

use IPC::Run3 qw( run3 );

use FindBin qw($Bin);

my $topdir = "$Bin/..";
my $cgi_dir = "$topdir/cgi";
my $cgi = "$cgi_dir/status.cgi";

sub run_cgi ($$$$) {
    my ($config, $method, $query_string, $cgi) = @_;
    chdir $Bin or die "Cannot chdir";

    my $cmd = sprintf("ICINGA_CGI_CONFIG=%s REMOTE_USER=icingaadmin REQUEST_METHOD=%s QUERY_STRING=%s %s",
        $config,
        $method,
        $query_string,
        "$cgi_dir/$cgi"
    );
    my @test = qw ( date );
    my ($in, $out, $err) = '';
    run3 ($cmd, \$in, \$out, \$err) or die "cat: $? - $! - $err";
    return $out;
}

1;
