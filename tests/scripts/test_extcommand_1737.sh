#!/bin/sh
#############################################################################################
# ICINGA TEST CONFIG SCRIPTS
# (c) 2009-2012 Icinga Development Team and Community Contributors
#
# #1737
# log error on non-existing host/service/contact/*group when sending a command to the core
#############################################################################################

now=`date +%s`
commandfile='/var/icinga/rw/icinga.cmd'

/usr/bin/printf "[%lu] ACKNOWLEDGE_HOST_PROBLEM;Host1;1;1;1;John Doe;Some comment\n" $now > $commandfile
