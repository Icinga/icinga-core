#!/usr/bin/perl

package Icinga::Test;

use strict;
use warnings;
use Carp;

use Exporter 'import';
our @EXPORT_OK = qw( run_cgi get_body run_cmd slurp_file );

use IPC::Run3 qw( run3 );

use FindBin qw($Bin);
use Env qw( DEBUG );

my $topdir = "$Bin/..";
my $cgi_dir = "$topdir/cgi";
my $cgi = "$cgi_dir/status.cgi";

sub run_cgi ($$$$) {
    my ($config, $method, $query_string, $cgi) = @_;
    chdir $Bin or die "Cannot chdir";

    my $cmd = sprintf("ICINGA_CGI_CONFIG='%s' REMOTE_USER=icingaadmin REQUEST_METHOD='%s' QUERY_STRING='%s' %s",
        $config,
        $method,
        $query_string,
        "$cgi_dir/$cgi"
    );
    print STDERR "\nDEBUG: execute $cmd\n", if $DEBUG;
    my ($in, $out, $err) = '';
    run3 ($cmd, \$in, \$out, \$err) or die "cat: $? - $! - $err";
    print STDERR "\nError executing $cmd: \n $err\n" if $err;
    return $out;
}

sub run_cmd (@) {
    my $cmd = shift;

    my ($in, $out, $err) = '';
    print STDERR "\nDEBUG: execute '". join(" ", @$cmd) . "'\n", if $DEBUG;
    run3 ($cmd, \$in, \$out, \$err) or die "cat: $? - $! - $err";
    print STDERR "\nError executing $cmd: \n $err\n" if $err;
    return $out;
}

sub get_body ($) {
    my $output = shift;
    # remove cr from output
    $output =~ s/\r//g;
    my $body; my $flag = 0;
    foreach my $line (split("\n", $output)) {
        $flag = 1 if $line =~ /^$/;
        $body .= $line if $flag;
    }
    return $body;
}

sub slurp_file ($) {
    my $filename = shift;
    carp "Filename $filename not found" unless -f $filename;
    open (my $fh, '<', $filename) 
        or carp "Could not open $filename for reading: $!";
    my $content = do { local $/; <$fh> };
    close($fh);
    return $content;
}
1;
