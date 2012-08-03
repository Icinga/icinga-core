#!/usr/bin/perl
#
# Copyright (c) 2012 Icinga Development Team
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
######   Icinga verification and reporting script     ######
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
sub get_icinga_web_data;

### preconfiguration ###
#critical system services
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
# option parsing
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
# script config
################################
print <<EOF;

############################################################################################################
##############################   Icinga sanity check and reporting script     ##############################
##############################  by Frankstar / Team Quality Assurance & VM    ##############################
############################################################################################################
EOF

#check if we are on windows
my $oscheck = $^O;
if ( $oscheck eq 'MSWin32' ) {
    print STDERR "We are on windows, will quit now!";
    exit 1;
}

#icinga base set
my $icinga_base = find_icinga_dir();
if (! $icinga_base ) {
	print STDERR color("red"), "\nIcinga dir not found!", color("reset");
	print STDERR "\nPlease enter your Icinga dir\nExample '/usr/local/icinga': ";
    $icinga_base = <STDIN>;
    chomp($icinga_base);
	$icinga_base = "$icinga_base/etc/";
    if (! -e "$icinga_base/icinga.cfg") {
		print STDERR color("red"), "\nCouldn't find icinga.cfg in path $icinga_base!\nWill quit now.\n", color("reset");
        exit 1;
    }
}

#cinga web base set
my $icinga_web_base = find_icinga_web_dir();
if (! $icinga_web_base ) {
	print STDERR color("red"), "\nIcinga-Web dir not found.", color("reset");
    print STDERR "\nPlease enter your Icinga-Web dir.\nExample '/usr/local/icinga-web': ";
    $icinga_web_base = <STDIN>;
    chomp($icinga_web_base);
	$icinga_web_base = "$icinga_web_base/app/";
    if (! -e "$icinga_web_base/config.php") {
		print STDERR color("red"), "\nCouldn't find config.php in path $icinga_web_base!\nWill skip Icinga-Web database tests\n", color("reset");
    }
}

#Icinga/Nagios plugins base set
my $idomod_cfg = "$icinga_base/idomod.cfg";
my $icinga_cfg = "$icinga_base/icinga.cfg";
my $pnp4nagios_base = find_pnp4nagios_dir();


#### DATABASE BACKEND ####

#sql server check
my $mysqlcheck = which('mysql');
my $psqlcheck = which('psql');

#ido2db.cfg SQL server parsing
my $ido2db_cfg = "$icinga_base/ido2db.cfg";
my $sqlservertype_cfg =  get_key_from_ini("$ido2db_cfg", 'db_servertype');

#ido2db server host name
my $sqlserver_cfg =  get_key_from_ini("$ido2db_cfg", 'db_host');
#ido2db DB user
my $sqluser_cfg = get_key_from_ini("$ido2db_cfg", 'db_user');
#ido2db DB name
my $sqldb_cfg = get_key_from_ini("$ido2db_cfg", 'db_name');
#ido2db password
my $sqlpw_cfg = get_key_from_ini("$ido2db_cfg", 'db_pass');

my ($dbh_cfg,$dbh_web, $dbh_cfg_error, $icinga_dbversion, $sth, $sth1) = '';

#icinga web DB databases.xml parsing
my $databases_xml = "$icinga_web_base/config/databases.xml";
my @sqldata_web = get_icinga_web_data("$databases_xml", "$sqlservertype_cfg");
my $sqluser_web = $sqldata_web[0];
my $sqlpw_web = $sqldata_web[1];
my $sqlserver_web = $sqldata_web[2];
my $sqlport_web = $sqldata_web[3];
my $sqldb_web = $sqldata_web[4];

if ($sqlservertype_cfg eq 'mysql') {

#mysql connection testing
	if ( !$mysqlcheck ) {
		print "mysql service not found, check your ido2db.cfg or mysql server\n";
	} else {
		print STDERR "\nmysql Found! - try to connect via ido2db.cfg\n";
		# ido2db.cfg connection test
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
			print STDERR color("green"), "ido2db.cfg mysql connection test OK!\n", color("reset");
			$dbh_cfg->disconnect();
		} else {
			print color("red"), "ido2db.cfg - mysql connect FAILED. Start config script", color("reset");
			print "\n";
			print STDERR "\nValues in '< >' are default parameters! Confirm with [Enter]\n";
			print STDERR "\nEnter your mysql server <localhost>: ";
			$sqlserver_cfg = <STDIN>;
			chomp($sqlserver_cfg);
			if ( !$sqlserver_cfg ) {
			$sqlserver_cfg = 'localhost';
			}

			print STDERR "Enter your mysql user <root>: ";
			$sqluser_cfg = <STDIN>;
			chomp($sqluser_cfg);
			if ( !$sqluser_cfg ) {
			$sqluser_cfg = 'root';
			}

			print STDERR "Enter your icinga idoutils database <icinga>: ";
			$sqldb_cfg = <STDIN>;
			chomp($sqldb_cfg);
			if ( !$sqldb_cfg ) {
			$sqldb_cfg = 'icinga';
			}

			system( 'stty', '-echo' );
			print STDERR "Enter your mysql password: ";
			$sqlpw_cfg = <STDIN>;
			chomp($sqlpw_cfg);
			system( 'stty', 'echo' );
			}
		}
} elsif ($sqlservertype_cfg eq 'psql') {

#postgresql connection testing
	if ( !$psqlcheck) {
		print "postgresql not found, check your ido2db.cfg or postgresql server\n";
	} else {
		print STDERR " postgresql found! - try to connect via ido2db.cfg\n";
		#FIXME
		#PSQL CONNECTION TEST, Same as for Mysql
	}
}

################################
# environment checks, reporting
################################

# perl version
my $perlversion = $^V;

# kernel version
my $osversion = which('uname') ? qx(uname -rp) : 'uname binary not found';
chomp($osversion);

# php version
my $phpversion = which('php') ? (qx(php -v))[0] : 'php binary not found';
chomp($phpversion);

# current date/time
my $date = localtime();

# apache info
my $bin;
my $apacheinfo = join( '  ',
      ( $bin = which( @{ $config_ref->{'critical_services'}->{'apache2'}->{'binaries'} }) )
    ? (qx($bin -V))[ 0, 2, 3, 5 ]
    : 'apache binary not found' );

# mysql info
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

# selinux check | selinuxenabled
my $selinux = which('selinuxenabled') ? qx(getenforce) : 'selinux binary not found';
chomp($selinux);

# log file test
#FIXME - PATH to syslog not hardcoded
#FIXME - check permissions (require su, or try sudo)
#IDEA, read in /etc/rsyslog.conf
#get line with "*.info;mail.none;authpriv.none;cron.none" and save path to variable (centos ie. /var/log/messages

my @idolog = get_error_from_log("/var/log/messages", 'ido2db');

################################
# icinga checks / reporting
################################

# check idomod.so/idomod.o

#FIXME - get idomod.so path from modules config
my $idomod_broker = get_key_from_ini("$icinga_base/icinga.cfg", 'broker_module');
if (!$idomod_broker){
	$idomod_broker = "no broker_module defined in icinga.cfg";
}
my $idomod_o = which('idomod.o');
if (!$idomod_o){
	$idomod_o = "Couldn't find idomod.o";
}
my $idomod_so = which('idomod.so');
if (!$idomod_so){
	$idomod_so = "Couldn't find idomod.so";
}

# check if ido2db is running
my $ido2dbproc = qx( ps aux | grep [i]do2db | wc -l );
chomp($ido2dbproc);

# check idomod Connections
my $idocon = ($ido2dbproc - '1');

### icinga.cfg parsing ###
# icinga external commands
my $icingaextcmd = get_key_from_ini("$icinga_base/icinga.cfg", 'check_external_commands');
my $icingaextcmdlog = get_key_from_ini("$icinga_base/icinga.cfg", 'log_external_commands');

# icinga user
my $icingacfguser = get_key_from_ini("$icinga_base/icinga.cfg", 'icinga_user');
chomp($icingacfguser);

# icinga group
my $icingacfggroup = get_key_from_ini("$icinga_base/icinga.cfg", 'icinga_group');

### ido2db.cfg parsing ###
# ido2db SLA#
my $ido2dbsla = get_key_from_ini("$ido2db_cfg", 'enable_sla');
if(!$ido2dbsla){$ido2dbsla = "no 'enable_sla' option found"};

# ido2db socket type
my $ido2dbsocket = get_key_from_ini("$ido2db_cfg", 'socket_type');

# ido2db TCP Port
my $ido2dbtcpport = get_key_from_ini("$ido2db_cfg", 'tcp_port');

# ido2db SSL Status
my $ido2dbssl = get_key_from_ini("$ido2db_cfg", 'use_ssl');

# ido2db Servertype
my $ido2dbservertype = get_key_from_ini("$ido2db_cfg", 'db_servertype');

# ido2db Socket Name
my $ido2dbsocketname = get_key_from_ini("$ido2db_cfg", 'socket_name');

#### ido2db.cfg parsing ####

# output socket

my $idomodsocket = get_key_from_ini("$idomod_cfg", 'output_type');
if ($idomodsocket eq 'unixsocket'){
    $idomodsocket = 'unix';
}
if ($idomodsocket eq 'tcpsocket'){
    $idomodsocket = 'tcp';
}
# output
my $idomodoutput = get_key_from_ini("$idomod_cfg", 'output');

# idomod SSL Status
my $idomodssl = get_key_from_ini("$idomod_cfg", 'use_ssl');

# idomod TCP port
my $idomodtcpport = get_key_from_ini("$idomod_cfg", 'tcp_port');

# check for idoutils Broker Modul#
my $brokermodulpath = "$icinga_base/modules";
my $brokermodul_idoutils = '';
if (! -e "$brokermodulpath/idoutils.cfg") {
	$brokermodul_idoutils = "No idoutils.cfg in $brokermodulpath";
} else {
	$brokermodul_idoutils = "idoutils.cfg in $brokermodulpath aktiv.";
}
#### resource.cfg / check user1 for correct Plugin Path####
my $resource_cfg = get_key_from_ini("$icinga_cfg", 'resource_file');
if (! -e "$resource_cfg") {
	print STDERR color("red"), "\nCouldn't find resource.cfg!\nPath definded in icinga.cfg: $resource_cfg\n", color("reset");
}
my $plugin_path = '';
my $raw_plugin_path = get_key_from_ini("$resource_cfg", '\$USER1\$');
chomp($raw_plugin_path);
# only show path if the plugin check_ping was found
if ($raw_plugin_path){
	$plugin_path = $raw_plugin_path if -e "$raw_plugin_path/check_ping";
} if (!$plugin_path){
	$plugin_path = "\$USER1\$ is no Path or an incorrect Path";
}

# check_disk / check for free disk space AND check plugin test
my $check_disk = (split(";", qx(su $icingacfguser -c '$plugin_path/check_disk -c 5%')))[0];

#### mysql queries ####
my $dbh_conn_error = '';
my @result_icingadb  = ();
my @row;
my @result_icingaconninfo = ();
my @result_icingawebdb  = ();

if ( !$mysqlcheck ) {
    print STDERR "no mysql found, skip queries\n";
} else {
    # connect to database
    $dbh_cfg = DBI->connect(
        "dbi:mysql:database=$sqldb_cfg; host=$sqlserver_cfg:mysql_server_prepare=1",
        "$sqluser_cfg",
        "$sqlpw_cfg",
        {   PrintError => 0,
            RaiseError => 0
        }
        )
        or $dbh_conn_error =
		"Mysql connect to Icinga database failed - Check credentials or mysql process!";

	if(!$dbh_conn_error){
		# query icinga DB version
		$icinga_dbversion = 'SELECT version FROM icinga_dbversion';
		$sth = $dbh_cfg->prepare($icinga_dbversion) or warn $DBI::errstr;

		$sth->execute() or warn $DBI::errstr;

		while ( @row = $sth->fetchrow_array() ) {
			push( @result_icingadb, @row );
		}

		# query icinga_conninfo
		my $icinga_conninfo = 'select conninfo_id, last_checkin_time from icinga_conninfo order by connect_time desc limit 2';
		$sth = $dbh_cfg->prepare($icinga_conninfo) or warn $DBI::errstr;

		$sth->execute() or warn $DBI::errstr;

		while ( @row = $sth->fetchrow_array() ) {
			push( @result_icingaconninfo, "id:", @row, "\n" );
		}

    $dbh_cfg->disconnect();

# icinga web connection
	$dbh_web = DBI->connect(
        "dbi:mysql:database=$sqldb_web; host=$sqlserver_web:mysql_server_prepare=1",
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

# query icinga_web db version
		my $icingaweb_dbversion = 'select version, modified from nsm_db_version';
		eval {
		$sth = $dbh_web->prepare($icingaweb_dbversion);
		};
		if ($@) {
			print
			"\nFailure! Cant Connect to Icinga-Web Database\n\n";
		} else {
		$sth = $dbh_web->prepare($icingaweb_dbversion);

			eval {
			$sth->execute();
			};
			if ($@) {
				print STDERR color("red"), "\nFailure! Cant Fetch Table 'version, modified' from nsm_db_version,\nMaybe your Icinga-Web Database Shema is below 1.7.0\n\n", color("reset");
			} else {
				$sth->execute();

				while ( @row = $sth->fetchrow_array() ) {
					push( @result_icingawebdb, @row );
				}
			$dbh_web->disconnect();
			}
		}
}


## other checks ##
my $usershell = (getpwnam ($icingacfguser))[8];

###########################
# output verbose reporting
###########################

if ($reporting or (!$reporting and not ($sanitycheck or $issuereport))){

print <<EOF;
############################################################################################################
############################             Verbose Information              ##################################
############################################################################################################
perl version: $perlversion
current date/time on server: $date

OS information:
 $distribution
 Kernel version: $osversion
 LC_LANG: $LANG
 Selinux status: $selinux

Webserver information:
 $apacheinfo
PHP nformation:
 $phpversion

MySQL Information:
 $mysqlver

Icinga general information:
 idoutils DB version: $result_icingadb[0]
 icinga version: $icingaversion
 ido2db version: $ido2dbversion
 ido2db processes: $ido2dbproc
 idomod connections: $idocon
 ido2db last connection info:
 @result_icingaconninfo
Icinga.cfg/resource.cfg information:
 external commands(1=on,0=off): $icingaextcmd
 log external commands(1=on,0=off): $icingaextcmdlog
 icinga user: $icingacfguser
 icinga group: $icingacfggroup
 user shell: $usershell
 plugin path: $plugin_path
 broker modul cfg: $idomod_broker
 broker modul dir: $brokermodul_idoutils

Icinga Web:
 DB version: $result_icingawebdb[0]
 DB last modified: $result_icingawebdb[1]

ido2db information:
 Server Type: $ido2dbservertype
 SSL Status: $ido2dbssl
 Socket Type: $ido2dbsocket
 Socket Name: $ido2dbsocketname
 TCP Port: $ido2dbtcpport
 SLA Status(1=on,0=off): $ido2dbsla

idomod information:
 idomod.o check: $idomod_o
 idomod.so check: $idomod_so
 output type: $idomodsocket
 output: $idomodoutput
 ssl status: $idomodssl
 tcp port: $idomodtcpport

ido2db errors in syslog:
 @idolog

plugin check with user permissions:
(check_disk - checks local HDD for free space)
 $check_disk

############################################################################################################
EOF
}

##########################
# Output sanity check
##########################
# color config
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
############################################################################################################
###############################                Sanity check                   ##############################
############################################################################################################

Database tests:
EOF
#Connection via ido2db.cfg
if (!$dbh_cfg_error){
	print $statusok,"Connection to database via ido2db.cfg\n";
}
else{
	print $statuscrit,"$dbh_cfg_error\n";
}
# MYSQL User Input Error
if ($dbh_conn_error){
	print $statuscrit,"$dbh_conn_error\n";
}

print <<EOF;

ido2db/idomod tests:
EOF
# ido2db -> idomod socket
if ($ido2dbsocket eq $idomodsocket){
	print $statusok,"ido2db/idomod socket - same socket configured";
} else {
	print $statuscrit,"ido2db/idomod sockets are configured differently";
}
print <<EOF;


config file checks:
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
	print $statusok, "resource.cfg - plugin path: $plugin_path";
} else {
	print $statuswarn, "resource.cfg - \$USER1\$ is not a valid path or an incorrect path";
}
### Service Status ###
print <<EOF;


Icinga essential services:
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

non-critical services:
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

############################################################################################################
EOF
}

#############################################
# Output reporting with issue tracker tags
#############################################

if ($issuereport){
print <<EOF;
############################################################################################################
#####################    Copy the following output and paste it to your issue     ##########################
############################################################################################################
*OS information:*
  <pre>
  OS name: $distribution,
  Kernel version: $osversion
  LC_LANG: $LANG
  Selinux status: $selinux
  </pre>

*Webserver information:*
  <pre>
  Apache:
  $apacheinfo
  PHP information:
  $phpversion

  MySQL information:
  $mysqlver
  </pre>

*Icinga general information:*
 <pre>
 idoutils db Version: $result_icingadb[0]
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
        print STDERR "\nInifile $file does not exist\n";
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
        print STDERR "Could not open inifile $file: $!\n";
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
    my @locations = qw ( /etc/icinga /opt/icinga/etc /usr/local/icinga/etc );
    foreach my $location (@locations) {
        return $location if -e "$location/icinga.cfg";
    }
    return undef;
}

sub find_icinga_web_dir {
    my @locations = qw ( /opt/icinga-web/app /usr/local/icinga-web/app );
    foreach my $location (@locations) {
        return $location if -e "$location/config.php";
    }
    return undef;
}

sub find_pnp4nagios_dir {
    my @locations = qw ( /etc/pnp4nagios /opt/pnp4nagios/etc /usr/local/pnp4nagios/etc );
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

sub get_icinga_web_data ($$) {
    my ( $file, $key ) = @_;

    if ( !-f $file ) {
        print STDERR "\n$file does not exist\n\n";
        return;
    }

    if ( open( my $fh, '<', $file ) ) {
        while ( my $line = <$fh> ) {
            chomp($line);
				$line =~ s/#.*//;
				if ( $line =~ /\s*"dsn">+$key:\/\/([^:]+):([^@]+)@([^:]+):([^\/]+)\/([^<]+)/ ) {
				return $1, $2, $3, $4, $5;
            }
        }
    } else {
        print STDERR "Could not open logfile $file: $!\n";
    }
}

sub usage{
print <<EOF;

icinga_verification -r|--reporting=[Shows the verbose reporting output]
                    -s|--sanitycheck=[Shows the sanity checks]
                    -i|--issuereport=[Shows a issue tracker prepared output]
		    no option=[Shows only the verbose reporting output]

This script will check certain settings/entries of your OS environ-
ment and Icinga config to assist you in finding problems when you
are using Icinga.

Sanity check states:

[OK  ] ok message.
[WARN] warning message, might effect the operation of Icinga
[CRIT] error message: Icinga will not work without resolving the problem(s)

EOF
}
