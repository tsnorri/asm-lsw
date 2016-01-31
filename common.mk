# Ignore Xcode's setting since the SDK may contain older versions of Clang and libc++.
unexport SDKROOT

OPT_FLAGS	= -gfull -O0
#OPT_FLAGS	= -Os
#OPT_FLAGS	= -O2

CFLAGS		= -c -std=c99 $(OPT_FLAGS) -Wall -Werror -Wno-unused
CXXFLAGS	= -c -std=c++14 -stdlib=libc++ $(OPT_FLAGS) -Wall -Werror -Wno-unused
CPPFLAGS	= -I../include $(INCLUDES)
LDFLAGS		= $(OPT_FLAGS) $(LIBPATHS) -ldivsufsort -lphf -lsdsl


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
