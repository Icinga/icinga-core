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

# preconfiguration
my $config_ref = {
    services => {
        apache2 => { binaries => [ 'httpd', 'apache2' ] },
        mysql => { binaries => [ 'mysqld' ] },
        icinga => { binaries => [ 'icinga' ] },
        ido2db => { binaries => [ 'ido2db' ] },
		snmptt => { binaries => [ 'snmptt' ] },
    }
};

################################
# Option parsing
################################

my ($verbose, $reporting, $sanitycheck, $issuereport) = ''; 
my $result = GetOptions( "verbose" => \$verbose, "reporting" => \$reporting, "sanitycheck" => \$sanitycheck, "issuereport" => \$issuereport);

################################
# Script Config
################################

#Check if we are on Windows
my $oscheck = $^O;
if ( $oscheck eq 'MSWin32' ) {
    print STDERR "We are on Windows, will quit now!";
    exit 1;
}

#Icinga Base Set
my $icinga_base = find_icinga_dir();

if (! $icinga_base ) {
    print STDERR "\nIcinga base not found.\nPlease enter your Icinga base: ";
    $icinga_base = <STDIN>;
    chomp($icinga_base);
    if (! -d $icinga_base) {
        print STDERR "Couldn't find icinga.cfg.";
        exit 1;
    }
}

#### MySQL Config if MySQL is used ####

#ido2db.cfg Mysql Config parsing

#ido2db Server Host Name
my $mysqlserver_cfg =  get_key_from_ini("$icinga_base/ido2db.cfg", 'db_host');
#ido2db DB User
my $mysqluser_cfg = get_key_from_ini("$icinga_base/ido2db.cfg", 'db_user');
#ido2db DB Name
my $mysqldb_cfg = get_key_from_ini("$icinga_base/ido2db.cfg", 'db_name');
#ido2db Password
my $mysqlpw_cfg = get_key_from_ini("$icinga_base/ido2db.cfg", 'db_pass');

#Mysql Server Check, is a mysql serverrunning?
my $mysqlcheck = which('mysql');

#Mysql Connection Testing
my ($dbh_cfg, $dbh_cfg_error, $icinga_dbversion, $sth, $sth1, $mysqldb) = '';

if ( !$mysqlcheck ) {
    print "mysql not found, skipping\n";
} else {

    print STDERR "\nMysql Found! - Try to connect via ido2db.cfg\n";
	
	# ido2db.cfg Connection test
    $dbh_cfg = DBI->connect(
        "dbi:mysql:database=$mysqldb_cfg; host=$mysqlserver_cfg:mysql_server_prepare=1",
        "$mysqluser_cfg",
        "$mysqlpw_cfg",
        {   PrintError => 0,
            RaiseError => 0
        }
        )
        or $dbh_cfg_error =
        "ido2db.cfg - MySQL Connect Failed.";

    if ( !$dbh_cfg_error ) {
	    print "ido2db.cfg Mysql Connection Test OK!\n";
        $dbh_cfg->disconnect();
    } else {        
		print color("red"), "ido2db.cfg - MySQL Connect FAILED. Start Config Script", color("reset");
		print "\n";
		print STDERR "\nValues in '< >' are default parameters! Confirm with [Enter]\n";
		print STDERR "\nEnter your MYSQL Server <localhost>: ";
		$mysqlserver_cfg = <STDIN>;
		chomp($mysqlserver_cfg);
		if ( !$mysqlserver_cfg ) {
		$mysqlserver_cfg = 'localhost';
		}

		print STDERR "Enter your MYSQL User <root>: ";
		$mysqluser_cfg = <STDIN>;
		chomp($mysqluser_cfg);
		if ( !$mysqluser_cfg ) {
        $mysqluser_cfg = 'root';
		}
	
		print STDERR "Enter your Icinga Database <icinga>: ";
		$mysqldb_cfg = <STDIN>;
		chomp($mysqldb_cfg);
		if ( !$mysqldb_cfg ) {
        $mysqldb_cfg = 'icinga';
		}

		system( 'stty', '-echo' );
		print STDERR "Enter your MYSQL Password: ";
		$mysqlpw_cfg = <STDIN>;
		chomp($mysqlpw_cfg);
		system( 'stty', 'echo' );
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
      ( $bin = which( @{ $config_ref->{'services'}->{'apache2'}->{'binaries'} }) )
    ? (qx($bin -V))[ 0, 2, 3, 5, 6, 7, 8 ]
    : 'apache binary not found' );

#Mysql Info
my $mysqlver =
    which('mysql')
    ? ( split( ",", qx(mysql -V) ) )[0]
    : 'mysql binary not found';


# distribution
my $distribution = get_distribution();

# icinga version
my $icingaversion = get_icinga_version();

# ido2db version
my $ido2dbversion = get_ido2db_version();

# Selinux Check | selinuxenabled
my $selinux = which('selinuxenabled') ? qx(getenforce) : 'selinux binary not found';
chomp($selinux);

#log file test
#FIXME - PATH to syslog not hardcoded
my @idolog = get_error_from_log("/var/log/messages", 'ido2db');

################################
# Icinga Checks / Reporting
################################

#check if ido2db is running
my $idocheck = qx( ps aux | grep [i]do2db | wc -l );
chomp($idocheck);

#icinga.cfg parsing
#icinga external commands
my $icingaextcmd = get_key_from_ini("$icinga_base/icinga.cfg", 'check_external_commands');

#ido2db.cfg parsing
#ido2db socket type
my $ido2dbsocket = get_key_from_ini("$icinga_base/ido2db.cfg", 'socket_type');

#ido2db TCP Port
my $ido2dbtcpport = get_key_from_ini("$icinga_base/ido2db.cfg", 'tcp_port');

#ido2db SSL Status
my $ido2dbssl = get_key_from_ini("$icinga_base/ido2db.cfg", 'use_ssl');

#ido2db Servertype
my $ido2dbservertype = get_key_from_ini("$icinga_base/ido2db.cfg", 'db_servertype');

#ido2db Socket Name
my $ido2dbsocketname = get_key_from_ini("$icinga_base/ido2db.cfg", 'socket_name');


#### IDOMOD.cfg parsing ####

#Output Socket
my $idomodsocket = get_key_from_ini("$icinga_base/idomod.cfg", 'output_type');
if ($idomodsocket eq 'unixsocket'){
	$idomodsocket = 'unix';
}

#Output
my $idomodoutput = get_key_from_ini("$icinga_base/idomod.cfg", 'output');

#idomod SSL Status
my $idomodssl = get_key_from_ini("$icinga_base/idomod.cfg", 'use_ssl');

#idomod TCP port
my $idomodtcpport = get_key_from_ini("$icinga_base/idomod.cfg", 'tcp_port');

#### MySQL Querys ####
my $dbh_conn_error = '';
my @result_icingadb  = ();
my @row;
my @result_icingaconninfo = ();

if ( !$mysqlcheck ) {
    print STDERR "no Mysql Found, skip Querys\n";
} else {
    # Connect to Database
    $dbh_cfg = DBI->connect(
        "dbi:mysql:database=$mysqldb_cfg; host=$mysqlserver_cfg:mysql_server_prepare=1",
        "$mysqluser_cfg",
        "$mysqlpw_cfg",
        {   PrintError => 0,
            RaiseError => 0
        }
        )
        or $dbh_conn_error = 
		"MySQL Connect Failed. - Check your input or the MySQL Process!";
		
	if(!$dbh_conn_error){
		# Query icinga DB Version
		$icinga_dbversion = 'SELECT version FROM icinga_dbversion';
		$sth = $dbh_cfg->prepare($icinga_dbversion) or warn $DBI::errstr;

		$sth->execute() or warn $DBI::errstr;

		while ( @row = $sth->fetchrow_array() ) {
			push( @result_icingadb, @row );
		}

		# Query icinga_conninfo
		my $icinga_conninfo =
        'select conninfo_id, last_checkin_time from icinga_conninfo order by connect_time desc limit 2';
		$sth1 = $dbh_cfg->prepare($icinga_conninfo) or warn $DBI::errstr;

		$sth1->execute() or warn $DBI::errstr;

		while ( @row = $sth1->fetchrow_array() ) {
			push( @result_icingaconninfo, "id:", @row, "\n" );
		}

    $dbh_cfg->disconnect();
	} else {
		print color("red"), "\n\n$dbh_conn_error\n\n", color("reset");
	}   
}

# Output Reporting 
# later create a fileout with the output
if ($reporting or (!$reporting and not ($sanitycheck or $issuereport))){

print <<EOF;
############################################################
######           Icinga Reporting Script              ######
######  by Frankstar / Team Quality Assurance & VM    ######
############################################################
Perlversion: $perlversion
Current Date/Time on Server: $date

OS Information:
  OS Name: $distribution,
  Kernel Version: $osversion
  LC_LANG: $LANG
  
Webserver Information:
  $apacheinfo
PHP Information: 
 $phpversion
 
MySQL Information:
 $mysqlver

Selinux Status:
 $selinux 
 
Icinga General Informations:
 icinga version: $icingaversion
 ido2db version: $ido2dbversion
 idomod Connections: $idocheck
 Icinga DB-Version: $result_icingadb[0]
 ido2db last Connection Info:
 @result_icingaconninfo 
Icinga.cfg Information:
 External Commands(1=on,0=off): $icingaextcmd
 
ido2db Information:
 Server Type: $ido2dbservertype
 SSL Status: $ido2dbssl
 Socket Type: $ido2dbsocket
 Socket Name: $ido2dbsocketname
 TCP Port: $ido2dbtcpport
 
idomod Information:
 Output Type: $idomodsocket
 Output: $idomodoutput
 SSL Status: $idomodssl
 TCP Port: $idomodtcpport

ido2db Errors in Syslog: 
 @idolog

############################################################ 
EOF
}
# Output Sanity Check
if ($sanitycheck){
print <<EOF;
############################################################
######           Icinga Verification Script           ######
######  by Frankstar / Team Quality Assurance & VM    ######
############################################################

Mysql Connection with ido2db.cfg:
EOF
if (!$dbh_cfg_error){
	print color("green"), " Connection OK!", color("reset");	
}
else{
	print color("red"), " $dbh_cfg_error\n", color("reset");
}
# MYSQL User Input Error
print color("red"), " $dbh_conn_error\n", color("reset");
print "\n";

print <<EOF;
ido2db - idomod Socket:
EOF
# ido2db -> idomod socket
if ($ido2dbsocket eq $idomodsocket){
	print color("green"), " ido2db/idomod - listen/write to the same socket", color("reset");
} else {
	print color("red"), " ido2db/idomod sockets are different!", color("reset");
}

print <<EOF;


Process Status:
EOF
foreach my $service (keys(%{ $config_ref->{'services'} })) {
    my $binary = which (@{ $config_ref->{'services'}->{$service}->{'binaries'} });
    if (! $binary ) {
        print color("yellow"), " [$service]",color("reset"), " no binary found.\n";
    } else {
        my $binary = basename($binary);
        my $status = qx(/bin/ps cax | /bin/grep $binary);
        if ( !$status ) {
            print color("red"), " [$service]", color("reset"),
                " found but not running\n";
        } else {
            print color("green"), " [$service]", color("reset"),
                " found and started\n";
        }
    }
}
print <<EOF;

############################################################
EOF
}

# Output Reporting with Issue Tracker Tags
if ($issuereport){
print <<EOF;
############################################################
######  Icinga Reporting with Issue Tracker Tags      ######
######  by Frankstar / Team Quality Assurance & VM    ######
############################################################
*Perlversion:* $perlversion
*Current Date/Time on Server:* $date

*OS Information:*
  <pre>
  OS Name: $distribution,
  Kernel Version: $osversion
  LC_LANG: $LANG
  </pre>
  
*Webserver Information:*
  <pre>
  $apacheinfo
  </pre>
*PHP Information:*
 <pre>
 $phpversion
 </pre>
 
*MySQL Information:*
 <pre>
 $mysqlver
 </pre>

*Selinux Status:*
 <pre>
 $selinux
 </pre> 
 
*Icinga General Informations:*
 <pre>
 icinga version: $icingaversion
 ido2db version: $ido2dbversion
 idomod Connections: $idocheck
 Icinga DB-Version: $result_icingadb[0]
 ido2db last Connection Info:
 @result_icingaconninfo 
 </pre>
*Icinga.cfg Information:*
 <pre>
 External Commands(1=on,0=off): $icingaextcmd
 </pre>
 
*ido2db Information:*
 <pre>
 Server Type: $ido2dbservertype
 SSL Status: $ido2dbssl
 Socket Type: $ido2dbsocket
 Socket Name: $ido2dbsocketname
 TCP Port: $ido2dbtcpport
 </pre>
 
*idomod Information:*
 <pre>
 Output Type: $idomodsocket
 Output: $idomodoutput
 SSL Status: $idomodssl
 TCP Port: $idomodtcpport
 </pre>

*ido2db Errors in Syslog:*
 <pre>
 @idolog
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
            if ( $line =~ /^\s*$key=([^\s]+)/ ) {
                return $1;
            }
        }
    } else {
        print STDERR "Could not open initfile $file: $!\n";
    }
}

sub which (@) {
    my @binaries = @_;
    my @path = reverse( split( ':', $PATH ));
    push @path, "$icinga_base/../bin";
    push @path, "$icinga_base/../sbin";
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
            if ( $line =~ /\s+$key: (.*)/) {
				print "\nFound error log in:","\n$file for key '$key':","\n$1 ", "\n" if $verbose;
                return $1;
				
            }
        }
    } else {
        print STDERR "Could not open logfile $file: $!\n";
    }
}
