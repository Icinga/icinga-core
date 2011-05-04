#!/bin/sh
#-- --------------------------------------------------------
#-- create_pgsqldb.sh
#-- DB definition for Postgresql
#--
#-- Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
#--
#-- current version: 2011-05-03 Thomas Dressler
#-- -- --------------------------------------------------------

#set -x
WD=`dirname $0`
cd $WD
WD=`pwd`
#prepare scripts in /tmp for running with user postgres
cp -r ../pgsql /tmp/.
chmod a+r /tmp/pgsql/*
chmod 777 /tmp/pgsql
(
cat <<'PGSCRIPT' 
#!/bin/sh
#set -x
cd `dirname $0`
#where to connect
#edit this!
DB=icinga
DBUSER=icinga
DBPASS=icinga

echo "Create icinga DB and User, Errors regarding non existing objects kann be ignored"
psql postgres >create_pgsqldb.log <<EOS1
-- \set ECHO all
 DROP DATABASE $DB;
 DROP USER $DBUSER;
 CREATE DATABASE $DB;
 CREATE USER $DBUSER WITH PASSWORD '$DBPASS';
 \q
EOS1
if [ $? == 0 ]; then
				createlang plpgsql icinga;
				echo "Create icinga objects..."
        PGPASSWORD=$DBPASS
        psql $DB -U $DBUSER >create_icinga_objects_pgsqldb.log 2>&1 <<EOS2
\set ECHO all
\set ON_ERROR_STOP 1
\echo "START"
select now();
\i pgsql.sql
select version as DBVersion from icinga_dbversion where name='idoutils';
\echo "END";
select now();
\q
EOS2
        if [ $? == 0 ]; then
                echo "Database ready"
        else
                echo "Database creation failed"
                exit 2
        fi
else
        echo "Error while creating Database/User"
        echo "Terminated"
        exit 1
fi
PGSCRIPT
)> /tmp/pgsql/pgsqldb.sh
chmod a+rx /tmp/pgsql/pgsqldb.sh

#run it
#set -x
su - postgres -c /tmp/pgsql/pgsqldb.sh
RET=$?
mv -f /tmp/pgsql/*.log .
echo "Dont forget to modify pg_hba.conf to trust icinga!(see icinga documentation)"
rm -rf /tmp/pgsql
echo "Logfiles:"
ls -l *pgsql*.log
exit $RET