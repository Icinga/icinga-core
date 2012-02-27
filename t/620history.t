#!/usr/bin/perl

use warnings;
use strict;
use FindBin qw($Bin);

use Test::More;

eval "use Test::HTML::Lint";
if ($@) {
	plan skip_all => "Need Test::HTML::Lint";
}

eval "use Test::WWW::Mechanize::CGI";
if ($@) {
	plan skip_all => "Need Test::WWW::Mechanize::CGI";
}

my $lint;
eval '$lint = HTML::Lint->new( only_types => HTML::Lint::Error::STRUCTURE )';

plan tests => 4;

chdir $Bin or die "Cannot chdir";

my $topdir = "$Bin/..";
my $cgi_dir = "$topdir/cgi";

my $mech = Test::WWW::Mechanize::CGI->new;

$mech->env( ICINGA_CGI_CONFIG => "etc/cgi.cfg" );
$mech->cgi_application("$cgi_dir/history.cgi");

$mech->get_ok("http://localhost/");

$mech->title_is("History", 'Title is \'History\'');

$mech->content_contains('[05-15-2009 00:56:31]  Icinga 1&#46;2&#46;1 starting&#46;&#46;&#46; &#40;PID&#61;48451&#41;',
    'Contains Icinga startup message');
html_ok( $lint, $mech->content, "HTML correct" );

