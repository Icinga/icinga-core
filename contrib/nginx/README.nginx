REQUIREMENTS:
=============

PHP 5.3
PHP-FPM 5.3 (PHP-APC Optional)
FASTCGI WRAPPER
NGINX 1.2 or newer

INSTALLATION
============
- Install all the required packages from your distro repository.
- Copy all the files to the configuration folder of nginx.
- Include the icinga-classic.vhost as a nginx virtual host.
- Restart nginx.


DEBIAN EXAMPLE
==============
Enable the dotdeb repository to get the latest version of PHP and Nginx (http://www.dotdeb.org)
# cat > /etc/apt/sources.list.d/dotdeb.list << EOF
deb http://packages.dotdeb.org squeeze all
deb-src http://packages.dotdeb.org squeeze all
EOF

Refresh the package list
# apt-get update

Upgrade the system
# apt-get upgrade

Install the required packages
# apt-get install nginx php5-fpm php-apc fcgiwrap

Configure to your taste the fpm and fcgiwrap config files (default values are almost safe)
# vi /etc/php5/fpm/php-fpm.conf
# vi /etc/php5/fpm/php.ini
# vi /etc/php5/fpm/pool.d/www.conf

Edit the FASTCGI WRAPPER init script, and change the user/group to your selected user:
# vi /etc/init.d/fcgiwrap
-FCGI_USER="www-data"
-FCGI_GROUP="www-data"
+FCGI_USER="icinga"
+FCGI_GROUP="icinga"


For more info check their docs.

Copy the config file into the sites-available folder for nginx
# cp icinga-classic.vhost nginx.icinga.conf nginx.pnp4nagios.conf nginx.security.conf /etc/nginx/sites-available

Modify the icinga-classic.vhost to include the path to the .conf files
# vi /etc/nginx/sites-available/icinga-classic.vhost
-    include nginx.security.conf;
+    include /etc/nginx/sites-available/nginx.security.conf;

-    include nginx.icinga.conf;
+    include /etc/nginx/sites-available/nginx.icinga.conf;

-    include nginx.pnp4nagios.conf;
+    include /etc/nginx/sites-available/nginx.pnp4nagios.conf;

Enable the virtual host for icinga classic
# ln -s /etc/nginx/sites-available/icinga-classic.vhost /etc/nginx/sites-enabled/icinga-classic.vhost

Change the user of nginx if needed
# vi /etc/nginx/nginx.conf

+ user=icinga;

Restart nginx
# /etc/init.d/nginx restart


Author
======
Francisco Miguel Biete <fmbiete at gmail dot com>
