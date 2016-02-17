# Ignore Xcode's setting since the SDK may contain older versions of Clang and libc++.
unexport SDKROOT

CC		?= gcc
CXX		?= g++
GCOVR	?= gcovr
MKDIR	?= mkdir
CLOC	?= cloc

WARNING_FLAGS	= -Wall -Werror -Wno-unused -Wno-missing-braces

LOCAL_CXXFLAGS		?= -std=c++14
OPT_FLAGS_DEBUG		?= -ggdb -O0
OPT_FLAGS_RELEASE	?= -O2

CFLAGS		= -c -std=c99 $(OPT_FLAGS) $(WARNING_FLAGS)
CXXFLAGS	= -c $(OPT_FLAGS) $(LOCAL_CXXFLAGS) $(WARNING_FLAGS)
CPPFLAGS	=	-I../include \
				-I../../lib/bandit \
				-I../../lib/sdsl/build/include \
				-I../../lib/sdsl/build/external/libdivsufsort/include \
				-I../../lib/phf \
				$(LOCAL_CPPFLAGS)
LDFLAGS		=	-L../../lib/sdsl/build/lib \
				-L../../lib/sdsl/build/external/libdivsufsort/lib \
				-L../../lib/phf \
				$(OPT_FLAGS) $(LOCAL_LDFLAGS) -ldivsufsort -lphf -lsdsl

ifeq ($(BUILD_STYLE),release)
	OPT_FLAGS	= $(OPT_FLAGS_RELEASE)
else
	OPT_FLAGS	= $(OPT_FLAGS_DEBUG)
endif


%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<

%.h %.c: %.ggo
	$(GENGETOPT) --input="$<"

%.cc: %.rl
	$(RAGEL) -G2 -o $@ $<

%.pdf: %.dot
	$(DOT) -Tpdf -o$@ $<

%.dot: %.rl
	$(RAGEL) -V -p -o $@ $<
