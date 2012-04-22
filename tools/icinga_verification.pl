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

# sub stubs
sub get_key_from_ini ($$);
sub which(@);
sub slurp($);
sub get_distribution;
sub find_icinga_dir;

################################
# Option parsing
################################

my $mysqldb = "icinga";

my $result = GetOptions( "icingadb=s" => \$mysqldb );

################################
# Script Config
################################

#Check if we are on Windows
my $oscheck = $^O;
if ( $oscheck eq 'MSWin32' ) {
    print STDERR "We are on Windows, will quit now!";
    exit 1;
}

# MySQL Config if MySQL is used
my $mysqlcheck = which('mysql');
my ( $mysqlserver, $mysqluser, $mysqlpw ) = '';

if ( !$mysqlcheck ) {
    print "mysql not found, skipping\n";
} else {

    print STDERR "\nMysql Found! - start Config Script\n";
    print STDERR "Values in '< >' are default parameters! Confirm with [Enter]\n";
    print STDERR "\nEnter your MYSQL Server <localhost>: ";
    $mysqlserver = <STDIN>;
    chomp($mysqlserver);
    if ( !$mysqlserver ) {
        $mysqlserver = 'localhost';
    }

    print STDERR "Enter your MYSQL User <root>: ";
    $mysqluser = <STDIN>;
    chomp($mysqluser);
    if ( !$mysqluser ) {
        $mysqluser = 'root';
    }

    system( 'stty', '-echo' );
    print STDERR "Enter your MYSQL Password: ";
    $mysqlpw = <STDIN>;
    chomp($mysqlpw);
    system( 'stty', 'echo' );
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

################################
# Environment Checks
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
      ( $bin = which( 'httpd', 'apache2' ) )
    ? (qx($bin -V))[ 0, 2, 3, 5, 6, 7, 8 ]
    : 'apache binary not found' );

#Mysql Info
my $mysqlver =
    which('mysql')
    ? ( split( ",", qx(mysql -V) ) )[0]
    : 'mysql binary not found';


# distribution
my $distribution = get_distribution();
################################
# Icinga Checks
################################

#check if ido2db is running
my $idocheck = qx( ps aux | grep [i]do2db | wc -l );
chomp($idocheck);

# ido2db.cfg parsing
######## read in complete file and write needed values in an Array !##################

#ido2db socket type
my $ido2dbsocket = get_key_from_ini("$icinga_base/ido2db.cfg", 'socket_type');

#ido2db TCP Port
my $ido2dbtcpport = get_key_from_ini("$icinga_base/ido2db.cfg", 'tcp_port');

#ido2db SSL Status
#use_ssl=

#ido2db Servertype
#db_servertype=

#ido2db Server Host Name
my $mysqlserver_cfg =  get_key_from_ini("$icinga_base/ido2db.cfg", 'db_host');

#ido2db Server port
#db_port=

#ido2db Server Socket
#db_socket=

#ido2db DB User
my $mysqluser_cfg = get_key_from_ini("$icinga_base/ido2db.cfg", 'db_user');

#ido2db DB Name
my $mysqldb_cfg = get_key_from_ini("$icinga_base/ido2db.cfg", 'db_name');

#ido2db Password
my $mysqlpw_cfg = get_key_from_ini("$icinga_base/ido2db.cfg", 'db_pass');

# MySQL Checks#
my $dbh_user         = '';
my $dbh_user_error   = '';
my $dbh_cfg          = '';
my $dbh_cfg_error    = '';
my $icinga_dbversion = '';
my $sth_user         = '';
my $sth1_user        = '';
my @result_icingadb  = ();
my @row;
my @result_icingaconninfo = ();

if ( !$mysqlcheck ) {
    print STDERR "no Mysql Found, skip Querys\n";
} else {

    # User Input Connect
    $dbh_user = DBI->connect(
        "dbi:mysql:database=$mysqldb; host=$mysqlserver:mysql_server_prepare=1",
        "$mysqluser",
        "$mysqlpw",
        {   PrintError => 0,
            RaiseError => 0
        }
        )
        or die color("red"),
        "\nMySQL Connect Failed. - check your input or MySQL Process\n",
        color("reset");

    chomp($dbh_user_error);

    # Query icinga DB Version
    $icinga_dbversion = 'SELECT version FROM icinga_dbversion';
    $sth_user = $dbh_user->prepare($icinga_dbversion) or warn $DBI::errstr;

    $sth_user->execute() or warn $DBI::errstr;

    while ( @row = $sth_user->fetchrow_array() ) {
        push( @result_icingadb, @row );
    }

    # Query icinga_conninfo
    my $icinga_conninfo =
        'select conninfo_id, last_checkin_time from icinga_conninfo order by connect_time desc limit 2';
    $sth1_user = $dbh_user->prepare($icinga_conninfo) or warn $DBI::errstr;

    $sth1_user->execute() or warn $DBI::errstr;

    while ( @row = $sth1_user->fetchrow_array() ) {
        push( @result_icingaconninfo, "id:", @row, "\n" );
    }

    $dbh_user->disconnect();

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
        "ido2db.cfg - MySQL Connect Failed. - check your config";

    chomp($dbh_cfg_error);

    if ( !$dbh_cfg_error ) {
        $dbh_cfg->disconnect();
    }
}


# Test Print Out
# later create a fileout with the output

print <<EOF;
############################################################
######   Icinga Verification and Reporting Script     ######
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
PHP Information: $phpversion
MySQL Information:
 $mysqlver
Icinga Informations:
 idomod Connections: $idocheck
 Icinga DB-Version: $result_icingadb[0]
 ido2db last Connection Info:
 @result_icingaconninfo
 ido2db.cfg mysql Test Connection
EOF

print color("red"), "\n $dbh_cfg_error\n", color("reset");
print <<EOF;
#Check Services
Process Status:
EOF

my @services = ( 'httpd', 'mysqld', 'snmptt', 'icinga', 'ido2db' );
foreach my $service (@services) {
    my $status = `/bin/ps cax | /bin/grep $service`;
    if ( !$status ) {
        print color("red"), " [$service]", color("reset"),
            " not found or started\n";
    } else {
        print color("green"), " [$service]", color("reset"),
            " found and started\n";
    }
}
print " ############################################################\n";

exit;

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
    foreach my $binary (@binaries) {
        map { -x "$_/$binary" && return "$_/$binary" }
            reverse( split( ':', $PATH ) );
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
