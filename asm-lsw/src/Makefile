include ../../local.mk
include ../../common.mk

CPPFLAGS	+= -DASM_LSW_EXCEPTIONS

OBJECTS		=	vector_source.o

all: libasm_lsw.a

clean:
	$(RM) libasm_lsw.a $(OBJECTS)

libasm_lsw.a: $(OBJECTS)
	ar cru libasm_lsw.a $(OBJECTS)
