#!/bin/bash

echo "Failed to compile"
echo "-----------------"
ls -1 *.cc | while read x
do
	if [ ! -e "${x%.*}.o" ]
	then
		echo "$x"
		# FIXME Change exit status to 1 here in order to stop on error.
	fi
done
