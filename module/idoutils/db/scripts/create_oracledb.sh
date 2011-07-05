#!/bin/sh
#-- --------------------------------------------------------
#-- create_oracledb.sh
#-- DB definition for Oracle
#--
#-- Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
#--
#-- current version: 2011-06-10 Thomas Dressler
#-- -- --------------------------------------------------------

#where database to connect
#edit this!
DB=XE
DBUSER=icinga
DBPASS=icinga

#set -x
SP=`which sqlplus`
if [ ! -x "$SP" ]; then
	echo "cannot find oracle sqlplus executable, terminate.."
	echo "make sure your oracle environment is set properly before starting this!"
	exit 4
fi 
WD=`dirname $0`
cd $WD
WD=`pwd`
cd ../oracle


if [ ! -r icinga_defines.sql ]; then
	echo create default parameter file icinga_defines.sql
	echo "
/*
filesystems to use for distributing index and data. In low frequency environments
this can be the same. trailing slash is mandantory
EDIT THIS!
*/
DEFINE DATAFS=./
DEFINE IDXFS=./
DEFINE LOBFS=./
/*
icinga tablespaces and user must fit definitions in create_icinga_objects_oracle.sql
EDIT THIS!
*/

DEFINE DATATBS=ICINGA_DATA1
DEFINE IDXTBS=ICINGA_IDX1
DEFINE LOBTBS=ICINGA_LOB1
DEFINE ICINGA_USER=$DBUSER
DEFINE ICINGA_PASSWORD=$DBPASS
" >icinga_defines.sql

fi

#run sqlplus as sys
echo "Enter password for oracle user 'SYS' on $DB to drop and create new Tablespaces and user $DBUSER"
read SYSPASS
$SP /nolog <<EOS1
--exit if connect errornous
whenever sqlerror exit failure
connect sys/${SYSPASS}@${DB} as sysdba;
-- -----------------------------------------
-- run user and tablespace creation
-- CAUTION: THIS WILL DROP EXISTING USER AND TABLESPACES WITH SAME NAME
-- -----------------------------------------
@create_oracle_sys.sql
EOS1
RET=$?

if [ $RET == 0 ]; then
	 #create icinga schema objects using newly created user
   $SP /nolog <<EOS2
   --exit if connect errornous
   whenever sqlerror exit failure
connect ${DBUSER}/${DBPASS}@${DB}
-- -----------------------------------------
-- create icinga objects
-- CAUTION: THIS WILL DROP EXISTING USER AND TABLESPACE WITH SAME NAME
-- -----------------------------------------
@oracle.sql
EOS2
	RET=$?
	#check if dbversion entered(last insert)
	if [ $RET == 0 ]; then
		echo "Connecting now as $DBUSER on $DB and  check icinga schema version"
  	$SP /nolog <<EOS3
connect ${DBUSER}/${DBPASS}@${DB}
Alter session set nls_date_format='YYYY-MM-DD HH24:MI';
select 'DB-Version'||version from dbversion where name='idoutils';
select 'END' from dual;
select sysdate from dual;
exit;
EOS3

     	if [ $? == 0 ]; then
                echo "Database ready"
                 RET=0
     	else
                echo "Schema creation check failed"
                RET=3
  		fi
     
  else
                echo "Schema creation failed"
                RET=2
  fi
else
        echo "Error while running Oracle SYS part"
        echo "Terminated"
        RET=1
fi

mv -f *oracle*.log $WD/.
cd $WD
echo "Logfiles:"
ls -l *oracle*.log
exit $RET
