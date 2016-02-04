#!/bin/bash

status=0
ls -1 *.cc | 
{
	while read x
	do
		if [ ! -e "${x%.*}.o" ]
		then
			if [ 0 -eq "${status}" ]
			then
				status=1
				echo "Failed to compile"
				echo "-----------------"
			fi
			echo "$x"
		fi
	done
	exit ${status}
}
