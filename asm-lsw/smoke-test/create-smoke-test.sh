#!/bin/bash

target=tests.mk

printf "# Automatically generated.\n\n" > "${target}"
printf "include ../../local.mk\n" >> "${target}"
printf "include ../../common.mk\n\n" >> "${target}"
printf ".PHONY: all clean\n\n" >> "${target}"
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

printf "\nclean:\n\t\$(RM) *.cc *.o *.gcno ${target}" >> "${target}"
