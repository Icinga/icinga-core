#!/usr/bin/perl -w

#*****************************************************************************
#
# log_hist.pl - Extract host/service information from log file(s)
#
# Copyright (c) 2012 Icinga Development Team (http://www.icinga.org)
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
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#****************************************************************************/
#
# 2012.07.13:  0.01 wn Initial version

use strict;

my $creator = "2012 Icinga Team";
my $version = "0.01";
my $script  = "log_hist.pl";

if (((defined @ARGV) and ($ARGV[0] =~ m/-h/)) or (! defined @ARGV)) {
   help();
   exit 0;
}

my ($host,$service,$files) = @ARGV;
$files = "*log" unless (defined $files);
my @files   = glob ("$files");
my %ts      = ();
my $hstate  = "?";
if (defined $service) {
   undef $service unless ($service);
}

for my $idx (0..$#files) {
   my $iFile = $files[$idx];
#print "$iFile\n";
   open (IFILE, "$iFile") || die "error during open of $iFile, RC=$!";
   while (<IFILE>) {
      chomp;
      next unless (/[ |;]$host;/);
      if (/SERVICE/) {
         if (defined $service) {
            next unless (/;$service/);
         } else {
            next;
         }
      }
      my ($ts,$alert,$data) = $_ =~ /\[(\d+)\] (.*?): (.*);/;
      next unless (defined $ts);
      my ($ss,$mi,$hh,$dd,$mm,$yy) = (localtime($ts))[0..5];
      $mm++;
      $yy+=1900;
      my $obj = (/HOST/) ? "H_" : ".S";
      my @fields = split(/;/,$data);
      my $no = "";
      my $state = "";
      my $type  = "";
      if (/EXTERNAL COMMAND/) {
         if (/_SVC/) {
            next unless (defined $service);
         }
         $state = $fields[0];
      } elsif (/HOST/) {
         $state = $fields[1];
         unless (/DOWNTIME|FLAPPING/) {
            $type = $fields[2];
            $no = $fields[3];
            if (/NOTIFICATION/) {
               $state = $fields[2];
               $type = $fields[3];
               $no = "($fields[0])";
            }
         } elsif (/DOWNTIME/) {
            $no = "($hstate)";
         }
         $hstate = "$state;$type";
      } elsif (/SERVICE/) {
         $state = $fields[2];
         unless (/DOWNTIME|FLAPPING/) {
            $type = $fields[3];
            $no = $fields[4];
            if (/NOTIFICATION/) {
               $state = $fields[3];
               $type = $fields[4];
               $no = "($fields[0])";
            }
         } elsif (/DOWNTIME/) {
            $no = "($hstate)";
         }
      } else {
         print "???: $_";
         next;
      }
      $ts{$ts.$obj} = sprintf "%4d.%02d.%02d %02d:%02d:%02d %s %-22s ", $yy,$mm,$dd,$hh,$mi,$ss,$obj,$alert;
      $ts{$ts.$obj} .= sprintf "%-8s %4s %s\n", $state, $type, $no;
   }
   close (IFILE);
}
foreach (sort keys %ts) {
   print $ts{$_};
}
exit 0;

sub help {
   print <<EOT
$script $version - Copyright $creator

The script may be used to extract information on hosts and/or services 
based on Icinga/Nagios log files.
The files are searched in the current directory.

$script <host name> [<service description>] [<file(s)>]

Use "" as service description if you search for a host and want to specify
file names other than the default (*.log)

<file(s)> defaults to "*.log"

EOT
}
