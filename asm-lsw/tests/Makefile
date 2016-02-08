include ../../local.mk
include ../../common.mk

.PHONY: run_tests
.PRECIOUS: tests

CPPFLAGS += -DASM_LSW_EXCEPTIONS

OBJECTS	=	bp_support_sparse_tests.o \
			test_main.o \
			trie_tests.o

all: tests

run_tests: tests
	./tests

tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) tests $(OBJECTS)
