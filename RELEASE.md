# Quality Assurance

Review and test the changes and issues for this version.
https://dev.icinga.org/projects/icinga-core/roadmap

# Release Workflow

## Authors

Update the [.mailmap](.mailmap) and [AUTHORS](AUTHORS) files:

    $ git log --use-mailmap | grep ^Author: | cut -f2- -d' ' | sort | uniq > AUTHORS

## Version

Update the current version (and db schema, if required)

    $ ./update-version 1.13.0
    $ ./update-version-schema 1.13.0

## Docs

Update the Changelog and generate the docs

    $ ./configure; make create-docs

## Changelog

Manually fetch the issue list from the [roadmap](https://dev.icinga.org/projects/icinga-core/roadmap).

## Git Tag

Commit these changes to the "master" branch:

    $ git commit -v -a -m "Release version <VERSION>"

For minor releases: Cherry-pick this commit into the "support" branch.

Create a signed tag (tags/v<VERSION>) on the "master" branch (for major
releases) or the "support" branch (for minor releases).

MF:

    $ git tag -u D14A1F16 -m "Version <VERSION>" v<VERSION>

Push the tag.

    $ git push --tags

For major releases: Create a new "support" branch:

    $ git checkout master
    $ git checkout -b support/1.13
    $ git push -u origin support/1.13

For minor releases: Push the support branch and cherry-pick the release commit into master:

    $ git push -u origin support/1.13
    $ git checkout master
    $ git cherry-pick support/1.13
    $ git push origin master

## Release Tarball


Use "git archive" to build the release tarball:

    $ VERSION=1.13.3
    $ git archive --format=tar --prefix=icinga-$VERSION/ tags/v$VERSION | gzip >icinga-$VERSION.tar.gz
    $ md5sum icinga-$VERSION.tar.gz > icinga-$VERSION.tar.gz.md5

Finally you should verify that the tarball only contains the files it should contain:

    $ VERSION=1.13.3
    $ tar ztf icinga-$VERSION.tar.gz | less

# External Dependencies

## Build Server

### Linux

* Build the newly created git tag for Debian/RHEL/SuSE.
* Start a new docker container and install/run icinga

Example for CentOS7:

    $ sudo docker run -ti centos:latest bash

    # yum -y install http://packages.icinga.org/epel/7/release/noarch/icinga-rpm-release-7-1.el7.centos.noarch.rpm
    # yum -y install icinga icinga-gui
    # icinga -v /etc/icinga/icinga.cfg

## Github Release

Create a new release from the newly created git tag.
https://github.com/Icinga/icinga-core/releases

Upload the release tarball amd md5 file.

## Online Documentation

Ssh into the web box, become docgen user
and run the update script (there's a daily cronjob for that).

## Announcement

* Create a new blog post on www.icinga.org/blog
* Send announcement mail to icinga-announce@lists.icinga.org
* Social media: [Twitter](https://twitter.com/icinga), [Facebook](https://www.facebook.com/icinga), [G+](http://plus.google.com/+icinga), [Xing](https://www.xing.com/communities/groups/icinga-da4b-1060043), [LinkedIn](https://www.linkedin.com/groups/Icinga-1921830/about)



Building Release Tarballs
-------------------------

In order to build a release tarball you should first check out the Git repository
in a new directory. If you're using an existing check-out you should make sure
that there are no local modifications:

$ git status

Here's a short check-list for releases:

* Update the .mailmap and AUTHORS files
    $ git log --use-mailmap | grep ^Author: | cut -f2- -d' ' | sort | uniq > AUTHORS
* Bump the version in icinga.spec.
* Update the current version (and db schema, if required)
    $ ./update-version 1.13.0
    $ ./update-version-schema 1.13.0
* Update the Changelog and generate the docs
    $ ./configure; make create-docs
* Commit these changes to the "support/1.13" branch and create a signed tag (tags/v<VERSION>).
    $ git commit -v -a -m "Release version <VERSION>"
    $ git tag -u D14A1F16 -m "Version <VERSION>" v<VERSION>
    $ git push --tags
* Merge the "support/1.13" branch into the "master" branch (using --ff-only).
    $ git checkout master
    $ git merge --ff-only support/1.13
    $ git push origin master



