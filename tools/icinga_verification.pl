#!/usr/bin/perl
#
# Copyright (c) 2012 Icinga Developer Team 
# Holzer Franz / Team Quality Assurance & VM 
# http://www.icinga.org
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
############################################################";
######   Icinga Verification and Reporting Script     ######";
######  by Frankstar / Team Quality Assurance & VM    ######";
############################################################";

use strict;
use warnings;
use DBI;
use Term::ANSIColor;

################################
# Script Config
################################

#Check if we are on Windows

my $oscheck = $^O;
if( $oscheck eq 'MSWin32' ){
	print "We are on Windows, will quit now!";
	exit;
}

# MySQL Config if MySQL is used

my $mysqlcheck = `find /usr/bin -name mysql`;
my $mysqldb = '';
my $mysqlserver = '';
my $mysqluser = '';
my $mysqlpw = '';

if (!$mysqlcheck ){
	print "no Mysql Found, skip Config";
}
else{
$mysqldb = "icinga";
print "\nMysql Found! - start Config Script\n";
print "\nEnter your MYSQL Server <localhost>: ";
$mysqlserver = <STDIN>;
chomp($mysqlserver);

print "Enter your MYSQL User <root>: ";
$mysqluser = <STDIN>;
chomp($mysqluser);

system('stty','-echo');
print "Enter your MYSQL Password: ";
$mysqlpw = <STDIN>;
chomp($mysqlpw);
system('stty','echo');
}

if (!$mysqlserver || !$mysqluser){
	print "\n\nNo MYSQL Server or User specified - will exit now\n";
	exit;
}

################################
# Environment Checks 
################################

# Perl Version
my @perlversion = `perl -v`;

# Kernel version
my $osversion = `uname -rp` ;

# search for OS Information Files
my @files = `find /etc -maxdepth 1 -name *-release 2>/dev/null`;
my @distriinfo;

if (@files == 0) {
	print "no release info File found in /etc/";
	exit -1;
} else {
	@distriinfo = `cat $files[0]`;
}

# PHP Version
my @phpversion = `php -v`;

#environment Language
my $envlang = `env | grep LANG`;

#Current Time/Date
my $date = `date`;

#Apache Info
my @apacheinfo = `httpd -V`;
chomp(@apacheinfo);

######ADD JAVA HOMES, ORCALE HOMES, PATH -> via env | grep ######

################################
# Icinga Checks 
################################


# verify that idomod connected via socket to ido2db
my $idocheck = `ps aux | grep ido2db | grep -v grep | wc -l`;
chomp($idocheck);

# MySQL Connect
my $dbh = '';
my $icinga_dbversion = '';
my $sth = '';
my @result_icingadb = ();
my @row;
my @result_icingaconninfo = ();


if (!$mysqlcheck ){
	print "no Mysql Found, skip Querys";
}
else{
$dbh = DBI->connect("dbi:mysql:database=$mysqldb; host=$mysqlserver:mysql_server_prepare=1", "$mysqluser", "$mysqlpw")
	or die "\n$DBI::errstr";

# Query icinga DB Version
$icinga_dbversion = 'SELECT version FROM icinga_dbversion';
$sth = $dbh->prepare($icinga_dbversion) or die $DBI::errstr;

$sth->execute() or die $DBI::errstr;


while(@row = $sth->fetchrow_array()){
	push(@result_icingadb,@row);
}

# Query icinga_conninfo
my $icinga_conninfo = 'select conninfo_id, last_checkin_time from icinga_conninfo order by connect_time desc limit 2';
my $sth1 = $dbh->prepare($icinga_conninfo) or die $DBI::errstr;

$sth1->execute() or die $DBI::errstr;

while(@row = $sth1->fetchrow_array()){
	push(@result_icingaconninfo,"id:",@row,"\n");
}
# MySQL Disconnect
$dbh->disconnect();
}

# Test Print Out
print "\n ############################################################";
print "\n ######   Icinga Verification and Reporting Script     ######";
print "\n ######  by Frankstar / Team Quality Assurance & VM    ######";
print "\n ############################################################";
print "\n $perlversion[1]";
print " Current Date/Time on Server: $date";
print "\n OS Information:\n";
print " OS Name: @distriinfo";
print " Kernel Version: $osversion";
print " Environment-$envlang";
print "\n Webserver Information:\n";
print " $apacheinfo[0] \n $apacheinfo[2] \n $apacheinfo[3] \n $apacheinfo[5] \n $apacheinfo[6] \n $apacheinfo[7] \n $apacheinfo[8] \n";
print "\n PHP Information:\n $phpversion[0]";
print "\n Icinga Informations:\n";
print " idomod Connections: $idocheck\n";
print " Icinga DB-Version: $result_icingadb[0]\n";
print "\n ido2db last Connection Info:\n";
print " @result_icingaconninfo";

#Check Services
print "\n Process Status:\n";

my @services = ( 'blub', 'httpd', 'mysqld', 'snmptt', 'icinga', 'ido2db');
 
 foreach my $service (@services) {
 my $status = `/bin/ps cax | /bin/grep $service`;
	if (!$status) {
		print color("red"), " [$service]", color("reset"), " not found or started\n";
	}
	else{
		print color("green"), " [$service]", color("reset"), " found and started\n";
	}
 }
print " ############################################################\n";

exit;