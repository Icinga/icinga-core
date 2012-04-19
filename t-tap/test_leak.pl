#!/usr/bin/perl
# slighty modified from the initial idea of mod_gearman
# https://github.com/sni/mod_gearman/blob/master/t/test_all.pl


use warnings;
use strict;
use Test::More;

my $pwd = `pwd`;
chomp $pwd;

# clean up old gearmands
#`ps -efl | grep gearmand | grep 54730 | awk '{ print \$4 }' | xargs kill`;

`type valgrind >/dev/null 2>&1`;
if($? != 0) {
    plan skip_all => 'valgrind required';
}

my $makeprep = `cd ..; make clean 2>&1 && make icinga && make cgis 2>&1; cd $pwd`;
is($?, 0, "make icinga && cgis is $?") or BAIL_OUT("no need to test without successful make icinga && make cgis!\n".$makeprep);

my $makeout = `make clean  2>&1 && make 2>&1`;
is($?, 0, "build rc is $?") or BAIL_OUT("no need to test without successful make!\n".$makeout);

my $skip_perl_mem_leaks = "";
#if(`grep -c '^#define EMBEDDEDPERL' config.h` > 0) {
#    $skip_perl_mem_leaks = "--suppressions=./t/valgrind_suppress.cfg";
#}

my $today 	 = `date '+%Y-%m-%d'`;
chomp $today;
my $vallog       = 'var/log/valgrind-'.$today.'.log';
my $testlog      = 'var/log/icinga_test-'.$today.'.log';
my $suppressions = 'var/log/suppressions-'.$today.'.log';
`>$suppressions`;
my @tests = $ARGV[0] || split/\s+/, `grep ^TESTS Makefile.in | awk -F = '{print \$2}'`;
for my $test (@tests) {
    next if $test =~ m/^\s*$/;
    `make $test 2>/dev/null`;
    is($?, 0, "$test build rc is $?");

    #my $cmd = "yes | valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes --track-origins=yes $skip_perl_mem_leaks --gen-suppressions=yes --log-file=$vallog ./$test >$testlog 2>&1";
    my $cmd = "yes | valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes --track-origins=yes $skip_perl_mem_leaks --log-file=$vallog ./$test >$testlog -v 2>&1";
    #diag($cmd);
    `$cmd`;
    is($?, 0, "$test valgrind exit code is $?") or diag(`cat $testlog`);

    `cat $vallog >> $suppressions`;

    is(qx(grep "ERROR SUMMARY: " $vallog | grep -v "ERROR SUMMARY: 0 errors"), "", "valgrind Error Summary")
      or BAIL_OUT("check memory $test in $vallog");
}

unlink($vallog);
unlink($testlog);

#diag(`ls -la $suppressions`);

done_testing();
