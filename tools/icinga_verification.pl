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

############################################################
######   Icinga Verification and Reporting Script     ######
######  by Frankstar / Team Quality Assurance & VM    ######
############################################################

use strict;
use warnings;
#FIXME
eval ("use DBI");
#eval { require DBI; DBI->import( LIST ); }; 
if ($@) {
	print STDERR "Perl module DBI not found\n";
	exit 1;
}
use Term::ANSIColor;
use Env qw (LANG PATH);
use Getopt::Long;
use File::Basename qw ( basename );

# sub stubs
sub get_key_from_ini ($$);
sub which(@);
sub slurp($);
sub get_distribution;
sub find_icinga_dir;
sub get_icinga_version;
sub get_ido2db_version;
sub get_error_from_log;

### preconfiguration ###
#Critical System Services
my $config_ref = {
    critical_services => {
        apache2 => { binaries => [ 'httpd', 'apache2', 'httpd2' ] },
        mysql => { binaries => [ 'mysqld' ] },
		postgresql => { binaries => [ 'postmaster' ] },
        icinga => { binaries => [ 'icinga' ] },
        ido2db => { binaries => [ 'ido2db' ] },
    },
	noncritical_services => {
		snmptt => { binaries => [ 'snmptt' ] },
		npcd => { binaries => [ 'npcd' ] },
	}   
};

################################
# Option parsing
################################

my ($verbose, $reporting, $sanitycheck, $issuereport, $help) = ''; 
my $result = GetOptions( 
					"verbose" => \$verbose, 
					"reporting" => \$reporting, 
					"sanitycheck" => \$sanitycheck, 
					"issuereport" => \$issuereport,
					"help" => \$help
					);
if ($help){
	usage();
	exit;
}

################################
# Script Config
################################
print <<EOF;

############################################################
######    Icinga Sanitycheck and Reporting Script     ######
######  by Frankstar / Team Quality Assurance & VM    ######
############################################################
EOF

#Check if we are on Windows
my $oscheck = $^O;
if ( $oscheck eq 'MSWin32' ) {
    print STDERR "We are on Windows, will quit now!";
    exit 1;
}

#Icinga Base Set
my $icinga_base = find_icinga_dir();

if (! $icinga_base ) {
    print STDERR "\nIcinga config dir not found.\nPlease enter your Icinga config dir: ";
    $icinga_base = <STDIN>;
    chomp($icinga_base);
    if (! -d $icinga_base) {
        print STDERR "Couldn't find icinga.cfg.";
        exit 1;
    }
}
#Icinga/Nagios Plugins Base Set
my $idomod_cfg = "$icinga_base/idomod.cfg";
my $icinga_cfg = "$icinga_base/icinga.cfg";
my $pnp4nagios_base = find_pnp4nagios_dir();

my $icingashell = (getpwnam ('icinga'))[8];
#### DATABASE BACKEND ####

#SQL Server Check
my $mysqlcheck = which('mysql');
my $psqlcheck = which('psql');

#ido2db.cfg SQL Server Parsing
my $ido2db_cfg = "$icinga_base/ido2db.cfg";
my $sqlservertype_cfg =  get_key_from_ini("$ido2db_cfg", 'db_servertype');

#ido2db Server Host Name
my $sqlserver_cfg =  get_key_from_ini("$ido2db_cfg", 'db_host');
#ido2db DB User
my $sqluser_cfg = get_key_from_ini("$ido2db_cfg", 'db_user');
#ido2db DB Name
my $sqldb_cfg = get_key_from_ini("$ido2db_cfg", 'db_name');
#ido2db Password
my $sqlpw_cfg = get_key_from_ini("$ido2db_cfg", 'db_pass');

my ($dbh_cfg,$dbh_web, $dbh_cfg_error, $icinga_dbversion, $sth, $sth1) = '';

#Icinga Web DB USER1
#FIXME
#parse options from
#/icinga-web/app/config/databases.xml
my $sqluser_web = "icinga_web";
my $sqlpw_web = "icinga_web";

if ($sqlservertype_cfg eq 'mysql') {

#Mysql Connection Testing
	if ( !$mysqlcheck ) {
		print "mysql service not found, check your ido2db.cfg or mysql Server\n";
	} else {

		print STDERR " Mysql Found! - Try to connect via ido2db.cfg\n";
	

		# ido2db.cfg Connection test
		$dbh_cfg = DBI->connect(
			"dbi:mysql:database=$sqldb_cfg; host=$sqlserver_cfg:mysql_server_prepare=1",
			"$sqluser_cfg",
			"$sqlpw_cfg",
			{   PrintError => 0,
				RaiseError => 0
			}
			)
			or $dbh_cfg_error =
			"ido2db.cfg - MySQL Connect Failed.";

		if ( !$dbh_cfg_error ) {
			print " ido2db.cfg Mysql Connection Test OK!\n";
			$dbh_cfg->disconnect();
		} else {        
			print color("red"), "ido2db.cfg - MySQL Connect FAILED. Start Config Script", color("reset");
			print "\n";
			print STDERR "\nValues in '< >' are default parameters! Confirm with [Enter]\n";
			print STDERR "\nEnter your MYSQL Server <localhost>: ";
			$sqlserver_cfg = <STDIN>;
			chomp($sqlserver_cfg);
			if ( !$sqlserver_cfg ) {
			$sqlserver_cfg = 'localhost';
			}

			print STDERR "Enter your MYSQL User <root>: ";
			$sqluser_cfg = <STDIN>;
			chomp($sqluser_cfg);
			if ( !$sqluser_cfg ) {
			$sqluser_cfg = 'root';
			}
	
			print STDERR "Enter your Icinga Database <icinga>: ";
			$sqldb_cfg = <STDIN>;
			chomp($sqldb_cfg);
			if ( !$sqldb_cfg ) {
			$sqldb_cfg = 'icinga';
			}

			system( 'stty', '-echo' );
			print STDERR "Enter your MYSQL Password: ";
			$sqlpw_cfg = <STDIN>;
			chomp($sqlpw_cfg);
			system( 'stty', 'echo' );
			}
		}
} elsif ($sqlservertype_cfg eq 'psql') {

#Postgresql Connection Testing
	if ( !$psqlcheck) {
		print "postgresql not found, check your ido2db.cfg or PostgreSQL Server\n";
	} else {
		print STDERR " Postgresql found! - Try to connect via ido2db.cfg\n";
		#FIXME
		#PSQL CONNECTION TEST, Same as for Mysql
	}
}

################################
# Environment Checks, Reporting
################################

# Perl Version
my $perlversion = $^V;

# Kernel version
my $osversion = which('uname') ? qx(uname -rp) : 'uname binary not found';
chomp($osversion);

# PHP Version
my $phpversion = which('php') ? (qx(php -v))[0] : 'php binary not found';
chomp($phpversion);

#Current Time/Date
my $date = localtime();

#Apache Info
my $bin;
my $apacheinfo = join( '  ',
      ( $bin = which( @{ $config_ref->{'critical_services'}->{'apache2'}->{'binaries'} }) )
    ? (qx($bin -V))[ 0, 2, 3, 5 ]
    : 'apache binary not found' );

#Mysql Info
my $mysqlver =
    which('mysql')
    ? ( split( ",", qx(mysql -V) ) )[0]
    : 'mysql binary not found';

# distribution
my $distribution = (split( ",", get_distribution() ))[0];

# icinga version
my $icingaversion = get_icinga_version();

# ido2db version
my $ido2dbversion = get_ido2db_version();

# Selinux Check | selinuxenabled
my $selinux = which('selinuxenabled') ? qx(getenforce) : 'selinux binary not found';
chomp($selinux);

#log file test
#FIXME - PATH to syslog not hardcoded
#IDEA, read in /etc/rsyslog.conf
#get line with "*.info;mail.none;authpriv.none;cron.none" and save path to variable (centos ie. /var/log/messages

my @idolog = get_error_from_log("/var/log/messages", 'ido2db');

################################
# Icinga Checks / Reporting
################################

#check idomod.so/idomod.o

my $idomod_broker = get_key_from_ini("$icinga_base/icinga.cfg", 'broker_module');
my $idomod_o = which('idomod.o');
if (!$idomod_o){
	$idomod_o = "Couldn't find idomod.o";
}
my $idomod_so = which('idomod.so');
if (!$idomod_so){
	$idomod_so = "Couldn't find idomod.so";
}

#check if ido2db is running
my $ido2dbproc = qx( ps aux | grep [i]do2db | wc -l );
chomp($ido2dbproc);

#check idomod Connections
my $idocon = ($ido2dbproc - '1');

### icinga.cfg parsing ###
#icinga external commands
my $icingaextcmd = get_key_from_ini("$icinga_base/icinga.cfg", 'check_external_commands');
my $icingaextcmdlog = get_key_from_ini("$icinga_base/icinga.cfg", 'log_external_commands');

#icinga user
my $icingacfguser = get_key_from_ini("$icinga_base/icinga.cfg", 'icinga_user');
chomp($icingacfguser);

#icinga group
my $icingacfggroup = get_key_from_ini("$icinga_base/icinga.cfg", 'icinga_group');

### ido2db.cfg parsing ###
#ido2db SLA#
my $ido2dbsla = get_key_from_ini("$ido2db_cfg", 'enable_sla');
if(!$ido2dbsla){$ido2dbsla = "no 'enable_sla' option found"};

#ido2db socket type
my $ido2dbsocket = get_key_from_ini("$ido2db_cfg", 'socket_type');

#ido2db TCP Port
my $ido2dbtcpport = get_key_from_ini("$ido2db_cfg", 'tcp_port');

#ido2db SSL Status
my $ido2dbssl = get_key_from_ini("$ido2db_cfg", 'use_ssl');

#ido2db Servertype
my $ido2dbservertype = get_key_from_ini("$ido2db_cfg", 'db_servertype');

#ido2db Socket Name
my $ido2dbsocketname = get_key_from_ini("$ido2db_cfg", 'socket_name');

#### ido2db.cfg parsing ####

#Output Socket

my $idomodsocket = get_key_from_ini("$idomod_cfg", 'output_type');
if ($idomodsocket eq 'unixsocket'){
    $idomodsocket = 'unix';
}

#Output
my $idomodoutput = get_key_from_ini("$idomod_cfg", 'output');

#idomod SSL Status
my $idomodssl = get_key_from_ini("$idomod_cfg", 'use_ssl');

#idomod TCP port
my $idomodtcpport = get_key_from_ini("$idomod_cfg", 'tcp_port');

#### resource.cfg / check user1 for correct Plugin Path####
my $resource_cfg = get_key_from_ini("$icinga_cfg", 'resource_file');
my $plugin_path = '';
my $raw_plugin_path = get_key_from_ini("$resource_cfg", '\$USER1\$');
chomp($raw_plugin_path);
#only show path if the plugin check_ping was found
if ($raw_plugin_path){
	$plugin_path = $raw_plugin_path if -e "$raw_plugin_path/check_ping";
} if (!$plugin_path){
	$plugin_path = "\$USER1\$ is no Path or an incorrect Path";
}

#Check_disk / Check for free disk space AND check Plugin test
my $check_disk = (split(";", qx(su $icingacfguser -c '$plugin_path/check_disk -c 5%')))[0];

#### MySQL Querys ####
my $dbh_conn_error = '';
my @result_icingadb  = ();
my @row;
my @result_icingaconninfo = ();
my @result_icingawebdb  = ();

if ( !$mysqlcheck ) {
    print STDERR "no Mysql Found, skip queries\n";
} else {
    # Connect to Database
    $dbh_cfg = DBI->connect(
        "dbi:mysql:database=$sqldb_cfg; host=$sqlserver_cfg:mysql_server_prepare=1",
        "$sqluser_cfg",
        "$sqlpw_cfg",
        {   PrintError => 0,
            RaiseError => 0
        }
        )
        or $dbh_conn_error = 
		"MySQL Connect to Icinga-DB Failed. - Check your input or the MySQL process!";
		
	if(!$dbh_conn_error){
		# Query icinga DB Version
		$icinga_dbversion = 'SELECT version FROM icinga_dbversion';
		$sth = $dbh_cfg->prepare($icinga_dbversion) or warn $DBI::errstr;

		$sth->execute() or warn $DBI::errstr;

		while ( @row = $sth->fetchrow_array() ) {
			push( @result_icingadb, @row );
		}

		# Query icinga_conninfo
		my $icinga_conninfo = 'select conninfo_id, last_checkin_time from icinga_conninfo order by connect_time desc limit 2';
		$sth = $dbh_cfg->prepare($icinga_conninfo) or warn $DBI::errstr;

		$sth->execute() or warn $DBI::errstr;

		while ( @row = $sth->fetchrow_array() ) {
			push( @result_icingaconninfo, "id:", @row, "\n" );
		}
		
    $dbh_cfg->disconnect();
	
#ICINGA-WEB Connection
	$dbh_web = DBI->connect(
        "dbi:mysql:database=icinga_web; host=$sqlserver_cfg:mysql_server_prepare=1",
        "$sqluser_web",
        "$sqlpw_web",
        {   PrintError => 0,
            RaiseError => 0
        }
        )
        or $dbh_conn_error = 
		"MySQL Connect to Icinga-Web DB Failed. - Check your input or the MySQL process!";
	
	} else {
		print color("red"), "\n\n$dbh_conn_error\n\n", color("reset");
	}  
	
# Query icinga_web db version
		my $icingaweb_dbversion = 'select version, modified from nsm_db_version';
		$sth = $dbh_web->prepare($icingaweb_dbversion);
		eval {
		$sth->execute();
		};
		if ($@) {
			print 
			"\nFailure! Cant Fetch Table 'modified' from nsm_db_version,\nMaybe your Icinga-Web Database Shema is below 1.7.0\n\n";
		} else {
			$sth->execute() or warn $DBI::errstr;
		
			while ( @row = $sth->fetchrow_array() ) {
				push( @result_icingawebdb, @row );
			}
		}	
	$dbh_web->disconnect();
}

###########################
# Output Verbose Reporting
###########################

if ($reporting or (!$reporting and not ($sanitycheck or $issuereport))){

print <<EOF;
############################################################
######              Verbose Informations              ######
############################################################
Perlversion: $perlversion
Current Date/Time on Server: $date

OS Information:
  $distribution
  Kernel Version: $osversion
  LC_LANG: $LANG
  Selinux Status: $selinux
  
Webserver Information:
  $apacheinfo
PHP Information: 
 $phpversion
 
MySQL Information:
 $mysqlver
 
Icinga General Information:
 DB-Version: $result_icingadb[0]
 icinga version: $icingaversion
 ido2db version: $ido2dbversion
 ido2db Processes: $ido2dbproc
 idomod Connections: $idocon
 ido2db last Connection Info:
 @result_icingaconninfo 
Icinga.cfg/resource.cfg Information:
 External Commands(1=on,0=off): $icingaextcmd
 Log External Commands(1=on,0=off): $icingaextcmdlog
 Icinga User: $icingacfguser
 Icinga Group: $icingacfggroup
 Icinga Shell: $icingashell
 Plugin Path: $plugin_path
 broker modul: $idomod_broker
 
Icinga Web:
 DB-Version: $result_icingawebdb[0]
 DB-last modified: $result_icingawebdb[1]
 
ido2db Information:
 Server Type: $ido2dbservertype
 SSL Status: $ido2dbssl
 Socket Type: $ido2dbsocket
 Socket Name: $ido2dbsocketname
 TCP Port: $ido2dbtcpport
 SLA Status(1=on,0=off): $ido2dbsla
 
idomod Information:
 idomod.o check: $idomod_o
 idomod.so check: $idomod_so
 Output Type: $idomodsocket
 Output: $idomodoutput
 SSL Status: $idomodssl
 TCP Port: $idomodtcpport

ido2db Errors in Syslog: 
 @idolog

Plugin Check with User Rights:
(check_disk - Checks local HDD for free Space)
 $check_disk
 
############################################################ 
EOF
}

##########################
# Output Sanitycheck
##########################
# Color config
my $colorgreen = color('green');
my $colorred = color('red');
my $coloryellow = color('yellow');
my $colorreset = color("reset");
my $ok = "[OK  ]";
my $warn = "[WARN]";
my $crit = "[CRIT]";
my $statusok = "$colorgreen $ok $colorreset";
my $statuswarn = "$coloryellow $warn $colorreset";
my $statuscrit = "$colorred $crit $colorreset";

if ($sanitycheck){
print <<EOF;
############################################################
######                 Sanity Check                   ######
############################################################

Database Tests:
EOF
#Connection via ido2db.cfg
if (!$dbh_cfg_error){
	print $statusok,"Connection to DB via ido2db.cfg\n";
}
else{
	print $statuscrit,"$dbh_cfg_error\n";
}
# MYSQL User Input Error
if ($dbh_conn_error){
	print $statuscrit,"$dbh_conn_error\n";
}

print <<EOF;


ido2db/idomod Tests:
EOF
# ido2db -> idomod socket
if ($ido2dbsocket eq $idomodsocket){
	print $statusok,"ido2db/idomod Socket - same socket configured";
} else {
	print $statuscrit,"ido2db/idomod Sockets are different configured";
}
print <<EOF;


Config File Checks:
EOF
# checks for a defined root user
if ($icingacfguser eq 'root'){
	print $statuswarn, "icinga.cfg - icinga_user = $icingacfguser";
} else {
	print $statusok, "icinga.cfg - icinga_user = $icingacfguser";
}
print "\n";
# check resource.cfg for $user1$
if ($raw_plugin_path eq $plugin_path){
	print $statusok, "resource.cfg - Plugin Path: $plugin_path";
} else {		
	print $statuswarn, "resource.cfg - \$USER1\$ is no Path or an incorrect Path";
}
### Service Status ###
print <<EOF;


Icinga essential Services:
EOF
foreach my $service (keys(%{ $config_ref->{'critical_services'} })) {
    my $binary = which (@{ $config_ref->{'critical_services'}->{$service}->{'binaries'} });
    if (! $binary ) {
        print $statuswarn, "$service - no binary found.\n";
    } else {
        my $binary = basename($binary);
        my $status = qx(/bin/ps cax | /bin/grep $binary);
        if ( !$status ) {
            print $statuscrit, "$service - found but not running\n";
        } else {
            print $statusok, "$service - found and started\n";
        }
    }
}
print <<EOF;

non-critical Services:
EOF
foreach my $service (keys(%{ $config_ref->{'noncritical_services'} })) {
    my $binary = which (@{ $config_ref->{'noncritical_services'}->{$service}->{'binaries'} });
    if (! $binary ) {
        print $statuswarn, "$service - no binary found.\n";
    } else {
        my $binary = basename($binary);
        my $status = qx(/bin/ps cax | /bin/grep $binary);
        if ( !$status ) {
            print $statuscrit, "$service - found but not running\n";
        } else {
            print $statusok, "$service - found and started\n";
        }
    }
}

print <<EOF;

############################################################
EOF
}

#############################################
# Output Reporting with Issue Tracker Tags
#############################################

if ($issuereport){
print <<EOF;
############################################################
### Copy the following Output and paste it to your Issue ###
############################################################
*OS Information:*
  <pre>
  OS Name: $distribution,
  Kernel Version: $osversion
  LC_LANG: $LANG
  Selinux Status: $selinux
  </pre>
  
*Webserver Information:*
  <pre>
  Apache:
  $apacheinfo
  PHP Information:
  $phpversion
  
  MySQL Information:
  $mysqlver
  </pre>
 
*Icinga General Information:*
 <pre>
 Icinga DB-Version: $result_icingadb[0]
 icinga version: $icingaversion
 ido2db version: $ido2dbversion
 </pre>

EOF
}


exit;

#### SUBs ####
sub get_key_from_ini ($$) {
    my ( $file, $key ) = @_;

    if ( !-f $file ) {
        print STDERR "Inifile $file does not exist\n";
        return;
    }

    if ( open( my $fh, '<', $file ) ) {
        while ( my $line = <$fh> ) {
            chomp($line);
			$line =~ s/#.*//;
            if ( $line =~ /^\s*$key=([^\s]+)/ ) {
                return $1;
            }
        }
    } else {
        print STDERR "Could not open initfile $file: $!\n";
    }
	return "";
}

sub which (@) {
    my @binaries = @_;
    my @path = reverse( split( ':', $PATH ));
    push @path, "$icinga_base/../bin";
    push @path, "$icinga_base/../sbin";
	push @path, "$icinga_base/../lib";
    if ($pnp4nagios_base) {
        push @path, "$pnp4nagios_base/../bin";
        push @path, "$pnp4nagios_base/../sbin";
    }
    print "looking for binaries in ", join(",", @path), "\n" if $verbose;

    foreach my $binary (@binaries) {
        map { -x "$_/$binary" && return "$_/$binary" }@path;
    }
    return undef;
}

sub slurp($) {
    my $file = shift;
    if ( -f $file ) {
        open( my $fh, '<', $file )
            or die "Could not open $file: $!";
        return do { local $/; <$fh> };
    } else {
        die "$file does not exist";
    }
}

sub get_distribution {

    #first try: lsb
    if ( which('lsb_release') ) {
        open( my $fh, '-|', "lsb_release -d -c -r " );
        my $version = do { local $/; <$fh> };
        close($fh);
        $version = join( ", ", split( "\n", $version ) );
        $version =~ s/\s+/ /g, return $version;
    } elsif ( -f '/etc/debian_version' ) {
        my $version = slurp('/etc/debian_version');
        chomp($version);
        return "Debian GNU/Linux $version";
    } elsif ( -f '/etc/redhat-release' ) {
        my $version = slurp('/etc/redhat-release');
        chomp($version);
        return $version;
    } else {
        return "unknown";
    }
}

sub find_icinga_dir {
    my @locations = qw ( /etc/icinga/ /opt/icinga/etc/ /usr/local/icinga/etc/ );
    foreach my $location (@locations) {
        return $location if -e "$location/icinga.cfg";
    }
    return undef;
}

sub find_pnp4nagios_dir {
    my @locations = qw ( /etc/pnp4nagios/ /opt/pnp4nagios/etc/ /usr/local/pnp4nagios/etc/ );
    foreach my $location (@locations) {
        return $location if -e "$location/pnp4nagios_release";
    }
    return undef;
}

sub get_icinga_version {
    if (which('icinga')) {
		open(my $fh, '-|', which('icinga') . " --help");
        while (my $line = <$fh>) {
            if ($line =~ /^Icinga (.*)/) {
                return $1;
            }
        }
        close($fh);
    } else {
        return 'icinga binary not found in PATH';
    }
}

sub get_ido2db_version {
    if (which('ido2db')) {
        open(my $fh, '-|', which('ido2db') . " --help");
        while (my $line = <$fh>) {
            if ($line =~ /^IDO2DB (.*)/) {
                return $1;
            }
        }
        close($fh);
    } else {
        return 'ido2db binary not found in PATH';
    }
}

sub get_error_from_log ($$) {
    my ( $file, $key ) = @_;

    if ( !-f $file ) {
        print STDERR "logfile $file does not exist\n";
        return;
    }

    if ( open( my $fh, '<', $file ) ) {
        while ( my $line = <$fh> ) {
            chomp($line);		
            if ( $line =~ /\s+$key: Error: (.*)/) {
				print "\nFound error log in:","\n$file for key '$key':","\n$1 ", "\n" if $verbose;
                return $1;
				
            }
        }
    } else {
        print STDERR "Could not open logfile $file: $!\n";
    }
}
  
sub usage{
print <<EOF;

icinga_verification -r|--reporting=[Shows the Verbose Reporting Output]
                    -s|--sanitycheck=[Shows the Sanity Checks]
                    -i|--issuereport=[Shows a Issue Tracker prepared Output]
		    no option=[Shows only the Verbose Reporting Output]

This script will check certain settings/entries of your OS environ-
ment and Icinga Config to assist you in finding problems when you 
are using Icinga.

Sanity Check States:

[OK  ] ok message.
[WARN] warning message, might effect the operation of Icinga
[CRIT] error message: Icinga will not work without resolving the problem(s)

EOF
}