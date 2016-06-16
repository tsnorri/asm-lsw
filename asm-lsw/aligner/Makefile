include ../../local.mk
include ../../common.mk

LDFLAGS			+= -L../src -lasm_lsw -lboost_iostreams-mt


OBJECTS			=	align.o \
					cmdline.o \
					create_index.o \
					main.o


all: asm-lsw-aligner


clean:
	$(RM) asm-lsw-aligner cmdline.c cmdline.h $(OBJECTS)


asm-lsw-aligner: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o asm-lsw-aligner $(OBJECTS)
