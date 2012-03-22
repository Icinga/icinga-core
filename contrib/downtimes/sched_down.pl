#!/usr/bin/perl -w

#*****************************************************************************
#
# sched_down.pl - Schedule recurring downtimes for Icinga
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
# 2011.11.18:  0.01 Initial version 

use strict;
use Getopt::Long qw(:config no_ignore_case bundling);

BEGIN {
	eval("use Date::Calc qw(:all)");
	unless (1) {
		print <<EOT;
Perl module Date::Calc is missing
Please install using "cpan Date::Calc" or via your package manager
EOT
		exit 3;
	}
}

#
# There is no need to change anything below this line
#

my $creator = "2011 Icinga Team";
my $version = "0.01";
my $script  = "sched_down.pl";

my $cFile = "/usr/local/icinga/etc/icinga.cfg";
my $dFile = "/usr/local/icinga/etc/downtime.cfg";

my $max_ahead = 2;	# plan schedules max. days ahead 
my $debug = $ENV{DEBUG} || 1;
my $forecast = "";	# forecast schedule
my $help = 0;
my $sFile = "";		# status file
my $oFile = "";		# objects cache
my $cPipe = "";		# command pipe
my %cDowntimes = ();	# current downtimes
my %pDowntimes = ();	# planned downtimes
my %sDowntimes = ();	# schedule definitions
my %sObject = ();		# srv per host
my %dt = ();			# date/time info
my %hg = ();			# hosts per hostgroup
my %sg = ();			# hosts per servicegroup
my $tmp = "";
my %days = (sun => 0, mon => 1, tue => 2, wed => 3, thu => 4, fri => 5, sat => 6);
my %months = (jan => 1, feb => 2, mar => 3, apr => 4, may => 5, jun => 6,
					jul => 7, aug => 8, sep => 9, 'oct' => 10, nov => 11, dec => 12);

# process command line parameters
Getopt::Long::Configure('bundling');
GetOptions(
    "h|help"        => \$help,
    "c|config=s"    => \$cFile,
    "s|schedule=s"  => \$dFile,
    "d|debug=s"     => \$debug,
    "m|max_ahead=s" => \$max_ahead,
    "f|forecast=s"  => \$forecast,
);

if ($help) {
	help();
	exit 0;
}

if ($forecast) {
	init_entry();
	$tmp->{host_name} = "dummyhost";
	$tmp->{commecnt} = "forecast";
	$tmp->{period} = $forecast;
	$sDowntimes{"dummyhost;;forecast"} = $tmp;
} else {
	read_config_file ($cFile,1);
	read_status_file ($sFile,2);
	read_object_file ($oFile,3);
	read_downtimes ($dFile,4);
}
plan_downtimes();
exit 0;

#
# Subroutines
#

sub init_entry {
	$tmp = {
		object              => "",
		host_name           => "",
		service_description => "",
		hostgroups          => "",
		servicegroups       => "",
		start_time          => 0,
		end_time            => 0,
		triggered_by        => 0,
		period              => "",
		duration            => 0,
		fixed               => 1,
		author              => "",
		comment             => "",
		propagate           => 0,
		register            => 1,
	}
}

sub init_entry2 {
	$tmp = {
		object              => "",
		type                => "",
		alias               => "",
		members             => "",
	}
}

sub init_dt {
	%dt = (
		start_ts => -1,
		start_y  => -1,
		start_m  => -1,
		start_w  => -1,
		start_d  =>  0,
		start_hh => -1,
		start_mm => -1,
		end_ts   => -1,
		end_y    => -1,
		end_m    => -1,
		end_w    => -1,
		end_d    =>  0,
		end_hh   => -1,
		end_mm   => -1,
		every    => -1,
	);
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
	info (0,"status_file=$sFile");
	info (0,"object_cache_file=$oFile");
	info (0,"command_file=$cPipe");
}

#
# read status file to get info about downtimes
#
sub read_status_file {
	my ($iFile,$rc) = @_;
	init_entry;
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
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
			if ($tmp->{object} =~ /downtime/) {
				$cDowntimes {"$tmp->{host_name};$tmp->{service_description};$tmp->{comment};$tmp->{start_time};$tmp->{end_time}"} = $tmp;
			}
			if ($tmp->{object} =~ /hoststatus|servicestatus/) {
				$sObject {"$tmp->{host_name};$tmp->{service_description};$tmp->{comment}"}++;
			}
			init_entry;
		}
		my @f = split(/=/,$_);
		$tmp->{$f[0]} = $f[1] if (exists($tmp->{$f[0]}));
	}
	close (IFILE);
	info (2, "--- current entries from $iFile ---");
	for my $key (sort keys %cDowntimes) {
		info (2,"$key: $cDowntimes{$key}{host_name},$cDowntimes{$key}{service_description},$cDowntimes{$key}{comment}");
	}
}

#
# read status file to get info about hostgroups/servicegroups/services
#
sub read_object_file {
	my ($iFile,$rc) = @_;
	my $ok = 0;
	init_entry2;
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
	while (<IFILE>) {
		chomp;
		s/#.*//;
		s/^\s+//;
		s/\s+$//;
		next if (/^$/);
		if (/^define\s+(\S+)\s+{/) {
			$tmp->{object} = $1;
			$ok = 1 if ($1 =~ /hostgroup|service/);
			next;
		}
		next unless ($ok);
		if (/}/) {
			if ($tmp->{object} =~ /hostgroup/) {
				$hg {"$tmp->{name}"} = $tmp;
			}
			if ($tmp->{object} =~ /servicegroup/) {
				$sg {"$tmp->{name}"} = $tmp;
			}
# services per host
			if ($tmp->{object} =~ /service$/) {
				$sObject {"$tmp->{host_name}"} .= "$tmp->{service_description};"
			}
			init_entry2;
			next;
		}
		my ($o,$v) = /(.*?)\s+(.*)/;
		if ($o =~ /(.+)group_name/) {
			$tmp->{type} = $1;
			$tmp->{name} = $v;
		} elsif (/^\w+/) {
			$tmp->{$o} = $v;
		}
	}
	close (IFILE);
	info (2, "--- current entries from $iFile ---");
	for my $key (sort keys %hg) {
		info (2,"HG $key ($hg{$key}{alias}): $hg{$key}{members}");
	}
	for my $key (sort keys %sg) {
		info (2,"SG $key ($sg{$key}{alias}): $sg{$key}{members}");
	}
}

#
# read downtime definition file
#
sub read_downtimes {
	my ($iFile,$rc) = @_;
	my $ok = 0;
	init_entry;
	open (IFILE, "$iFile") || info (0,"Error during open of $iFile, RC=$!") && exit $rc;
	while (<IFILE>) {
		chomp;
		s/\s*#.*//;
		s/^\s+//;
		s/\s+$//;
		s/\s*;.*//;
		next if /^$/;
		if (/define\s+(\S+)\s+{/) {
			$tmp->{object} = $1;
			$ok = 1 if ($1 =~ /downtime/);
			next;
		}
		next unless ($ok);
		if (/}/) {
# skip definition unless "register 1" 
			if ($tmp->{register}) {
				$tmp->{host_name} = $tmp->{hostgroups} if ($tmp->{hostgroups});
				$tmp->{host_name} = $tmp->{servicegroups} if ($tmp->{servicegroups});
				$sDowntimes {"$tmp->{host_name};$tmp->{service_description};$tmp->{comment}"} = $tmp;
			}
			init_entry;
			$ok = 0;
			next;
		}
		my ($o,$v) = /(.*?)\s+(.*)/;
		if (/_period/) {
			$tmp->{period} .= "$v;";
		} elsif (/^\w+/) {
			$tmp->{$o} = $v;
		}
	}
	close (IFILE);
	info (2, "--- current entries from $iFile ---");
	for my $key (sort keys %sDowntimes) {
		info (2,"Schedule $key: $sDowntimes{$key}{duration},$sDowntimes{$key}{fixed}");
	}
}

#
# plan new downtimes, skip existing ones
# 
sub plan_downtimes {
# current time
	my $cTime = time();
	my ($year,$mon,$mday,$hour,$min,$sec,$yday,$wday,$isdst) = Localtime($cTime);
# use other date if specified using environment variable FAKE_DATE
	if ($ENV{FAKE_DATE}) {
		($year,$mon,$mday) = $ENV{FAKE_DATE} =~ /(\d\d\d\d)(\d\d)(\d\d)/;
		my $dow = Day_of_Week ($year,$mon,$mday);
		my $txt = Day_of_Week_to_Text ($dow);
		print "FAKE_DATE set to $txt $year.$mon.$mday\n";
	}	
# use other time if specified using environment variable FAKE_TIME
	if ($ENV{FAKE_TIME}) {
		($hour,$min) = $ENV{FAKE_TIME} =~ /(\d\d)(\d\d)/;
		print "FAKE_TIME set to $hour:$min:00\n";
	}	
# calculate unix timestamp based on given data
	$cTime = Mktime($year,$mon,$mday,$hour,$min,0);

# loop through downtime definitions
	for my $key (sort keys %sDowntimes) {
		my $hh1 = 0;
		my $mm1 = 0;
		my $hh2 = -1;
		my $mm2 = 0;
		my $cmd = "";
		my ($h,$s,$c) = split(/;/,$key);
		my $duration = $sDowntimes{$key}{duration};
		my $a = $sDowntimes{$key}{author};
		my @r = split(/;/,$sDowntimes{$key}{period});
		my $f = $sDowntimes{$key}{fixed};
		my $hg = $sDowntimes{$key}{hostgroups};
		my $sg = $sDowntimes{$key}{servicegroups};
		my $p = $sDowntimes{$key}{propagate};
		my $t = 0;
		my $diff = 0;
		my $z1 = 0;
		my $z2 = 0;
		my $t1 = 0;
		my $t2 = 0;
		info (1,"--- Planning $key: $sDowntimes{$key}{duration},$sDowntimes{$key}{period}");
		info (2,"Found object $key") if (exists($sObject{$key}));
# loop through date/time definitions
		for my $i (0..$#r) {
			init_dt;
			my ($year,$mon,$mday,$hour,$min,$sec,$yday,$wday,$isdst) = Localtime($cTime);
			my ($ds, $ts) = $r[$i] =~ /^(.*?)\s(\d+:.*)/;
			my @ts = split(",",$ts.",");
			info (2, "- DS:$ds / TS:$ts");
			if ($ds =~ m#(.*)/\s*(\d+)#) {
				($ds,$dt{every}) = ($1,$2);
			}
# start - end
			if ($ds =~ m#(\S.*)\s+-\s+(\S.*)#) {
				analyse_period ("start",$1);
				analyse_period ("end",$2);
			}
			else {
# set end to start if no end definitions present
				analyse_period ("start",$1);
				if ($dt{every} < 0) {
					$dt{end_y} = $dt{start_y};
					$dt{end_m} = $dt{start_m};
					$dt{end_w} = $dt{start_w};
					$dt{end_d} = $dt{start_d};
					$dt{end_hh} = $dt{start_hh};
					$dt{end_mm} = $dt{start_mm};
					$dt{end_ts} = $dt{start_ts};
				}
			}
				
# loop through time definitions
			for my $j (0..$#ts) {
				info (2, "Y/M/D/W Y/M/D/W E TS: $dt{start_y}/$dt{start_m}/$dt{start_d}/$dt{start_w} $dt{end_y}/$dt{end_m}/$dt{end_d}/$dt{end_w} $dt{every} $ts[$j]");
				if ($ts[$j] =~ /-/) {
					($dt{start_hh},$dt{start_mm},$dt{end_hh},$dt{end_mm}) = $ts[$j] =~ /(\d+):(\d+)\s*-\s*(\d+):(\d+)/;
				} else {
					($dt{start_hh},$dt{start_mm}) = $ts[$j] =~ /(\d+):(\d+)/;
				}
#				$dt{start_ts} = Mktime($year,$mon,$mday,$dt{start_hh},$dt{start_mm},0);
				$dt{start_ts} = 0;
				$dt{end_ts} = $dt{start_ts};
				my $offs = 0;
# start year specified
				if ($dt{start_y} > 0) {
					$dt{start_ts} = Mktime ($dt{start_y},$dt{start_m},$dt{start_d},$dt{start_hh},$dt{start_mm},0);
					info (2,"Y1: ".localtime($dt{start_ts}));
					info (2,"cTime1: ".$cTime." / ".$dt{start_ts}." ".$z1);
					$diff = $dt{start_ts} - $cTime;
					next if ($diff > 86400*$max_ahead);
# end year specified
					if ($dt{end_y} > 0) {
						my $midnight = ($dt{end_hh} == 24 ? 1 : 0);	# end time 24:00?
						$dt{end_hh} -= $midnight * 24;
						$dt{end_ts} = Mktime ($dt{end_y},$dt{end_m},$dt{end_d},$dt{end_hh},$dt{end_mm},0) + $midnight * 86400;
						info (2,"Y2: ".localtime($dt{end_ts}));
						info (2,"cTime2: ".$cTime." / ".$dt{end_ts}." ".$z2);
						next if ($cTime > $dt{end_ts});
					}
# loop until start timestamp is at least current timestamp
					while ($cTime > ($dt{start_ts} = Mktime ($dt{start_y},$dt{start_m},$dt{start_d},$dt{start_hh},$dt{start_mm},0))) {
						$dt{every} = 7 if (($dt{start_w} >= 0) and ($wday == $dt{start_w}));
						if ($dt{every} > 0) {
							($dt{start_y},$dt{start_m},$dt{start_d}) = Add_Delta_YMD ($dt{start_y},$dt{start_m},$dt{start_d},0,0,$dt{every});
							$dt{start_ts} = Mktime ($dt{start_y},$dt{start_m},$dt{start_d},$dt{start_hh},$dt{start_mm},0);
							info (2,"E: ".localtime($dt{start_ts}));
							next; 
						}
						$dt{every} = 1;
					}
				} 
# week day specified
				elsif ($dt{start_w} >= 0) {
					$dt{start_ts} = Mktime ($year,$mon,$mday,$dt{start_hh},$dt{start_mm},0);
					$dt{end_w} += 7 if ($dt{end_w} < $dt{start_w});
					$offs = ($dt{start_w} < $wday + $offs ? $dt{start_w} + 7 - $wday : $dt{start_w} - $wday - $offs); 
					if ($dt{start_d} > 0) {
						my ($y,$m,$d) = Nth_Weekday_of_Month_Year ($year, $mon, $dt{start_w}, $dt{start_d});
						if ($dt{start_m} > 0) {
							($y,$m,$d) = Nth_Weekday_of_Month_Year ($year, $dt{start_m}, $dt{start_w}, $dt{start_d});
						}
						my $doy = Day_of_Year ($y,$m,$d);
						info (2, "N1: $offs / $doy");
						if ($yday > $doy) {
							($y,$m,$d) = Add_Delta_YMD ($y,$m,$d,1,0,0);
							($y,$m,$d) = Nth_Weekday_of_Month_Year ($y, $m, $dt{start_w}, $dt{start_d});
							$doy = Day_of_Year ($y,$m,$d);
						}
						info (2, "N2: $offs / $doy");
						if ($doy - $yday > $max_ahead) {
							info (1,"Rejected: ==> more than $max_ahead days ahead");
							next;
						}
						$dt{start_ts} = Mktime ($y,$m,$d,$dt{start_hh},$dt{start_mm},0);
						info (2,"N3: ".localtime($dt{start_ts}));
					}
# day not specified
					elsif ($dt{start_d} < 0) {
						$mon = $dt{start_m} if ($dt{start_m} > 0);
						my ($y,$m,$d) = Nth_Weekday_of_Month_Year ($year,$mon,$dt{start_w},5);
						($y,$m,$d) = Nth_Weekday_of_Month_Year ($year,$mon,$dt{start_w},5+(defined $d)+$dt{start_d});
						$dt{start_ts} = Mktime ($year,$m,$d,$dt{start_hh},$dt{start_mm},0);
						info (2,"N4: ".localtime($dt{start_ts}));
					}	
					else {
						$dt{start_ts} += $offs * 86400;
						info (2,"N5: ".localtime($dt{start_ts}));
						$dt{every} = 1;
					}
				}
# month specified
				elsif ($dt{start_m} > 0) {
					$offs = $dt{start_d};
					if ($dt{start_d} < 0) {
						$offs = Days_in_Month($year,$dt{start_m})+1+$dt{start_d};
					}
					while ($cTime > ($dt{start_ts} = Mktime ($year,$dt{start_m},$offs,$dt{start_hh},$dt{start_mm},0))) {
						info (2,"M0: ".localtime($dt{start_ts}));
						($year,$dt{start_m},$offs) = Add_Delta_YMD ($year,$dt{start_m},$offs,1,0,0);
						if ($dt{start_d} < 0) {
							$offs = Days_in_Month($year,$dt{start_m})+1+$dt{start_d};
						}
					}
					$dt{start_ts} = Mktime ($year,$dt{start_m},$offs,$dt{start_hh},$dt{start_mm},0);
					info (2,"M.: ".localtime($dt{start_ts}));
					my $doy = Day_of_Year ($year,$dt{start_m},$offs);
					info (2, "M1: $offs / $doy");
					if ($yday > $doy) {
						$dt{start_ts} = Mktime ($year,$dt{start_m},$offs,$dt{start_hh},$dt{start_mm},0);
						$doy = Day_of_Year ($year,$dt{start_m},$offs);
					}
					info (2, "M2: $offs / $doy");
					if ($yday > $doy) {
						info (1,"Rejected: date already passed");
						next;
					}
					if ($doy - $yday > $max_ahead) {
						info (1,"Rejected: ==> more than $max_ahead days ahead");
						next;
					}
#					$dt{start_ts} += 86400 * $offs;
					info (2,"M3: ".localtime($dt{start_ts}));
				}
				elsif ($dt{start_d} != 0) {
					$dt{every} = 1 if ($dt{every} == -1);
					if ($dt{start_d} < 0) {
						$dt{start_d} = Days_in_Month($year,$dt{start_m})+1+$dt{start_d};
					}	
					while ($cTime > ($dt{start_ts} = Mktime ($year,$mon,$dt{start_d},$dt{start_hh},$dt{start_mm},0))) {
						info (2,"D1: ".localtime($dt{start_ts}));
						if ($dt{start_d} != $dt{end_d}) {
							($year,$mon,$dt{start_d}) = Add_Delta_YMD ($year,$mon,$dt{start_d},0,0,$dt{every});
						} else {
							($year,$mon,$dt{start_d}) = Add_Delta_YMD ($year,$mon,$dt{start_d},0,1,0);
						}
					}
					info (2,"D2: ".localtime($dt{start_ts}));
					$diff = ($dt{start_ts} - $cTime) / 86400;
					if ($diff > $max_ahead) {
						info (1,"Rejected: ==> more than $max_ahead days ahead");
						next;
					}
				}
				elsif ($dt{start_d} < 0) {
				}		
				else {
					print "???\n";
				}
				info (2,"T1: ".localtime($dt{start_ts}));
				$diff = $dt{start_ts} - $cTime;
				next if ($diff > 86400*$max_ahead);
				if ($dt{end_hh} >= 0) {
#					$offs++ if ($dt{end_hh}*60+$dt{end_mm} < $dt{start_hh}*60+$dt{start_mm});
					$diff = ($dt{end_hh}*60+$dt{end_mm} - $dt{start_hh}*60-$dt{start_mm}) * 60;
					$dt{end_ts} = $dt{start_ts} + $diff;
					$dt{end_ts} += 86400 if ($diff < 0);
#					$dt{end_ts} = Mktime($year,$mon,$mday,$dt{end_hh},$dt{end_mm},0) + $offs * 86400;
					info (2,"from ".localtime($dt{start_ts})." until ".localtime($dt{end_ts}));
				}
				if (($dt{end_hh} < 0) and $duration) {
					$dt{end_ts} = $dt{start_ts} + $duration * 60;
#					print "$key: day $dt{start_w} (offs $offs) $dt{start_ts} ".localtime($dt{start_ts})." for $duration mins\n";
				}
				if (exists($cDowntimes{"$key;$dt{start_ts};$dt{end_ts}"})) {
					info (1,"Rejected: ==> already planned $key:".localtime($dt{start_ts})." to ".localtime($dt{end_ts})." for $duration mins");
					next;
				}
				$f = 0 if ($dt{end_ts} - $dt{start_ts} != $duration * 60);
				my $data = "$dt{start_ts};$dt{end_ts};$f;$t;$duration;$a;$c";
				info (1, "planned: $key: ".localtime($dt{start_ts})." to ".localtime($dt{end_ts})." for $duration mins");
				if ("$h;$s" =~ /;$/) {	# no service_description
					if ($hg) {	# host_groups defined
						my @member = split (/,/,$hg{$hg}->{members});
						my $comment = $sDowntimes{$key}{comment};
						my $cnt = 0;
						for my $idx (0..$#member) {
							$cnt++ if (exists($cDowntimes{"$member[$idx];;$comment;$dt{start_ts};$dt{end_ts}"}));
						}
						if ($cnt) {
							info (1,"Rejected: ==> already planned HG $key:".localtime($dt{start_ts})." to ".localtime($dt{end_ts})." for $duration mins");
							next;
						}
						$cmd = "SCHEDULE_HOSTGROUP_HOST_DOWNTIME;$hg;$data";
						$pDowntimes{"$hg;;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_HOSTGROUP_HOST_DOWNTIME;$hg;$data";
					} elsif ($sg) {
						my @mem = split (/,/,$sg{$sg}->{members});
						my @member = ();
						for (my $idx = 0; $idx <= $#mem; $idx+=2) {
							push @member, $mem[$idx];
						}
						my $comment = $sDowntimes{$key}{comment};
						my $cnt = 0;
						for my $idx (0..$#member) {
							$cnt++ if (exists($cDowntimes{"$member[$idx];;$comment;$dt{start_ts};$dt{end_ts}"}));
						}
						if ($cnt) {
							info (1,"Rejected: ==> already planned SG $key:".localtime($dt{start_ts})." to ".localtime($dt{end_ts})." for $duration mins");
							next;
						}
						$cmd = "SCHEDULE_SERVICEGROUP_HOST_DOWNTIME;$sg;$data";
						$pDowntimes{"$sg;;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_SERVICEGROUP_HOST_DOWNTIME;$sg;$data";
					} else {			# host_name defined
						if ($p) {
					 		$cmd = "SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME;$h;$data";
					 		$pDowntimes{"$h;;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME;$h;$data";
						} else {
					 		$cmd = "SCHEDULE_HOST_DOWNTIME;$h;$data";
					 		$pDowntimes{"$h;;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_HOST_DOWNTIME;$h;$data";
						}
					}
				} else {					# service_description defined
					if ($hg) {			# hostgroups defined
						if ($s =~ /^all$/i) {
							my @member = split (/,/,$hg{$hg}->{members});
							my $comment = $sDowntimes{$key}{comment};
							my $cnt = 0;
							for my $idx (0..$#member) {
								my @srv = split (/;/,$sObject{$member[$idx]});
								for my $idy (0..$#srv) {
									$cnt++ if (exists($cDowntimes{"$member[$idx];$srv[$idy];$comment;$dt{start_ts};$dt{end_ts}"}));
								}
							}
							if ($cnt) {
								info (1,"Rejected: ==> already planned HGs $key:".localtime($dt{start_ts})." to ".localtime($dt{end_ts})." for $duration mins");
								next;
							}
							$cmd = "SCHEDULE_HOSTGROUP_SVC_DOWNTIME;$hg;$data";
							$pDowntimes{"$hg;;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_HOSTGROUP_SVC_DOWNTIME;$hg;$data";
						}
					} elsif ($sg) {	# servicegroups defined
						if ($s =~ /^all$/i) {
							my @member = split (/,/,$sg{$sg}->{members});
							my $comment = $sDowntimes{$key}{comment};
							my $cnt = 0;
							for (my $idx = 0; $idx <= $#member; $idx+=2) {
								$cnt++ if (exists($cDowntimes{"$member[$idx];$member[$idx+1];$comment;$dt{start_ts};$dt{end_ts}"}));
							}
							if ($cnt > $#member) {
								info (1,"Rejected: ==> already planned SGs $key:".localtime($dt{start_ts})." to ".localtime($dt{end_ts})." for $duration mins");
								next;
							}
							$cmd = "SCHEDULE_SERVICEGROUP_SVC_DOWNTIME;$hg;$data";
							$pDowntimes{"$sg;;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_SERVICEGROUP_SVC_DOWNTIME;$hg;$data";
						}
					} else {				# host_name defined
						if ($s =~ /^all$/i) {
							my @srv = split (/;/,$sObject{$h});
							my $comment = $sDowntimes{$key}{comment};
							my $cnt = 0;
							for my $idy (0..$#srv) {
								$cnt++ if (exists($cDowntimes{"$h;$srv[$idy];$comment;$dt{start_ts};$dt{end_ts}"}));
							}
							if ($cnt) {
								info (1,"Rejected: ==> already planned Hs $key:".localtime($dt{start_ts})." to ".localtime($dt{end_ts})." for $duration mins");
								next;
							}
				 			$cmd = "SCHEDULE_HOST_SVC_DOWNTIME;$h;$data";
				 			$pDowntimes{"$h;;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_HOST_SVC_DOWNTIME;$h;$data";
						} else {
				 			$cmd = "SCHEDULE_SVC_DOWNTIME;$h;$s;$data";
				 			$pDowntimes{"$h;$s;$dt{start_ts};$dt{end_ts}"} = "SCHEDULE_SVC_DOWNTIME;$h;$s;$data";
						}	
					}	
				}
				info (0,"CMD:$cmd");
			}
		}
	}
	my $old = "";
	my $new = "";
	if ($debug) {
		print "\n";
		info (1, "--- Debugging enabled so NO commands will be sent to $cPipe! ---");
	}
	if ($forecast) {
		print "\n";
		info (1, "--- Forecasting enabled so NO commands will be sent to $cPipe! ---");
	}
	for my $key (sort keys %pDowntimes) {
		($new) = $key =~ /^(.*?;.*?);/;
		if ($old ne $new) {
			info (1, "CMD: [$cTime] $pDowntimes{$key}");
			unless ($debug or $forecast) {
				open (CMD,">$cPipe") || info (0,"Error opening $cPipe, RC=$!") && exit 5;
				print CMD "[$cTime] $pDowntimes{$key}\n";
			close (CMD);
			}
	   }
		else {
			info (2, "o=n: [$cTime] $pDowntimes{$key}");
		}
		$old = $new;
	}
}	

# for examples and explanation see 
# http://docs.icinga.org/latest/objectdefinitions.html#objectdefinitions-timeperiod
sub analyse_period {
	my ($index, $ds) = @_;
	info (2,$ds);
	if ($ds =~ m#(\d+)-(\d+)-(\d+)#) { # exact date: 2011-10-31
		$dt{$index."_y"} = $1;
		$dt{$index."_m"} = $2;
		$dt{$index."_d"} = $3;
	}
	elsif ($ds =~ m#^day\s+(\S+)#) { # day: day 5; day -1
		$dt{$index."_d"} = $1;
	}
	elsif ($ds =~ m#(\S..).*?day(\s*\S.*)?#) { # weekday: friday
		$dt{$index."_w"} = $days{lc($1)};
		if (defined $2) {
			$ds =~ s/.*?day\s+//;
			if ($ds =~ m#(\S+?)\s+(\S..)?#) { # monday 2; tuesday -1; sunday 1 april
				$dt{$index."_d"} = $1;
				$dt{$index."_m"} = $months{lc($2)} if (defined $2);
			} else {
				$dt{$index."_d"} = $ds if (defined $ds);
			}
		}
	}
	elsif ($ds =~ m#(\S..).*?\s+(\S+)#) { # month: april 10
		$dt{$index."_m"} = $months{lc($1)};
		$dt{$index."_d"} = $2;
	}
}

sub info {
	my ($lvl,$line) = @_;
	print "  "x$lvl."$lvl: $line\n" if ($lvl <= $debug);
}

sub help {
	print <<EOD;

$script $version - Copyright $creator

This script schedules downtimes based on several files:
- icinga.cfg (used to get command_file, status_file and objects_cache_file)
- downtime.cfg (downtime definitions)

Usage:
$script [options]
   -c | --config=s     Icinga main config (/usr/local/icinga/etc/icinga.cfg) 
   -s | --schedule=s   schedule definitions (/usr/local/icinga/etc/downtime.cfg)
   -m | --max_ahead=s  plan max. days ahead (default = 2)
   -f | --forecast=s   forecast next schedule (no files are read)
   -d | --debug=s      0|1|2 (default = 1)
   -h | --help         display this help

Note: Enabled debugging and/or forecasting will prevent that schedules 
      are sent to the command pipe (downtimes are only calculated)!

Setting environment variables influences the behaviour:
- FAKE_DATE (YYYYMMDD): date deviating from current date
- FAKE_TIME (HHMM)    : time deviating from current time
- DEBUG (0|1|2)       : disables/enables debugging information
  0 = no debugging / cmds are sent to external command pipe!
  Note: the command line option take precedence over the environment variable

For details on timeperiod definition please take a look at
https://dev.icinga.org/issues/1867.

EOD
}
