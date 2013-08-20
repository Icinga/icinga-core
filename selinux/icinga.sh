#!/bin/sh -e

DIRNAME=`dirname $0`
cd $DIRNAME
USAGE="$0 [ --update ]"
if [ `id -u` != 0 ]; then
echo 'You must be root to run this script'
exit 1
fi

if [ $# -eq 1 ]; then
	if [ "$1" = "--update" ] ; then
		time=`ls -l --time-style="+%x %X" icinga.te | awk '{ printf "%s %s", $6, $7 }'`
		rules=`ausearch --start $time -m avc --raw -se icinga`
		if [ x"$rules" != "x" ] ; then
			echo "Found avc's to update policy with"
			echo -e "$rules" | audit2allow -R
			echo "Do you want these changes added to policy [y/n]?"
			read ANS
			if [ "$ANS" = "y" -o "$ANS" = "Y" ] ; then
				echo "Updating policy"
				echo -e "$rules" | audit2allow -R >> icinga.te
				# Fall though and rebuild policy
			else
				exit 0
			fi
		else
			echo "No new avcs found"
			exit 0
		fi
	else
		echo -e $USAGE
		exit 1
	fi
elif [ $# -ge 2 ] ; then
	echo -e $USAGE
	exit 1
fi

echo "Building and Loading Policy"
set -x
make -f /usr/share/selinux/devel/Makefile icinga.pp || exit
/usr/sbin/semodule -i icinga.pp

# Fixing the file context on /usr/bin/icinga
/sbin/restorecon -F -R -v /usr/bin/icinga
/sbin/restorecon -F -R -v /usr/bin/ido2db
# Fixing the file context on /etc/rc\.d/init\.d/icinga
/sbin/restorecon -F -R -v /etc/rc\.d/init\.d/icinga
/sbin/restorecon -F -R -v /etc/rc\.d/init\.d/ido2db
# Fixing the file context on /var/run/icinga.pid
#/sbin/restorecon -F -R -v /var/run/icinga.pid
# Fixing the file context on /var/log/icinga/gui/.htaccess
/sbin/restorecon -F -R -v /var/log/icinga/gui/.htaccess
# Fixing the file context on /var/log/icinga/gui/index.htm
/sbin/restorecon -F -R -v /var/log/icinga/gui/index.htm
# Fixing the file context on /var/spool/icinga/checkresults
/sbin/restorecon -F -R -v /var/spool/icinga/checkresults
# Fixing the file context on /var/spool/icinga
/sbin/restorecon -F -R -v /var/spool/icinga
# Fixing the file context on /var/log/icinga/gui
/sbin/restorecon -F -R -v /var/log/icinga/gui
# Fixing the file context on /var/log/icinga/archives
/sbin/restorecon -F -R -v /var/log/icinga/archives
# Fixing the file context on /var/log/icinga
/sbin/restorecon -F -R -v /var/log/icinga
# Fixing the file context on /var/spool/icinga/cmd
/sbin/restorecon -F -R -v /var/spool/icinga/cmd
/sbin/restorecon -F -R -v /usr/lib64/icinga/cgi
/sbin/restorecon -F -R -v /usr/share/icinga

###
/sbin/restorecon -F -R -v /var/log/icinga-web
/sbin/restorecon -F -R -v /var/cache/icinga-web
