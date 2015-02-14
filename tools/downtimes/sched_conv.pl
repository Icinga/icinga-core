#!/usr/bin/perl -w

#*****************************************************************************
#
# sched_conv.pl - Convert downtime definitions to Icinga format
#
# Copyright (c) 2011-2015 Icinga Development Team (http://www.icinga.org)
#
# License:
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#****************************************************************************/
#
# 2011.11.15:  0.01 Initial version
# 2012.03.07:  0.02 "compress" weekdays to one definition line

use strict;

my $creator = "2012 Icinga Team";
my $version = "0.02";
my $script  = "sched_conv.pl";

if ((defined @ARGV) and ($ARGV[0] =~ m/-h/)) {
	help();
	exit 0;
}
my $iFile = $ARGV[0] || "schedule.cfg";
my $oFile = $ARGV[1] || "downtime.cfg";
my $h = my $s = my $a = my $c = my $r = my $hg = my $sg = "";
	my $b = my $e = my $f = my $d = my $m = 0;
my %day = (mon => 1, tue => 2, wed => 3, thu => 4, fri => 5, sat => 6, sun => 7);
my @day = ('','monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday', 'sunday');
open (IFILE, "$iFile") || die "Error opening $iFile, RC=$!";
open (OFILE, ">$oFile") || die "Error creating $oFile, RC=$!";
while (<IFILE>) {
	chomp;
	next if (/^\s*#/);
	s/^\s+//g;
	s/\s+$//g;
	my ($o,$v) = /(.*?)\s+(.*)/;
	if (defined($v)) {
		$h = $v if (/^host_name/);
		$hg = $v if (/^hostgroup_name/);
		$sg = $v if (/^servicegroup_name/);
		$s = $v if (/^service_description/);
		$b = $v if (/^start|time/);
		$e = $v if (/^end/);
		$d = $v if (/^duration/);
		$f = $v if (/^fixed/);
		$a = $v if (/^user/);
		$c = $v if (/^comment/);
		$r = lc($v) if (/^days_of_week/);
		$m = $v if (/^days_of_month/);
	}	
	if (/}/) {
		print OFILE "define downtime {\n";
		print OFILE "   host_name           $h\n" if ($h);
		print OFILE "   hostgroups          $hg\n" if ($hg);
		print OFILE "   servicegroups       $sg\n" if ($sg);
		print OFILE "   service_description $s\n" if ($s);
		print OFILE "   author              $a\n" if ($a);
		print OFILE "   comment             $c\n" if ($c);
		print OFILE "   fixed               $f\n" if ($f);
		print OFILE "   duration            $d\n" if ($d);
		if ($r) {
			# convert names to numbers
			for (keys %day) {
				$r =~ s/$_/$day{$_}/;
			}
			my @w = sort (split(/,/,$r));
			$r = $day[$w[0]];
			for (1..$#w) {
				if ($w[$_-1]+1 != $w[$_]) {
					$r .= " - $day[$w[$_-1]],$day[$w[$_]]";
				}
			}
			$r .= " - $day[$w[$#w]]" if ($r ne $day[$w[$#w]]);
			my @r = split(/,/,$r);
			for my $i (0..$#r) {
				print OFILE "   downtime_period     $r[$i] $b";
				print OFILE "-$e" if ($e);
				print OFILE "\n";
			}
		}
		if ($m) {
			my @m = split(/,/,$m);
			$m = $m[0];
			for (1..$#m) {
				if ($m[$_-1]+1 != $m[$_]) {
					$m .= " - $m[$_-1],$m[$_]";
				}
			}
			$m .= " - $m[$#m]" if ($m ne $m[$#m]);
			@m = split(/,/,$m);
			for my $i (0..$#m) {
				print OFILE "   downtime_period     day $m[$i] $b";
				print OFILE "-$e" if ($e);
				print OFILE "\n";
			}
		}
		if ($r and $m) {
			print "--- $h,$s: ---\n";
			print "days_of_week and days_of_month in one definition\n";
			print "This will work different now!!!\n";
		}
		if ((! $r) and (! $m)) {
			print OFILE "   downtime_period     monday - sunday $b";
			print OFILE "-$e" if ($e);
			print OFILE "\n";
		}
		print OFILE "}\n";
		$h = $s = $a = $c = $r = $m = "";
		$b = $e = $f = $d = 0;
	}
}
close (IFILE);
close (OFILE);
exit;

sub help {
	print <<EOT
$script $version - Copyright $creator

The script may be used to convert downtime definitions created by Steve
Shipway's script which are written to "schedule.cfg".
It might be used as well to convert definitions written by NagiosXI to
the file "recurringdowntime.cfg".
 
$script [input file [output file]]

Called without any arguments the script sets
input file := schedule.cfg
output file:= downtime.cfg

Caveat:
Please note that definitions containing "days_of_week" AND 
"days_of_month" at the same time are NOT converted correctly (and I
wonder who may need such a combination). 

EOT
}
