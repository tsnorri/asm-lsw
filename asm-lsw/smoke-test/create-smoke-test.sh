#!/bin/bash

target=tests.mk

echo "include ../../local.mk" > "${target}"
echo "include ../../common.mk" >> "${target}"
echo -n "all:" >> "${target}"
rm -f all.cc

ls -1 ../include/asm_lsw | while read x
do
	b="${x%.*}"
	include="#include <asm_lsw/${x}>"
	if [ ! -e "${b}.cc" ]
	then
		echo "${include}" > "${b}.cc"
	fi
	echo "${include}" >> "all.cc"
	echo -n " ${b}.o" >> "${target}"
done

echo " all.o" >> "${target}"

printf "\n\nclean:\n\t\$(RM) *.cc *.o ${target}" >> "${target}"
