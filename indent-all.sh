#!/bin/sh

for file in `find . -type f -name "*.[c]"`; do
	 sh indent.sh $file
done
