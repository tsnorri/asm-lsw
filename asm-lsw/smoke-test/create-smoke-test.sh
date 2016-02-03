#!/bin/bash

echo "include ../../local.mk" > Makefile
echo "include ../../common.mk" >> Makefile
echo -n "all:" >> Makefile
rm -f all.cc

ls -1 ../include/asm_lsw | while read x
do
	b="${x%.*}"
	include="#include <asm_lsw/${x}>"
	echo "${include}" > "${b}.cc"
	echo "${include}" >> "all.cc"
	echo -n " ${b}.o" >> Makefile
done

echo " all.o" >> Makefile

printf "\n\nclean:\n\t\$(RM) *.cc *.o Makefile" >> Makefile
