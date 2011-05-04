#!/bin/sh
#-- --------------------------------------------------------
#-- create_mysqldb.sh
#-- DB definition for MySQL
#--
#-- Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
#--
#-- current version: 2011-05-03 Thomas Dressler
#-- -- --------------------------------------------------------


#set -x
#where to connect
#edit this!
DB=icinga
DBUSER=icinga
DBPASS=icinga
DBHOST=localhost
DBADMIN=root

WD=`dirname $0`
cd $WD
WD=`pwd`
cd ../mysql

echo "Enter password for mysql user '$DBADMIN' or <enter> if none"
read ROOTPASS
if [ -s "ROOTPASS" ];then
	P=-p$ROOTPASS
fi
echo "drop existing DB $DB and user $DBUSER..."
mysql -u $DBADMIN -h $DBHOST $P  mysql <<EOS1
 DROP DATABASE IF EXISTS $DB;
 DROP USER '$DBUSER'@'$DBHOST' ;
 flush privileges;
 \q
EOS1

echo "create new DB $DB, user $DBUSER and objects..."
mysql -u $DBADMIN -h $DBHOST $P --verbose >$WD/create_mysqldb.log mysql <<EOS2
 CREATE DATABASE $DB;
 CREATE USER '$DBUSER'@'$DBHOST'  IDENTIFIED BY '$DBPASS';
 GRANT USAGE ON *.* TO '$DBUSER'@'$DBHOST' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0;
 GRANT SELECT , INSERT , UPDATE , DELETE ON $DB.* to '$DBUSER'@'$DBHOST';
 FLUSH PRIVILEGES ;
 use $DB;
 select "START Schema Script";
select now();
source mysql.sql
select "END Schema Script";
select now();
 \q
EOS2

if [ $? == 0 ]; then
				echo "Check icinga schema version with DB User $DBUSER..."
        mysql $DB -u $DBUSER -p$DBPASS -h $DBHOST -s <<EOS3
select "DB-Version",version from icinga_dbversion where name='idoutils';
\q
EOS3

        if [ $? == 0 ]; then
                echo "Database ready"
                RET=0
        else
                echo "Database creation failed"
                RET=2
        fi
else
        echo "Error while creating Database/User"
        RET=1
fi
echo "Logfiles:"
cd $WD
ls -l *mysql*.log
exit $RET
