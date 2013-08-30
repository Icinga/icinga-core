#!/bin/bash
###
# Make a tarball
# (c) 2013 NETWAYS GmbH
# by Markus Frosch <markus.frosch@netways.de
###

set -e

name=`basename *.spec .spec`
version=`git describe --tags HEAD | sed 's/^v//'`
filename="$name-$version"

git archive --worktree-attributes \
    -o "../${filename}.tar.gz" \
    --prefix="$filename/" HEAD

echo "created ../${filename}.tar.gz"

