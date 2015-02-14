#!/usr/bin/perl -w

#*****************************************************************************
#
# sched_down.pl - Schedule recurring downtimes for Icinga
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#****************************************************************************/
#
# 2011.11.18:  0.01 Initial version
# 2012.03.05:  0.02 Easter based holidays, local holidays, examine mode
# 2012.03.14:  0.03 reworked calculation of start / end dates, changed hash key
# 2012.03.15:  0.04 add "-t" to specify deviating date/time
# 2012.03.19:  0.05 bugfix: enable no blank before opening brace, missing srv
# 2012.03.20:  0.06 set name for local holiday file, add some perfdata
# 2012.03.21:  0.07 bugfix: fix end date check; splitted code
# 2012.03.22:  0.08 bugfix: set author/comment if blank
#                   enable no blank before opening brace (2nd try)
# 2012.07.13:  0.09 bugfix: fix exit from procedure
# 2012.09.24:  0.10 bugfix: fix "all" services
# 2012.10.11:  0.11 bugfix: fix calculation of fixed/flex
# 2013.02.04:  0.12 bugfix: fix start when fixed date; fix examine host
# 2014.05.07:  0.13 bugfix: fix single service; omit duplicate CMD-entries
#                   fix start calculation
# 2014.06.07:  0.14 add workdays

use strict;
use Getopt::Long qw(:config no_ignore_case bundling);

eval ("use Date::Calc qw(:all)");
if ($@) {
		print <<EOT;
Perl module Date::Calc is missing
Please install using "cpan Date::Calc" or via your package manager
EOT
	exit 3;
}

#
# There are no user serviceable parts below this line
#

my $creator = "2011-2014 Icinga Team";
my $version = "0.14";
my $script  = "sched_down.pl";

my $cFile = "/usr/local/icinga/etc/icinga.cfg";
my $dFile = "/usr/local/icinga/etc/downtime.cfg"; # downtime definition
my $lFile = "/usr/local/icinga/etc/holiday.cfg";  # local date / holiday definitions

my $max_ahead  = 2;	# plan schedules max. days ahead 
my $max_ahead2 = 7;	# schedules must end within n days
my $debug      = $ENV{DEBUG} || 1;
my $examine    = "";	# only examine schedules
my $examine_ts = "";
my $forecast   = 0;	# forecast schedule
my $help       = 0;
my $sFile      = "";	# status file
my $oFile      = "";	# objects cache
my $cPipe      = "";	# command pipe
my %cDowntimes = ();	# current downtimes
my @sDowntimes = ();	# schedule definitions
my %pDowntimes = ();	# planned downtimes
my %tmp        = ();
my %sObject    = ();	# srv per host
my $dt         = "";	# date/time info
my $cTime      = time();
my $timestamp  = "";
my %numbers    = (scheduled => 0, defined => 0, new => 0);
my %hg         = ();	# hosts per hostgroup
my %sg         = ();	# hosts per servicegroup
my %holiday    = ();	# contains holiday definitions using names
my %days       = (mon => 1, tue => 2, wed => 3, thu => 4, fri => 5, sat => 6, sun => 7);
my %months     = (jan => 1, feb => 2, mar => 3, apr => 4, may => 5, jun => 6,
                  jul => 7, aug => 8, sep => 9, 'oct' => 10, nov => 11, dec => 12);
my @fixed      = ("flex","fixed","ff_unset");
my %day_off    = ();	# non-workdays, v0.14
my @workdays   = ();	# workdays within a certain month, v0.14

# process command line parameters
Getopt::Long::Configure('bundling');
GetOptions(
    "h|help"        => \$help,
    "c|config=s"    => \$cFile,
    "s|schedule=s"  => \$dFile,
    "l|local=s"     => \$lFile,
    "d|debug=s"     => \$debug,
    "m|max_ahead=s" => \$max_ahead,
    "e|examine=s"   => \$examine,
    "f|forecast"    => \$forecast,
    "t|timestamp=s" => \$timestamp,
);

if ($help) {
	help();
	exit 0;
}

set_date_time ();
flexible_holidays ();
local_holidays ($lFile,5) if (-f "$lFile");
print_special_dates ();

if ($examine) {
	my $tmp = init_entry ();
	$tmp->{host_name} = "dummyhost";
	$tmp->{comment}   = "examine";
	$tmp->{period}    = $examine;
	push @sDowntimes, $tmp;
	$sObject {"dummyhost;"}++;			# 0.12
} else {
	read_config_file ($cFile,1);
 	%cDowntimes = read_status_file ($sFile,2);
	my ($ref_hg, $ref_sg) = read_object_file ($oFile,3);
	%hg = %$ref_hg;
	%sg = %$ref_sg;
#	(%hg,%sg) = read_object_file ($oFile,3);
	@sDowntimes = read_downtimes ($dFile,4);
}
plan_downtimes ();
select STDOUT;
printf "Already scheduled: %d, Definitions: %d, newly planned: %d | Existing=%d defined=%d new=%d\n",
	$numbers{scheduled},$numbers{defined},$numbers{new},$numbers{scheduled},$numbers{defined},$numbers{new} unless ($examine);
exit 0;

#
# Subroutines
#

#
# init entry for scheduled / defined downtimes
#
sub init_entry {
	my $self = {
		object              => "",
		host_name           => 0,
		service_description => "",
		hostgroups          => "",
		servicegroups       => "",
		start_time          => 0,
		end_time            => 0,
		triggered_by        => 0,
		period              => "",
		duration            => 0,
		fixed               => 2,
		author              => "icingaadmin",
		comment             => "$script $version",
		propagate           => 0,
		register            => 1,
	};
	return $self;
}

#
# init entry for hostgroups / servicegroups
#
sub init_entry2 {
	my $self = {
		object              => "",
		type                => "",
		alias               => "",
		members             => "",
	};
	return $self;
}

#
# init entry for date/time definition
#
sub init_dt {
	my $self = {
		start_ts => -1,
		start_y  => -1,
		start_m  => -1,
		start_w  => -1,
		start_d  =>  0,
		start_l  =>  0,	# v0.14
		start_hh => -1,
		start_mm => -1,
		end_ts   => -1,
		end_y    => -1,
		end_m    => -1,
		end_w    => -1,
		end_d    =>  0,
		end_l    =>  0,	# v0.14
		end_hh   => -1,
		end_mm   => -1,
		every    => -1,
	};
	return $self;
}

#
# read main config file to retrieve location of several other files
#
sub read_config_file {
	my ($iFile,$rc) = @_;
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
	while (<IFILE>) {
		chomp;
		($sFile) = $_ =~ /status_file=(.*)/ if (/^status_file/);
		($oFile) = $_ =~ /object_cache_file=(.*)/ if (/^object_cache_file/);
		($cPipe) = $_ =~ /command_file=(.*)/ if (/^command_file/);
	}
	close (IFILE);
	info (1,"status_file=$sFile");
	info (1,"object_cache_file=$oFile");
	info (1,"command_file=$cPipe");
}

#
# read status file to get info about planned downtimes
#
sub read_status_file {
	my ($iFile,$rc) = @_;
	my %Downtimes = ();
	my $tmp = init_entry ();
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
	info (2, "Processing $iFile");
	while (<IFILE>) {
		chomp;
		s/#.*//;
		s/^\s+//;
		s/\s+$//;
		next if (/^$/);
		if (/^(\S+)\s+{/) {
			$tmp->{object} = $1;
			next;
		}
		if (/}/) {
			if ($tmp->{object} =~ /hostdowntime|servicedowntime/) {
				$numbers{scheduled}++;
				$Downtimes {"$tmp->{host_name};$tmp->{service_description};$tmp->{start_time};$tmp->{end_time}"} = $tmp;
			}
			if ($tmp->{object} =~ /hoststatus|servicestatus/) {
				$sObject {"$tmp->{host_name};$tmp->{service_description}"}++;
			}
			$tmp = init_entry ();
		}
		if (my ($obj,$val) = /(.*?)=(.*)/) {
			$tmp->{$obj} = $val if (exists($tmp->{$obj}));
		}
	}
	close (IFILE);
	info (2, "--- current entries from $iFile ---");
	for my $key (sort keys %Downtimes) {
		info (2,"$key: $Downtimes{$key}{host_name},$Downtimes{$key}{service_description},$Downtimes{$key}{comment}");
	}
	return %Downtimes;
}

#
# read object cache file to get info about hostgroups/servicegroups/services
#
sub read_object_file {
	my ($iFile,$rc) = @_;
	my %hGroup = ();
	my %sGroup = ();
	my $ok = 0;
	my $tmp = init_entry2 ();
	my %srv = ();
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
	info (2, "Processing $iFile");
	while (<IFILE>) {
		chomp;
		s/#.*//;
		s/^\s+//;
		s/\s+$//;
		next if (/^$/);
		if (/^define\s+(\S+)\s*{/) {
			$tmp->{object} = $1;
			$ok = 1 if ($1 =~ /hostgroup|service/);
			next;
		}
		next unless ($ok);
		if (/}/) {
			if ($tmp->{object} =~ /hostgroup/) {
				$hGroup {"$tmp->{name}"} = $tmp;
			}
			if ($tmp->{object} =~ /servicegroup/) {
				$sGroup {"$tmp->{name}"} = $tmp;
			}
# services per host
			if ($tmp->{object} =~ /service$/) {
				my %t = %srv;
				if (exists ($sObject {"$tmp->{host_name}"})) {
					%t = (%t, %{$sObject {"$tmp->{host_name}"}});
				}
				$sObject {"$tmp->{host_name}"} = \%t;
			}
			$tmp = init_entry2 ();
			%srv = ();
			next;
		}
		my ($obj,$val) = /(.*?)\s+(.*)/;
		next unless (defined $val);	# skip if value is empty
		if ($obj =~ /(.+)group_name/) {
			$tmp->{type} = $1;
			$tmp->{name} = $val;
		} elsif ($obj =~ /service_description/) {
			$srv{lc($val)} = $val;
		} elsif (/^\w+/) {
			$tmp->{$obj} = $val;
		}
	}
	close (IFILE);
	info (2, "--- current entries from $iFile ---");
	for my $key (sort keys %hGroup) {
		info (2,"HG $key ($hGroup{$key}{alias}): $hGroup{$key}{members}");
	}
	for my $key (sort keys %sGroup) {
		info (2,"SG $key ($sGroup{$key}{alias}): $sGroup{$key}{members}");
	}
	return (\%hGroup, \%sGroup);
}

#
# read downtime definition file
#
sub read_downtimes {
	my ($iFile,$rc) = @_;
	my @Downtimes = ();
	my $ok = 0;
	my $tmp = init_entry ();
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
	info (2, "Processing $iFile");
	while (<IFILE>) {
		chomp;
		s/\s*#.*//;
		s/^\s+//;
		s/\s+$//;
		s/\s+/ /g;
		s/\s*;.*//;
		next if /^$/;
		if (/define\s+(\S+)\s*{/) {
			$tmp->{object} = $1;
			$ok = 1 if ($1 =~ /downtime/);
			next;
		}
		next unless ($ok);
		if (/}/) {
			$numbers{defined}++;
			$tmp->{host_name} = $tmp->{hostgroups} if ($tmp->{hostgroups});
			$tmp->{host_name} = $tmp->{servicegroups} if ($tmp->{servicegroups});
			push @Downtimes, $tmp;
			$tmp = init_entry ();
			$ok = 0;
			next;
		}
		my ($obj,$val) = /(.*?)\s+(.*)/;
		if (/_period/) {
			$tmp->{period} .= "$val;";
		} elsif (/^\w+/) {
			$tmp->{$obj} = $val;
		}
	}
	close (IFILE);
	info (2, "--- current entries from $iFile ---");
	for my $key (0..$#Downtimes) {
		info (2,"Schedule: $Downtimes[$key]{host_name};$Downtimes[$key]{service_description};$Downtimes[$key]{duration} mins,$fixed[$Downtimes[$key]{fixed}]");
	}
	return @Downtimes;
}

#
# process local holiday definitions
#
sub local_holidays {
	my ($iFile,$rc) = @_;
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
	info (2, "Processing $iFile");
	while (<IFILE>) {
		chomp;
		s/\s*#.*//;
		s/^\s+//;
		s/\s+$//;
		s/\s*;.*//;
		next if /^$/;
		if (m#(\S+)\s+(\S+)#) {
			my $var = lc($1);
			my $calc = lc($2);
			if ($calc =~ /date_easter/i) {
				if ($calc =~ /\(([+|-]?\d+)\)/) {
					$holiday{$var} = date_easter ("$1");
					next;
				}
			} elsif ($calc =~ /day_in_week_of_month/i) {
				if ($calc =~ /\((\d+),(\d),([+|-]?\d).*\)/) {
					$holiday{$var} = date_in_week_of_month ("$1","$2","$3");
					next;
				}
# v0.14 ---> 
			} elsif ($calc =~ /day_off/i) {
				if (exists $holiday{$var}) {
					$day_off{$var} = 1;
					$day_off{$holiday{$var}} = 1;
				} else {
					info (0, "holiday \"$var\" is not defined");
				}
				next;
# v0.14 <---
			}
		}
		info (0, "Format of $_ seems to be wrong");
	}
	close (IFILE);
}

#
# plan new downtimes, skip existing ones
# 
sub plan_downtimes {
# loop through downtime definitions
	for my $index (0..$#sDowntimes) {
		next unless ($sDowntimes[$index]{register});
		my @hst = split(/,/,$sDowntimes[$index]{host_name});
		my @srv = split (/,/,$sDowntimes[$index]{service_description});
		push @srv, "" unless @srv;
		my $c = $sDowntimes[$index]{comment} || "recurring downtime";
		my $duration = $sDowntimes[$index]{duration};
		my $a = $sDowntimes[$index]{author} || "$script $version";
		my @r = split(/;/,$sDowntimes[$index]{period});
		my $f = $sDowntimes[$index]{fixed};
		my $hg = $sDowntimes[$index]{hostgroups};
		my $sg = $sDowntimes[$index]{servicegroups};
		my $p = $sDowntimes[$index]{propagate};
		my $t = 0;
		my $diff = 0;
		for (0..$#hst) {
			my $h = $hst[$_];
			for (0..$#srv) {
				my $s = $srv[$_];
				my $key = "$h;$s;$c";
	 			info (2,"-- Entry $key: $sDowntimes[$index]{duration} mins on: $sDowntimes[$index]{period}");
				if ($hg) {
					unless (exists ($hg{"$h"})) {
						info (2, "object $h does not exist");
						next;
					}
				} elsif ($sg) {
					unless (exists ($sg{"$h"})) {
						info (2, "object $h does not exist");
						next;
					}
				} elsif ($s ne "all")  {
					unless (exists ($sObject{"$h;$s"})) {
						info (2, "object $h;$s does not exist");
						next;
					}
				}
	# loop through date/time definitions
				for my $i (0..$#r) {
					my @cmd = ();
					$dt = init_dt ();
					info (1, "planning $key: $r[$i] $timestamp");
					my ($ds, $ts) = $r[$i] =~ /^(.*?)\s(\d+:.*)/;
					unless (defined ($ts)) {
						info (0, "no timeperiod HH:MM-HH:MM defined");
						exit 11;
					}
					unless (defined ($ds)) {
						info (0, "no dates defined");
						exit 12;
					}
					$examine_ts = "$ds $ts $timestamp";
					my @ts = split(",",$ts.",");
					my ($year,$mon,$mday,$hour,$min,$sec,$yday,$wday,$isdst) = Localtime($cTime);
					info (2, "DS:$ds / TS:$ts");
					if ($ds =~ m#(.*)/\s*(\d+)#) {
						($ds,$dt->{every}) = ($1,$2);
					}
		# start and end defined
					if ($ds =~ m#(\S.*)\s+-\s+(\S.*)#) {
						next if analyse_period ("start",$1);
						next if analyse_period ("end",$2);
						$dt->{end_m} = $dt->{start_m} if (($dt->{start_m} > 0) and ($dt->{end_m} < 0));
					}
					else {
		# set end to start if no end definition present
						next if analyse_period ("start",$1);
						if ($dt->{every} < 0) {
							$dt->{end_y} = $dt->{start_y};
							$dt->{end_m} = $dt->{start_m};
							$dt->{end_w} = $dt->{start_w};
							$dt->{end_d} = $dt->{start_d};
							$dt->{end_l} = $dt->{start_l};
							$dt->{end_ts} = $dt->{start_ts};
						}
					}
	
		# loop through time definitions
					for my $j (0..$#ts) {
						next if (check_ts ($dt, $ts[$j], $duration));
						workdays_in_month ($mon);
						next if (set_start ($dt, $cTime, $year, $mon, $mday));
	
						my $offs = 0;
						my $midnight = ($dt->{start_hh}*60+$dt->{start_mm} >= $dt->{end_hh}*60+$dt->{end_mm});	# across midnight?
						if ($dt->{end_hh} == 24) {	# end time 24:00?
							$dt->{end_hh} = 0;
							$midnight = 1;
						}
						info (2, "F ".fmt_dt ($dt->{start_ts},1,1));	
						next if (start_loop ($dt,$year));
						# adjust possible DST change
						$dt->{start_ts} = Mktime ((Localtime($dt->{start_ts}))[0..2],$dt->{start_hh},$dt->{start_mm},0);
			
						$dt->{end_ts} = $dt->{start_ts};
						$dt->{end_ts} = Mktime ((Localtime($dt->{end_ts}))[0..2],$dt->{end_hh},$dt->{end_mm},0);
						next if (check_end ($dt));
						info (2, "E1:".fmt_dt ($dt->{end_ts},1,1)." midnight: $midnight");
						($year,$mon,$mday) = Add_Delta_YMD ((Localtime($dt->{end_ts}))[0..2],0,0,$midnight);
						$dt->{end_ts} = Mktime ($year,$mon,$mday,$dt->{end_hh},$dt->{end_mm},0);
		
						if (exists($cDowntimes{"$h;$s;$dt->{start_ts};$dt->{end_ts}"})) {
							info (1,"Rejected: ==> already planned $h;$s:".fmt_dt($dt->{start_ts},1,1)." to ".fmt_dt($dt->{end_ts},1,1)." for $duration mins");
							next;
						}
						my $diff = $dt->{end_ts} - $dt->{start_ts};
						$f = ($diff == ($duration*60)) ? 1 : 0;
						$f = 1 if ($duration == 0);
						my $data = "$dt->{start_ts};$dt->{end_ts};$f;$t;".($duration*60).";$a;$c";
						next if (set_cmd(\@cmd,$dt,$key,$data,$hg,$sg,$h,$s,$c,$p,$duration));
	
						for (0..$#cmd) {
							info (0,"CMD:$cmd[$_]");
							my @f = split (/;/,$cmd[$_]);
							my $srv = $f[2];
							my $start = $f[3];
							my $end = $f[4];
							if ($f[0] =~ /HOST/) {
								$start = $f[2];
								$end = $f[3];
								$srv = "";
							}
							my ($yy1,$mm1,$dd1,$hh1,$mi1) = (Localtime($start))[0..5];
							my ($yy2,$mm2,$dd2,$hh2,$mi2) = (Localtime($end))[0..5];
							$f[0] = sprintf "%04d.%02d.%02d %02d:%02d - %04d.%02d.%02d %02d:%02d %s",
								$yy1,$mm1,$dd1,$hh1,$mi1,$yy2,$mm2,$dd2,$hh2,$mi2,$f[1];
							$f[0] .= ";".$srv if ($srv);
							$tmp{"$f[0]"}++;
						}
						$cDowntimes{"$h;$s;$dt->{start_ts};$dt->{end_ts}"}++;
					}
				}
			}
		}
	}
	foreach (keys %cDowntimes) {
		my ($host,$srv,$start,$end) = split (/;/,$_);
		my ($yy1,$mm1,$dd1,$hh1,$mi1) = (Localtime($start))[0..5];
		my ($yy2,$mm2,$dd2,$hh2,$mi2) = (Localtime($end))[0..5];
		my $key = sprintf "%04d.%02d.%02d %02d:%02d - %04d.%02d.%02d %02d:%02d %s",
			$yy1,$mm1,$dd1,$hh1,$mi1,$yy2,$mm2,$dd2,$hh2,$mi2,$host;
		$key .= ";".$srv if ($srv);
		$tmp{"$key"}++;
	}
	my $old = "";
	if ($debug) {
		info (1, "--- Debug enabled so NO commands will be sent to $cPipe! ---");
	}
	if ($examine) {
		info (0, "--- Examine enabled so NO commands will be sent to $cPipe! ---");
	}
	if ($forecast) {
		info (0, "--- Forecast enabled so NO commands will be sent to $cPipe! ---");
	}
	info (2,"--- Scheduled downtimes ---");
	foreach (sort keys %tmp) {
		info (2, $_);
	}
	for my $key (sort keys %pDowntimes) {
		if ($old ne $key) {
			if ($examine) {
				my @f = split(/;/,$pDowntimes{$key});
				print localtime($f[2])." to ". localtime($f[3]).", ".$examine_ts."\n";
			} else {
				print "CMD: [$cTime] $pDowntimes{$key}\n";
			}
			unless ($debug or $examine or $forecast) {
				$numbers{new}++;
				open (CMD,">$cPipe") || info (0,"Error opening $cPipe, RC=$!") && exit 5;
				print CMD "[$cTime] $pDowntimes{$key}\n";
				close (CMD);
			}
	   }
		else {
			info (2, "o=n: [$cTime] $pDowntimes{$key}");
		}
		$old = $key;
	}
}	

# check if downtime is already planned
sub already_planned {
	my ($ptr,$type,$step,$key,$key2,$duration) = @_;
	my @array = @{$ptr};
	my $cnt = 0;
	for (my $idx = 0; $idx <= $#array; $idx+=$step) {
		$cnt++ if (exists($cDowntimes{"$array[$idx];$key2"}));
	}
	if ($cnt) {
		info (1,"Rejected: ==> already planned ${type}s $key:".localtime($dt->{start_ts})." to ".localtime($dt->{end_ts})." for $duration mins");
	}
	return $cnt;
}

# examine date of given period
# for examples and explanation see 
# http://docs.icinga.org/latest/objectdefinitions.html#objectdefinitions-timeperiod
sub analyse_period {
	my ($index, $ds) = @_;
   $ds =~ s/\s+$//;
	$ds = lc($ds);
   info (2,"$index => :$ds:");
   if ($ds =~ m#^(easter.*?)([+|-]\d+)\s*#) {
		my $holiday = lc($1);
		info (2,"flex holiday $1 $2");
   	if (exists $holiday{$holiday}) { # flexible holiday
      	($dt->{$index."_y"},$dt->{$index."_m"},$dt->{$index."_d"}) = (Localtime(($holiday{$holiday}+$2*86400)))[0..2];
			return;
		}
	}
   my ($nb) = $ds =~ m#^(\S+)\s*#;
   if (exists $holiday{$nb}) { # flexible holiday
		info (2,"flex holiday $nb");
      ($dt->{$index."_y"},$dt->{$index."_m"},$dt->{$index."_d"}) = (Localtime($holiday{lc($1)}))[0..2];
		return;
   }
	if ($ds =~ m#(\d+)-(\d+)-(\d+)#) { # exact date: 2011-10-31
		info (2, "exact date: $1.$2.$3");
		$dt->{$index."_y"} = $1;
		$dt->{$index."_m"} = $2;
		$dt->{$index."_d"} = $3;
	}
	elsif ($ds =~ m#^day\s+(\S+)#) { # day: day 5; day -1
		info (2, "day of month: $1");
		$dt->{$index."_d"} = $1;
	}
# v0.14 --->
	elsif ($ds =~ m#^workday\s+(\S+)\s+(\S..)?#) { # workday: workday -1 april
		info (2, "workday $1 of month: $2");
		$dt->{$index."_l"} = $1;
		$dt->{$index."_m"} = $months{lc($2)} if (defined $2);
	}
	elsif ($ds =~ m#^workday\s+(\S+)#) { # workday: workday 5; workday -1
		info (2, "workday of month: $1");
		$dt->{$index."_l"} = $1;
	}
# v0.14 <---
	elsif ($ds =~ m#(\S..).*?day(\s*\S.*)?#) { # weekday: friday
		info (2, "weekday: $1");
		$dt->{$index."_w"} = $days{lc($1)};
		if (defined $2) {
			info (2, "of week: $2");
			$ds =~ s/.*?day\s+//;
			if ($ds =~ m#(\S+?)\s+(\S..)?#) { # monday 2; tuesday -1; sunday 1 april
				$dt->{$index."_d"} = $1;
				$dt->{$index."_m"} = $months{lc($2)} if (defined $2);
			} else {
				$dt->{$index."_d"} = $ds if (defined $ds);
			}
		}
	}
	elsif ($ds =~ m#(\S..).*?\s+(\S+)#) { # month: april 10
		info (2, "month: $1");
		$dt->{$index."_m"} = $months{lc($1)};
		$dt->{$index."_d"} = $2;
	}
	elsif ($ds =~ m#^([-]?\d+)#) { # day ?
		info (2, "day2: $1");
		$dt->{$index."_d"} = $1;
	}
	else {
		info (0,"$ds: not a valid definition");
		return 1;
	}
	return 0;
}

# check time definition, check if end date already passed
sub check_ts {
	my ($dt,$ts,$duration) = @_;
	info (2, "Y/M/D/W/L Y/M/D/W/L E TS/Dur: $dt->{start_y}/$dt->{start_m}/$dt->{start_d}/$dt->{start_w}/$dt->{start_l} $dt->{end_y}/$dt->{end_m}/$dt->{end_d}/$dt->{end_w}/$dt->{end_l} $dt->{every} $ts/$duration");	#c v0.14
	if ($ts =~ /(\d+):(\d+)\s*-\s*(\d+):(\d+)/) {
		$dt->{start_hh} = $1;
		$dt->{start_mm} = $2;
		$dt->{end_hh} = $3;
		$dt->{end_mm} = $4;
	} elsif ($ts =~ /(\d+):(\d+)/) {
		$dt->{start_hh} = $1;
		$dt->{start_mm} = $2;
		$dt->{end_hh} = $dt->{start_hh};
		$dt->{end_mm} = $dt->{start_mm} + $duration % 60;
		if ($dt->{end_mm} > 59) {
			$dt->{end_mm} -= 60;
			$dt->{end_hh}++;
		}
		$dt->{end_hh} = $dt->{start_hh} + int($duration / 60);
		if ($dt->{end_hh} > 23) {
			$dt->{end_hh} -= 24;
		}
	} else {
		info (1, "$ts is invalid");
		return 1;
	}
	if (($dt->{end_y} > 0) and ($dt->{end_m} > 0) and ($dt->{end_d} > 0)) {
		if (Mktime ($dt->{end_y},$dt->{end_m},$dt->{end_d},23,59,59) < $cTime) {
			info (1, "end already passed");
			printf "%-52s, %s\n", "end already passed", $examine_ts if ($examine);
			return 1;
		}
	}
	return 0;
}

# try to set the earliest date
sub set_start {
	my ($dt,$cTime,$year,$mon,$mday) = @_;
	if ($dt->{start_y} > 0) {
		if ($dt->{start_m} > 0) {
			if ($dt->{start_d} > 0) {
				$dt->{start_ts} = Mktime ($dt->{start_y}, $dt->{start_m}, $dt->{start_d}, $dt->{start_hh},$dt->{start_mm},0);
				start_loop($dt,$year);
			}
		}	
	} else { # y <= 0
		if ($dt->{start_m} > 0) {
			if ($dt->{start_w} > 0) {
				if ($dt->{start_d} != 0) {
					$dt->{start_ts} = date_in_week_of_month ($dt->{start_m}, $dt->{start_w}, $dt->{start_d},$dt->{start_hh},$dt->{start_mm});
					start_loop($dt,$year);
				}
			} else { # w <= 0
				if ($dt->{start_d} > 0) {
					$dt->{start_ts} = Mktime ($year, $dt->{start_m}, $dt->{start_d},$dt->{start_hh},$dt->{start_mm},0);
					start_loop($dt,$year);
				} elsif ($dt->{start_d} < 0) {
					my $dim = Days_in_Month ($year, $dt->{start_m});
					$dt->{start_ts} = Mktime ($year, $dt->{start_m}, $dim+1+$dt->{start_d},$dt->{start_hh},$dt->{start_mm},0);

					if ($dt->{start_ts} < $cTime) {
						$dim = Days_in_Month ($year+1, $dt->{start_m});
						$dt->{start_ts} = Mktime ($year+1, $dt->{start_m}, $dim+1+$dt->{start_d},$dt->{start_hh},$dt->{start_mm},0);
					}
				} elsif ($dt->{start_l}) { # d = 0, l != 0
					workdays_in_month ($dt->{start_m});
					$dt->{start_ts} = Mktime ($year, $dt->{start_m}, $workdays[$dt->{start_l}],$dt->{start_hh},$dt->{start_mm},0);

					if ($dt->{start_ts} < $cTime) {
						$dt->{start_ts} = Mktime ($year+1, $dt->{start_m}, 1,$dt->{start_hh},$dt->{start_mm},0);
					}
				}	
			}
		} else {	# m <= 0
			if ($dt->{start_w} > 0) {
				if ($dt->{start_d} > 0) {
					$dt->{start_ts} = date_in_week_of_month ($mon, $dt->{start_w},$dt->{start_d},$dt->{start_hh},$dt->{start_mm});
					if ($dt->{start_ts} > $cTime + $max_ahead*86400) {
						$dt->{start_ts} = date_in_week_of_month ($mon+1, $dt->{start_w},$dt->{start_d},$dt->{start_hh},$dt->{start_mm});
					}
				} else {
					if ($dt->{start_w} == $dt->{end_w}) {
						$dt->{start_ts} = date_in_week_of_month ($mon, $dt->{start_w},$dt->{start_d},$dt->{start_hh},$dt->{start_mm});
					} else {	
						$dt->{start_ts} = Mktime ($year,$mon,1,$dt->{start_hh},$dt->{start_mm},0);
					}
				}
			} else { # w <= 0
				if ($dt->{start_d} > 0) {
					$dt->{start_ts} = Mktime ($year, $mon, $dt->{start_d},$dt->{start_hh},$dt->{start_mm},0);
#					$dt->{start_ts} = Mktime ($year, $mon, $mday, $dt->{start_hh},$dt->{start_mm},0);
				} elsif ($dt->{start_d} == 0) {
					if (($dt->{start_l}) and (defined($workdays[$dt->{start_l}]))) {
						$dt->{start_ts} = Mktime ($year, $mon, $workdays[$dt->{start_l}],$dt->{start_hh},$dt->{start_mm},0);
					} else {	
						$dt->{start_ts} = Mktime ($year, $mon, 1,$dt->{start_hh},$dt->{start_mm},0);
					}
				} else {
					my $dim = Days_in_Month ($year, $mon);
					$dt->{start_ts} = Mktime ($year, $mon, $dim+1+$dt->{start_d},$dt->{start_hh},$dt->{start_mm},0);
				}
			}
		}
	}
	if ($dt->{start_ts} > $cTime + $max_ahead*86400) {
		my $txt_ahead = " is more than $max_ahead day".(($max_ahead > 1) ? "s" : "")." away";
		info (1, "Rejected: ==> start" . $txt_ahead);
		return 1;
	}
	return 0;
}

sub start_loop {
	my ($dt,$year) = @_;
	my $diff = abs($dt->{every});
	my $loop = 1;
	my $s0 = 0;
	my $s1 = 0;
	my $e0 = 0;
	while ($loop) {
		if ($dt->{start_ts} > $cTime + $max_ahead*86400) {
			my $txt_ahead = " is more than $max_ahead day".(($max_ahead > 1) ? "s" : "")." away";
			info (1, "Rejected: ==> start" . $txt_ahead);
			return 1;
		}
		my ($y,$m,$d) = (Localtime($dt->{start_ts}))[0..2];
		my ($w) = (Localtime($dt->{start_ts}))[7];
		workdays_in_month($m);	# v0.14
		if (($dt->{start_d} > 0) and ($dt->{start_w} > 0)) {
			$s0 = Mktime (Nth_Weekday_of_Month_Year($y,$m,$dt->{start_w},$dt->{start_d}),$dt->{start_hh},$dt->{start_mm},0);
			if ($dt->{start_m} > 0) {
				$s1 = Mktime (Nth_Weekday_of_Month_Year($y+1,$dt->{start_m},$dt->{start_w},$dt->{start_d}),$dt->{start_hh},$dt->{start_mm},0);
			} else {
				$s1 = Mktime (Nth_Weekday_of_Month_Year($y+1,$m,$dt->{start_w},$dt->{start_d}),$dt->{start_hh},$dt->{start_mm},0);
			}
		}
		if (($dt->{end_d} > 0) and ($dt->{end_w} > 0)) {
			if ($dt->{end_m} > 0) {
				$e0 = Mktime (Nth_Weekday_of_Month_Year($y,$dt->{end_m},$dt->{end_w},$dt->{end_d}),$dt->{start_hh},$dt->{start_mm},0);
			} else {
				$e0 = Mktime (Nth_Weekday_of_Month_Year($y,$m,$dt->{end_w},$dt->{end_d}),$dt->{start_hh},$dt->{start_mm},0);
			}
		}
		$loop = 0;
		$loop |= ($y < $dt->{start_y})   << 0;
		$loop |= ($y > $dt->{end_y})     << 0 if ($dt->{end_y} > 0);
		$loop |= ($m < $dt->{start_m})   << 1;
		$loop |= ($m > $dt->{end_m})     << 1 if ($dt->{end_m} > 0);
		$loop |= ($d < $dt->{start_d})   << 2 if (($dt->{start_d} > 0) and ($dt->{start_w} < 0));
		$loop |= ($d > $dt->{end_d})     << 2 if (($dt->{end_d} > 0) and ($dt->{end_w} < 0));
		$loop |= ($dt->{start_ts} < $s0) << 3 if ($dt->{start_m} == $m);
		if (($dt->{end_d} > 0) and ($dt->{end_w} > 0)) {
			$loop |= ($dt->{start_ts} > $e0) << 4 if (($dt->{start_m} == $dt->{end_m}) and ($dt->{start_w} == $dt->{end_w}));
			$loop |= ($dt->{start_ts} > $e0) << 4 if ($m == $dt->{end_m});
			$loop |= ($dt->{start_ts} > $e0) << 4 if ($dt->{end_m} < 0);
			if (($e0 > 0) and ($dt->{start_ts} > $e0) and ($s1)) {
				$dt->{start_ts} = $s1;
				next;
			}
		}
		if (($dt->{end_d} > 0) and ($dt->{end_w} < 0) and ($d > $dt->{end_d})) {
			$dt->{start_ts} = Mktime ((Add_Delta_YMD($y,$m,$dt->{start_d},0,1,0)),$dt->{start_hh},$dt->{start_mm},0);
			next;
		}
		$loop |= ($w < $dt->{start_w}) << 5 if (($dt->{start_w} > 0) and ($dt->{start_d} == $dt->{end_d}));
		$loop |= ($w < $dt->{start_w}) << 5 if (($dt->{start_w} > 0) and ($m == $dt->{start_m}) and ($w < $dt->{start_d}));
		$loop |= ($w > $dt->{end_w})   << 6 if (($dt->{end_w} > 0) and ($dt->{end_m} > 0) and ($m == $dt->{end_m}));
		$loop |= ($w > $dt->{end_w})   << 6 if (($dt->{end_w} > 0) and ($dt->{end_m} < 0));
		$loop |= ($w > $dt->{end_w})   << 6 if (($dt->{end_w} > 0) and ($dt->{start_d} > 0) and ($dt->{start_d} == $dt->{end_d}));
		$loop |= ($dt->{start_ts} < $cTime) << 7;
		$loop |= ($d < $workdays[$dt->{start_l}]) << 8 if (($dt->{start_l}) and (defined($workdays[$dt->{start_l}])));	# v0.14
		$loop |= ($d > $workdays[$dt->{end_l}])   << 8 if (($dt->{end_l}) and (defined($workdays[$dt->{end_l}])));	# v0.14
		$loop |= (! defined($workdays[$dt->{start_l}])) << 8 if ($dt->{start_l});	# v0.14
		$loop |= (! defined($workdays[$dt->{end_l}]))   << 8 if ($dt->{end_l});	# v0.14
		info (2, "S " . fmt_dt ($dt->{start_ts},1,1) . " :" . unpack("B10",$loop|1024) . ": " . $s0 . " " . $e0);
		$dt->{start_ts} = Mktime (Add_Delta_YMD((Localtime($dt->{start_ts}))[0..2],0,0,$diff),$dt->{start_hh},$dt->{start_mm},0) if ($loop);
	}
	info (2, "L " . fmt_dt ($dt->{start_ts},1,1) . " :" . unpack("B10",$loop|1024) . ": " . $s0 . " " . $e0);
}

# check end date (current date < end < current date+max_ahead2)
sub check_end {
	my $txt_ahead = " is more than $max_ahead2 day".(($max_ahead2 > 1) ? "s" : "")." away";
	my ($dt) = @_;
	my $count = 0;
	my $loop = 1;
	while (($loop) and ($count <= $max_ahead)) {
		my ($year,$mon,$mday,$hour,$min,$sec,$yday,$wday,$isdst) = Localtime($dt->{end_ts});
		info (2, "E ".localtime($dt->{end_ts}));	
		if ($dt->{end_y} > 0) {
			$loop *= (($year >= $dt->{start_y}) and ($year <= $dt->{end_y}));
			info (3, "EY $loop");
		}
		if ($dt->{end_m} > 0) {
			$loop *= (($mon >= $dt->{start_m}) and ($mon <= $dt->{end_m}));
			info (3, "EM $loop");
		}
		if ($dt->{end_d} > 0) {
			if ($dt->{end_w} < 0) {
				$loop *= (($mday >= $dt->{start_d}) and ($mday <= $dt->{end_d}));
			}
			info (3, "ED $loop");
		}
		if (($dt->{end_w} > 0) and ($dt->{end_m} < 0)) {
			$loop *= (($wday >= $dt->{start_w}) and ($wday <= $dt->{end_w}));
			info (3, "EW $loop");
		}
		$loop |= ($dt->{end_ts} >= $cTime);
		info (3, "ET $loop");
		$count++ if ($dt->{every} < 0);
		$loop = not($loop);
		$dt->{end_ts} = Mktime (Add_Delta_YMD ($year,$mon,$mday,0,0,abs($dt->{every})),$dt->{end_hh},$dt->{end_mm},0) if ($loop);
	}
	if ($count > $max_ahead + $max_ahead2) {
		info (1, "Rejected: ==> end" . $txt_ahead);
		next;
	}
}

sub set_cmd {
	my ($cmd,$dt,$key,$data,$hg,$sg,$h,$s,$c,$p,$duration) = @_;
	my $extcmd = "";
	my $f = ($dt->{end_ts} - $dt->{start_ts} == $duration * 60) ? 1 : 0;
	my $key2 = "$s;$dt->{start_ts};$dt->{end_ts}";
	info (1, "HG:$hg, SG:$sg, H:$h, S:$s possibly on: ".localtime($dt->{start_ts})." to ".localtime($dt->{end_ts})." for $duration mins");
	if ("$h;$s" =~ /;$/) {	# no service_description
		if ($hg) {	# host_groups defined
			my @member = split (/,/,$hg{$h}->{members});
			return 1 if (already_planned (\@member,"HG",1,$key,$key2,$duration));
			$extcmd = "SCHEDULE_HOSTGROUP_HOST_DOWNTIME;$h;$data";
		} elsif ($sg) {
			my @mem = split (/,/,$sg{$h}->{members});
			my @member = ();
			for (my $idx = 0; $idx <= $#mem; $idx+=2) {
				push @member, $mem[$idx];
			}
			return 1 if (already_planned (\@member,"SG",2,$key,$key2,$duration));
			$extcmd = "SCHEDULE_SERVICEGROUP_HOST_DOWNTIME;$h;$data";
		} else {			# host_name defined
			if (exists($cDowntimes{"$h;;$dt->{start_ts};$dt->{end_ts}"})) {
				info (1, "Rejected: ==> already planned $key");
				return 1;
			}
			if ($p) {
		 		$extcmd = "SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME;$h;$data";
			} else {
		 		$extcmd = "SCHEDULE_HOST_DOWNTIME;$h;$data";
			}
		}
		add_cmd (\@$cmd, $h, $key2, $extcmd);
	} else {					# service_description defined
		if ($hg) {			# hostgroups defined
			my @member = split (/,/,$hg{$h}->{members});
			my $cnt = 0;
			if ($s =~ /^all$/i) {
				for my $idx (0..$#member) {
					my %srv = %{$sObject{$member[$idx]}};
					foreach my $idy (keys %srv) {
						$cnt++ if (exists($cDowntimes{"$member[$idx];$srv{$idy};$dt->{start_ts};$dt->{end_ts}"}));
					}
				}
				if ($cnt) {
					info (1,"Rejected: ==> already planned HGs $key:".localtime($dt->{start_ts})." to ".localtime($dt->{end_ts})." for $duration mins");
					return 1;
				}
				$extcmd = "SCHEDULE_HOSTGROUP_SVC_DOWNTIME;$h;$data";
				add_cmd (\@$cmd,$h,$key2,$extcmd);
			} else { # one or more services defined
				next if (already_planned (\@member,"HG",1,$key,$key2,$duration));
				for my $idx (0..$#member) {
					if (exists $sObject{"$member[$idx]"}->{lc($s)}) {
						$extcmd = "SCHEDULE_SVC_DOWNTIME;$member[$idx];$s;$data";
						add_cmd (\@$cmd,$member[$idx],$key2,$extcmd);
					}
				}	
				return 1;
			}
		} elsif ($sg) {	# servicegroups defined
			my @member = split (/,/,$sg{$h}->{members});
			if ($s =~ /^all$/i) {
				return 1 if (already_planned (\@member,"SG",2,$key,$key2,$duration));
				$extcmd = "SCHEDULE_SERVICEGROUP_SVC_DOWNTIME;$h;$data";
				add_cmd (\@$cmd,$h,$key2,$extcmd);
			} else {
				return 1 if (already_planned (\@member,"SG",2,$key,$key2,$duration));
				for my $idx (0..$#member) {
					if (exists $sObject{$member[$idx]}->{lc($s)}) {		# 0.13
						$extcmd = "SCHEDULE_SVC_DOWNTIME;$member[$idx];$s;$data";
						add_cmd (\@$cmd,$member[$idx],$key2,$extcmd);
					}
				}	
			}
		} else {				# host_name defined
			if ($s =~ /^all$/i) {
				my %srv = %{$sObject{$h}};
				my $cnt = 0;
				foreach my $idy (keys %srv) {
					$cnt++ if (exists($cDowntimes{"$h;$srv{$idy};$dt->{start_ts};$dt->{end_ts}"}));
				}
				if ($cnt) {
					info (1,"Rejected: ==> already planned Hs $key:".localtime($dt->{start_ts})." to ".localtime($dt->{end_ts})." for $duration mins");
					return 1;
				}
	 			$extcmd = "SCHEDULE_HOST_SVC_DOWNTIME;$h;$data";
			} else {
				if (exists($cDowntimes{"$h;$s;$dt->{start_ts};$dt->{end_ts}"})) {
					info (1, "Rejected: ==> already planned $key");
					return 1;
				}
	 			$extcmd = "SCHEDULE_SVC_DOWNTIME;$h;$s;$data";
			}	
			add_cmd (\@$cmd,$h,$key2,$extcmd);
		}	
	}
	return 0;
}

# add command for planned downtime (unless already done)   0.13
sub add_cmd {
	my ($cmd,$h,$key2,$extcmd) = @_;
	return if ($pDowntimes{"$h;$key2"});
	push @$cmd, $extcmd;
	$pDowntimes{"$h;$key2"} = $extcmd;
}

# print information line
sub info {
	my ($lvl,$line) = @_;
	select STDOUT;
	select STDERR if ($lvl);
	print "  "x$lvl."$lvl: $line\n" if ($lvl <= $debug);
}

# calculate dates of holidays within a given year
sub flexible_holidays {
	my ($year,$month,$day) = (Localtime($cTime))[0..2];
	my $dow = Day_of_Week ($year,12,24);
	my $dow2 = Day_of_Week ($year+1,12,24);

	$holiday{carnival_monday}        = date_easter (-48);
	$holiday{mardi_gras}             = date_easter (-47);
	$holiday{ash_wednesday}          = date_easter (-46);
	$holiday{maundy_thursday}        = date_easter (-3);
	$holiday{good_friday}            = date_easter (-2);
	$holiday{easter_sunday}          = date_easter (0);
	$holiday{easter_monday}          = date_easter (+1);
	$holiday{ascension_day}          = date_easter (+39);
	$holiday{whit_sunday}            = date_easter (+49);
	$holiday{whit_monday}            = date_easter (+50);
	$holiday{trinity}                = date_easter (+56);
	$holiday{corpus_christi}         = date_easter (+60);

	$holiday{twelfth_day}            = next_date ($year,1,6);
	$holiday{labour_day}             = next_date ($year,5,1);
	$holiday{assumption_day}         = next_date ($year,8,15);
	$holiday{penance_day}            = next_date ($year,11,22);
	$holiday{penance_day} -= $dow * 86400;
	if ($holiday{penance_day} < Mktime ($year,$month,1,0,0,0)) {
		$holiday{penance_day} = next_date ($year+1,11,22);
		$holiday{penance_day} -= $dow2 * 86400;
	}
	$holiday{christmas_day}          = next_date ($year,12,25);
	$holiday{boxing_day}             = next_date ($year,12,26);
	
	# translation for the german speaking people ;-)
	$holiday{heilige_drei_koenige}   = $holiday{twelfth_day};
	$holiday{rosenmontag}            = $holiday{carnival_monday};
	$holiday{faschingsdienstag}      = $holiday{mardi_gras};
	$holiday{aschermittwoch}         = $holiday{ash_wednesday};
	$holiday{gruendonnerstag}        = $holiday{maundy_thursday};
	$holiday{karfreitag}             = $holiday{good_friday};
	$holiday{ostersonntag}           = $holiday{easter_sunday};
	$holiday{ostermontag}            = $holiday{easter_monday};
	$holiday{chr_himmelfahrt}        = $holiday{ascension_day};
	$holiday{pfingstsonntag}         = $holiday{whit_sunday};
	$holiday{pfingstmontag}          = $holiday{whit_monday};
	$holiday{fronleichnam}           = $holiday{corpus_christi};
	$holiday{maria_himmelfahrt}      = $holiday{assumption_day};
	$holiday{trinitatis}             = $holiday{trinity};
	$holiday{buss_und_bettag}        = $holiday{penance_day};
	$holiday{tag_der_arbeit}         = $holiday{labour_day};
	$holiday{weihnachtstag_1}        = $holiday{christmas_day};
	$holiday{weihnachtstag_2}        = $holiday{boxing_day};
	
	# only lowercase keys
	my %tmp = ();
	foreach (keys %holiday) {
		$tmp{lc($_)} = $holiday{$_};
	}
	%holiday = %tmp;
}

sub print_special_dates {
	if ($debug > 1) {
		info (2, "Flexible dates");
		my %tmp = ();
		foreach (keys %holiday) {
			$tmp{$holiday{$_}} .= "; $_";
		}
		foreach (sort keys %tmp) {
			info (3, fmt_dt ($_,1) . ": " . substr($tmp{$_},2) . (exists $day_off{$_} ? " (day off)" : ""));
		}
	}
}

# set current time, possibly altered by environment variables
sub set_date_time {
	# current time
	$cTime = time ();
	my ($year,$mon,$mday,$hour,$min) = (Localtime($cTime))[0..4];
	# use other date if specified using environment variable FAKE_DATE
	if ($ENV{FAKE_DATE}) {
		($year,$mon,$mday) = $ENV{FAKE_DATE} =~ /(\d\d\d\d)(\d\d)(\d\d)/;
		my $dow = Day_of_Week ($year,$mon,$mday);
		my $txt = Day_of_Week_to_Text ($dow);
		info (2, "FAKE_DATE set to $txt $year.$mon.$mday");
	}	
	# use other time if specified using environment variable FAKE_TIME
	if ($ENV{FAKE_TIME}) {
		($hour,$min) = $ENV{FAKE_TIME} =~ /(\d\d)(\d\d)/;
		info (2, "FAKE_TIME set to $hour:$min:00");
	}	
	if ($timestamp) {
		if (length($timestamp) > 8) { # YYYYMMDDHHMI
			($year,$mon,$mday,$hour,$min) = $timestamp =~ /(\d\d\d\d)(\d\d)(\d\d)(\d\d)(\d\d)/;
			my $dow = Day_of_Week ($year,$mon,$mday);
			my $txt = Day_of_Week_to_Text ($dow);
		} elsif (length($timestamp) > 4) { # YYYYMMDD
			($year,$mon,$mday) = $timestamp =~ /(\d\d\d\d)(\d\d)(\d\d)/;
		} elsif (length($timestamp) == 4) { # HHMI
			($hour,$min) = $timestamp =~ /(\d\d)(\d\d)/;
		} else {
			print "Timestamp is incorrect";
			return 1;
		}
	}
	# calculate unix timestamp based on given data
	$cTime = Mktime($year,$mon,$mday,$hour,$min,0);
	my ($yday,$wday,$isdst) = (Localtime($cTime))[6..8];
	info (2, sprintf "--- curr date/time: %04d-%02d-%02d %02d:%02d, yday: %03d, weekday %d (%s), DST: %s",
		$year,$mon,$mday,$hour,$min, $yday, $wday, substr(Day_of_Week_to_Text($wday),0,3), ($isdst) ? "yes" : "no");
	return 0;
}

# the dates are calculated based on easter sunday
sub date_easter {
	my ($offset) = @_;
	my $delta = 0;
	my ($year) = (Localtime($cTime))[0];
	eval { $delta = $offset * 1 };
	if ($?) {
		print "invalid $offset\n";
		exit 99;	
	}
	my $tmp = Mktime (Add_Delta_Days (Easter_Sunday($year),$delta),0,0,0);
	# the date has already passed
	if ($cTime > $tmp) {
		eval { $delta = $offset * 1 };
		if ($?) {
			print "invalid $offset\n";
			exit 98;	
		}
		$tmp = Mktime (Add_Delta_Days (Easter_Sunday($year+1),$delta),0,0,0);
	}
	return $tmp;
}

# calculate timestamp for fixed holiday
sub next_date {
	my ($year,$month, $day) = @_;
	my $c_ts = Mktime ((Localtime($cTime))[0..1],1,0,0,0);
	my $ts =  Mktime ($year,$month,$day,0,0,0);
	$ts = Mktime ($year+1,$month,$day,0,0,0) if ($ts < $c_ts);
	return $ts;
}

# the dates are calculated based on three to five parameters:
# month (1-12), weekday (1=monday - 7=sunday), week within month (-1=last, 1-5)
# hour (0-23) and minute (0-59) are optional and set to zero unless defined
# 
sub date_in_week_of_month {
	my ($m,$wd,$w, $hh,$mi) = @_;
	my $year = (Localtime($cTime))[0];
	my $tmp = 0;
	if ($m > 12) {
		$year++;
		$m %= 12;
	}
	$hh = 0 unless defined ($hh);
	$mi = 0 unless defined ($mi);

	if ($w == 0) {
		$tmp = Mktime (Nth_Weekday_of_Month_Year($year,$m,$wd,1),$hh,$mi,0);
	} elsif ($w > 0) {
		$tmp = Mktime (Nth_Weekday_of_Month_Year($year,$m,$wd,$w),$hh,$mi,0);
	} else { # w < 0, calculate weekday of week from end of month
		if (Nth_Weekday_of_Month_Year($year,$m,$wd,6+$w)) {
			$tmp = Mktime (Nth_Weekday_of_Month_Year($year,$m,$wd,6+$w),$hh,$mi,0);
		} else {
			$tmp = Mktime (Nth_Weekday_of_Month_Year($year,$m,$wd,5+$w),$hh,$mi,0);
		}
	}
	return $tmp;
}

# v0.14 --->
# calculate workdays in a month (monday to friday excluding "days off")
# array contains entries with day of month as value
# this way array[n] returns the day of month of the n-th workday
# negative values are allowed (-1=last, -2=2nd-to-last, ...)
#
sub workdays_in_month {
	my ($month) = @_;
	my $year = (Localtime($cTime))[0];
	my $max = Days_in_Month ($year, $month);
	return if ((defined($workdays[0])) and ($workdays[0] eq "$month:"));
	@workdays = ("$month:");
	for my $day (1..$max) {
		$workdays[$day] = (Day_of_Week($year,$month,$day) >= 6 ? 0 : $day);
	}
	foreach my $key (sort keys %holiday) {
		my ($y,$m,$d) = (Localtime($holiday{$key}))[0..2];
		next unless (($year == $y) and ($month == $m));
		$workdays[$d] = 0 if ((Localtime($holiday{$key}))[7] >= 6);
		$workdays[$d] = 0 if (exists($day_off{$key}));
	}
	my $idx = $max;
	while ($idx > 0) {
		splice (@workdays,$idx,1) unless ($workdays[$idx]);
		$idx--;
	}
	info (3, "Workdays " . join(',',@workdays));
}
# v0.14 <---

# v0.14 --->
sub fmt_dt {
	my ($ts,$wd,$t) =  @_;
	my ($y,$m,$d,$hh,$mm) = (Localtime($ts))[0..4];
	my $w = Day_of_Week_Abbreviation((Localtime($ts))[7]);
	my $s = sprintf "%4d.%02d.%02d", $y, $m, $d;
	$s .= sprintf " %02d:%02d", $hh, $mm if ($t);
	return ($wd ? "$w $s" : $s);
}	
# v0.14 <---	

sub help {
	print <<EOD;

$script $version - Copyright $creator

This script schedules downtimes based on several files:
- icinga.cfg (used to get command_file, status_file and objects_cache_file)
- downtime.cfg (downtime definitions)
- holiday.cfg (local holiday definitions, if present)

Usage:
$script [options]
   -c | --config=s     Icinga main config
                       default: $cFile
   -s | --schedule=s   schedule definitions
                       default: $dFile
   -l | --local=s      local holiday definitions
                       default: $lFile
   -m | --max_ahead=s  plan max. days ahead (default = 2)
   -f | --forecast=s   forecast next schedules
   -e | --examine=s    examine period and show next schedule
                       specify date and time instead like in downtime_period
   -d | --debug=s      0|1|2|3 (default = 1)
   -t | --timestamp=s  specify deviating time/date
                       YYYYMMDDhhmi, YYYYMMDD, or hhmi
   -h | --help         display this help

Note: Enabled debugging, forecasting, and/or examine will prevent that schedules
      are sent to the command pipe (downtimes are only calculated)!

Setting environment variables influences the behaviour:
- FAKE_DATE (YYYYMMDD): date deviating from current date
- FAKE_TIME (HHMM)    : time deviating from current time
- DEBUG (0|1|2|3)     : disables/enables debugging information
  0 = no debugging / cmds are sent to external command pipe!
  Note: the command line option take precedence over the environment variable

For details on timeperiod definitions please take a look at the documentation
or https://dev.icinga.org/issues/1867.

EOD
}
