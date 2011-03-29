# $Id$
# Authority: cmr
# Upstream: The icinga devel team <icinga-devel at lists.sourceforge.net>
# Needs libdbi
# ExcludeDist: el4 el3

%define logdir %{_localstatedir}/log/icinga

%define apacheconfdir  %{_sysconfdir}/httpd/conf.d
%define apacheuser apache

Summary: Open Source host, service and network monitoring program
Name: icinga
Version: 1.4.0
Release: 1%{?dist}
License: GPLv2+
Group: Applications/System
URL: http://www.icinga.org/

Source0: http://dl.sf.net/%{name}/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: gcc
BuildRequires: gd-devel > 1.8
BuildRequires: httpd
BuildRequires: zlib-devel
BuildRequires: libpng-devel
BuildRequires: libjpeg-devel
BuildRequires: libdbi-devel
BuildRequires: perl(ExtUtils::Embed)
Provides: nagios = %{version}

%description
Icinga is an application, system and network monitoring application.
It can escalate problems by email, pager or any other medium. It is
also useful for incident or SLA reporting.

Icinga is written in C and is designed as a background process,
intermittently running checks on various services that you specify.

The actual service checks are performed by separate plug-in programs
which return the status of the checks to Icinga.

Icinga is a fork of the nagios project.

%package gui
Summary: Web content for %{name}
Group: Applications/System
Requires: %{name} = %{version}-%{release}
Requires: httpd
Requires: %{name}-doc

%description gui
This package contains the webgui (html,css,cgi etc.) for %{name}

%package idoutils
Summary: database broker module for %{name}
Group: Applications/System
Requires: %{name} = %{version}-%{release}

%description idoutils
This package contains the idoutils broker module for %{name} which provides
database storage via libdbi.

%package api
Summary: PHP api for %{name}
Group: Applications/System
Requires: php

%description api
PHP api for %{name}

%package doc
Summary: documentation %{name}
Group: Applications/System
 
%description doc
Documentation for %{name}


%prep
%setup -q -n %{name}-%{version}

# /usr/local/nagios is hardcoded in many places
%{__perl} -pi.orig -e 's|/usr/local/nagios/var/rw|%{_localstatedir}/spool/nagios/rw|g;' contrib/eventhandlers/submit_check_result

%build
%configure \
    --datadir="%{_datadir}/%{name}" \
    --datarootdir="%{_datadir}/%{name}" \
    --libexecdir="%{_datadir}/%{name}" \
    --localstatedir="%{_localstatedir}/log/%{name}" \
    --with-checkresult-dir="%{_localstatedir}/spool/%{name}/checkresults" \
    --sbindir="%{_libdir}/%{name}/cgi" \
    --sysconfdir="%{_sysconfdir}/%{name}" \
    --with-cgiurl="/%{name}/cgi-bin" \
    --with-command-user="icinga" \
    --with-command-group="icingacmd" \
    --with-gd-lib="%{_libdir}" \
    --with-gd-inc="%{_includedir}" \
    --with-htmurl="/%{name}" \
    --with-init-dir="%{_initrddir}" \
    --with-lockfile="%{logdir}/%{name}.pid" \
    --with-mail="/bin/mail" \
    --with-icinga-user="icinga" \
    --with-icinga-group="icinga" \
    --with-template-objects \
    --with-template-extinfo \
    --enable-event-broker \
    --enable-embedded-perl \
    --enable-idoutils \
    --with-httpd-conf=%{apacheconfdir} \
    --with-init-dir=%{_initrddir}
%{__make} %{?_smp_mflags} all

%install
%{__rm} -rf %{buildroot}
%{__mkdir} -p %{buildroot}/%{apacheconfdir}
%{__make} install-unstripped \
    install-init \
    install-commandmode \
    install-config \
    install-webconf \
    install-idoutils \
    install-api \
    DESTDIR="%{buildroot}" \
    INSTALL_OPTS="" \
    INSTALL_OPTS_WEB="" \
    COMMAND_OPTS="" \
    INIT_OPTS=""

### FIX log-paths
%{__perl} -pi -e '
        s|log_file.*|log_file=%{logdir}/%{name}.log|;
        s|log_archive_path=.*|log_archive_path=%{logdir}/archives|;
        s|debug_file=.*|debug_file=%{logdir}/%{name}.debug|;
        s|command_file=.*|command_file=%{_localstatedir}/spool/%{name}/rw/%{name}.cmd|;
   ' %{buildroot}%{_sysconfdir}/%{name}/%{name}.cfg

### Move command file dir to spool dir
mv %{buildroot}%{_localstatedir}/log/%{name}/rw %{buildroot}%{_localstatedir}/spool/%{name}

### make logdirs
%{__mkdir} -p %{buildroot}%{logdir}/
%{__mkdir} -p %{buildroot}%{logdir}/archives/

### move idoutils sample configs to final name
mv %{buildroot}%{_sysconfdir}/%{name}/ido2db.cfg-sample %{buildroot}%{_sysconfdir}/%{name}/ido2db.cfg
mv %{buildroot}%{_sysconfdir}/%{name}/idomod.cfg-sample %{buildroot}%{_sysconfdir}/%{name}/idomod.cfg

### copy idutils db-script
cp -r module/idoutils/db %{buildroot}%{_sysconfdir}/%{name}/idoutils


%pre
# Add icinga user
/usr/sbin/groupadd icinga 2> /dev/null || :
/usr/sbin/groupadd icingacmd 2> /dev/null || :
/usr/sbin/useradd -c "icinga" -s /sbin/nologin -r -d /var/icinga -G icingacmd -g icinga icinga 2> /dev/null || :


%post
/sbin/chkconfig --add icinga

%preun
if [ $1 -eq 0 ]; then
    /sbin/service icinga stop &>/dev/null || :
    /sbin/chkconfig --del icinga
fi

%pre gui
# Add apacheuser in the icingacmd group
  /usr/sbin/usermod -a -G icingacmd %{apacheuser}

%post idoutils
/sbin/chkconfig --add ido2db

%preun idoutils
if [ $1 -eq 0 ]; then
    /sbin/service idoutils stop &>/dev/null || :
    /sbin/chkconfig --del ido2db
fi


%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,icinga,icinga,-)
%attr(755,root,root) %{_initrddir}/%{name}
%dir %{_sysconfdir}/%{name}
%config(noreplace) %{_sysconfdir}/%{name}/cgi.cfg
%config(noreplace) %{_sysconfdir}/%{name}/cgiauth.cfg
%config(noreplace) %{_sysconfdir}/%{name}/icinga.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/commands.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/contacts.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/localhost.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/printer.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/switch.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/templates.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/timeperiods.cfg
%config(noreplace) %{_sysconfdir}/%{name}/objects/windows.cfg
%config(noreplace) %{_sysconfdir}/%{name}/resource.cfg
%{_bindir}/icinga
%{_bindir}/icingastats
%{_bindir}/p1.pl
%{logdir}
%dir %{_localstatedir}/spool/%{name}/checkresults
%attr(2755,icinga,icingacmd) %{_localstatedir}/spool/%{name}/rw/

%files doc
%defattr(-,icinga,icinga,-)
%{_datadir}/%{name}/docs
%{_datadir}/%{name}/doxygen

%files gui
%defattr(-,icinga,icinga,-)
%config(noreplace) %attr(-,root,root) %{apacheconfdir}/%{name}.conf
%dir %{_datadir}/%{name}
%{_libdir}/%{name}/cgi
%{_datadir}/%{name}/contexthelp
%{_datadir}/%{name}/images
%{_datadir}/%{name}/index.html
%{_datadir}/%{name}/js
%{_datadir}/%{name}/main.html
%{_datadir}/%{name}/media
%{_datadir}/%{name}/menu.html
%{_datadir}/%{name}/robots.txt
%{_datadir}/%{name}/sidebar.html
%{_datadir}/%{name}/ssi
%{_datadir}/%{name}/stylesheets
%{_datadir}/%{name}/top.html

%files idoutils
%defattr(-,icinga,icinga,-)
%attr(755,root,root) %{_initrddir}/ido2db
%config(noreplace) %{_sysconfdir}/%{name}/ido2db.cfg
%config(noreplace) %{_sysconfdir}/%{name}/idomod.cfg
%{_sysconfdir}/%{name}/idoutils
%{_bindir}/ido2db
%{_bindir}/log2ido
%{_bindir}/idomod.o

%files api
%defattr(-,icinga,icinga,-)
%{_datadir}/%{name}/icinga-api
%attr(-,%{apacheuser},%{apacheuser}) %{_datadir}/%{name}/icinga-api/log


%changelog
* Thu Feb 17 2011 Christoph Maser <cmaser@gmx.de> - 1.3.0-3
- move command file to /var/spool/icinga
- move checkresults file to /var/spool/icinga
- move pidfile file to /var/log/icinga

* Tue Feb 15 2011 Christoph Maser <cmaser@gmx.de> - 1.3.0-2
- move cgis to libdir
- remove suse suppot (packages available at opensuse build system)
- add doxygen docs

* Wed Nov 03 2010 Michael Friedrich <michael.friedrich@univie.ac.at> - 1.3.0-1
- prepared 1.3.0, added log2ido for idoutils install

* Mon Oct 25 2010 Christoph Maser <cmaser@gmx.de> - 1.2.1-1
- update for release 1.2.1
- add build dep for httpd
- set INSTALL_OPTS_WEB=""

* Thu Sep 30 2010 Christoph Maser <cmaser@gmx.de> - 1.2.0-1
- update for release 1.2.0

* Mon Sep 20 2010 Michael Friedrich <michael.friedrich@univie.ac.at> - 1.0.3-4
- remove php depency for classic gui

* Wed Sep 01 2010 Christoph Maser <cmaser@gmx.de> - 1.0.3-3
- Put documentation in a separate package

* Tue Aug 31 2010 Christoph Maser <cmaser@gmx.de> - 1.0.3-2
- Set icinga-api logdir ownership to apache user 
- add php dependency for icinga-gui subpackage

* Wed Aug 18 2010 Christoph Maser <cmaser@gmx.de> - 1.0.3-1
- Update to 1.0.3-1

* Thu Jul 05 2010 Christoph Maser <cmaser@gmx.de> - 1.0.2-2
- Enable debuginfo

* Thu Jun 24 2010 Christoph Maser <cmaser@gmx.de> - 1.0.2-1
- Update to 1.0.2-1

* Wed Mar 03 2010 Christoph Maser <cmr@financial.com> - 1.0.1-1
- Update to 1.0.1-1

* Tue Dec 15 2009 Christoph Maser <cmr@financial.com> - 1.0-1
- Update to 1.0-1

* Mon Oct 26 2009 Christoph Maser <cmr@financial.com> - 1.0-0.RC1.2
- Split out icinga-api in sub package

* Mon Oct 26 2009 Christoph Maser <cmr@financial.com> - 1.0-0.RC1.1
- Update to 1.0-RC1
- Correct checkconfig --del in idoutils #preun

* Mon Oct 26 2009 Christoph Maser <cmr@financial.com> - 0.8.4-3
- Use icingacmd group and add apache user to that group instead
  of using apachegroup as icinga command group.

* Wed Oct 07 2009 Christoph Maser <cmr@financial.com> - 0.8.4-2
- make packages openSUSE compatible
- add #apachecondir, #apacheuser, #apachegroup depending on vendor
- configure add --with-httpd-conf=#{apacheconfdir} 
- configure add --with-init-dir=#{_initrddir}

* Wed Sep 16 2009 Christoph Maser <cmr@financial.com> - 0.8.4-1
- Update to version 0.8.4.

* Tue Sep 15 2009 Christoph Maser <cmr@financial.com> - 0.8.3-3
- Apply patch from 
  https://git.icinga.org/index?p=icinga-core.git;a=commit;h=8b3505883856310472979b152b9960f81cdbaad7

* Tue Sep 15 2009 Christoph Maser <cmr@financial.com> - 0.8.3-2
- Apply patch from 
  https://git.icinga.org/index?p=icinga-core.git;a=commit;h=068baf7bfc99a2a5a88b64d06df49d7395008b40

* Wed Sep 09 2009 Christoph Maser <cmr@financial.com> - 0.8.3-1
- Update to version 0.8.3.

* Thu Aug 27 2009 Christoph Maser <cmr@financial.com> - 0.8.2-3
- fix dir name ndoutils -> idoutils

* Thu Aug 27 2009 Christoph Maser <cmr@financial.com> - 0.8.2-2
- fix idututils post script
- copy database scripts from source to sysconfigdir

* Sat Aug 22 2009 Christoph Maser <cmr@financial.com> - 0.8.2-1
- Update to release 0.8.2.
- remove idoutils-init, init-script for ido2db is shipped now 

* Sun Jul 19 2009 Christoph Maser <cmr@financial.com> - 0.8.1-1
- initial package

