# What is Icinga 1.x?

Icinga 1.x began as a fork of Nagios and is backward compatible.
So, Nagios configurations, plugins and addons can all be used with Icinga 1.x.
Though Icinga 1.x retains all the existing features of its predecessor, it builds
on them to add many long awaited patches and features requested by the user community.

Icinga is an enterprise grade open source monitoring system which keeps watch over a
network and any conceivable network resource, notifies the user of errors and recoveries,
and generates performance data for reporting.
Scalable and extensible, Icinga can monitor complex, large environments across dispersed
locations. Icinga is licensed under GPL V2 and is free to use, distribute and modify.

If you are looking for Icinga 2, please checkout https://www.icinga.org/icinga/icinga-2/

# With Icinga you can

MONITOR – ANY NETWORK AND ALL ITS RESOURCES

Network services: SMTP, POP3, HTTP, SNMP, NNTP, PING, etc.
Host resources: CPU load, disk utilization, system logs, etc
Server components: Switches, routers, temperature and humidity sensors, etc

NOTIFY –WHEN ISSUES ARISE AND ARE RESOLVED

Through any channel (eMail, SMS, phone call, etc)
Escalate alerts to other users or communication channels
Fine tune notification settings (accounting for dependencies between hosts & services)

REPORT – ON PERFORMANCE AND PLAN AHEAD

Capacity utilization to plan for growth
Chart graphs with addons like PNP or Grapher

# More on Icinga

Icinga takes open source monitoring to the next level - check out the features new and old.

https://www.icinga.org/icinga/icinga-1/

For installation instructions, use a web browser to read the HTML documentation
in the html/docs subdirectory.

# Online resources

Visit the Icinga homepage at https://www.icinga.org for

* online documentation 		http://docs.icinga.org
* wiki 				https://wiki.icinga.org
* new releases			https://www.icinga.org/blog/
* bug reports 			https://dev.icinga.org
* faq				https://www.icinga.org/icinga/faq/get-help/
* support			https://www.icinga.org/support/
* packages			https://www.icinga.org/download/

Please report any bugs, feature requests, ideas, feedback and help make Icinga
better!

Thanks for using Icinga :-)


# Known Issues

These items are known to be buggy/not working. Feel free to help reproduce and
patch these issues away.

* `check_period != 24x7` causes scheduler to skip checks [#1782](https://dev.icinga.org/issues/1782)
* Duplicated service in service groups when service is added via nested servicegroup [#4856](https://dev.icinga.org/issues/4856)
* 'reset modified attributes' does not restore old configuration value [#4928](https://dev.icinga.org/issues/4928)

