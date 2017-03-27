[![Build Status](https://travis-ci.org/Icinga/icinga-core.svg?branch=master)](https://travis-ci.org/Icinga/icinga-core)

# Icinga 1.x

![Icinga Logo](https://www.icinga.org/wp-content/uploads/2014/06/icinga_logo.png)

#### Table of Contents

1. [About][About]
2. [License][License]
3. [Installation][Installation]
4. [Documentation][Documentation]
5. [Support][Support]
6. [Development and Contributions][Development]

## About

Icinga 1.x began as a fork of Nagios and is backward compatible.
So, Nagios configurations, plugins and addons can all be used with Icinga 1.x.
Though Icinga 1.x retains all the existing features of its predecessor, it builds
on them to add many long awaited patches and features requested by the user community.

Icinga is an enterprise grade open source monitoring system which keeps watch over a
network and any conceivable network resource, notifies the user of errors and recoveries,
and generates performance data for reporting.
Scalable and extensible, Icinga can monitor complex, large environments across dispersed
locations. Icinga is licensed under GPL V2 and is free to use, distribute and modify.

Icinga 2 is the new monitoring core flagship and actively developed. Icinga 1.x
receives security and bug fixes only.

Please checkout https://www.icinga.com/products/icinga-2/ for more details.

## License

Icinga 1 and the Icinga 1 documentation are licensed under the terms of the GNU
General Public License Version 2, you will find a copy of this license in the
LICENSE file included in the source package.

## Installation

Read the [INSTALLING](INSTALLING) file for more information about how to install it.

## Documentation

The documentation is located in the [html/doc/](html/doc/) directory. The latest documentation
is also available on https://docs.icinga.com

## Support

Check the project website at https://www.icinga.com for status updates. Join the
[community channels](https://www.icinga.com/community/get-involved/) for questions
or ask an Icinga partner for [professional support](https://www.icinga.com/services/support/).

## Development

The Git repository is located on [GitHub](https://github.com/Icinga/icinga-core).

Icinga 1 is written in C and can be built on Linux/Unix.

### Contributing

There are many ways to contribute to Icinga -- whether it be sending patches,
testing, reporting bugs, or reviewing and updating the documentation. Every
contribution is appreciated!

Read the [contributing section](https://www.icinga.com/community/get-involved/) and
get familiar with the code.

Pull requests on [GitHub](https://github.com/Icinga/icinga-core) are preferred.

### Testing

Basic unit test coverage is provided by running `make test` during package builds.

Snapshot packages from the laster development branch are available inside the
[package repository](http://packages.icinga.org).

[About]: #about
[License]: #license
[Installation]: #installation
[Documentation]: #documentation
[Support]: #support
[Development]: #development
