include ../../local.mk
include ../../common.mk

.PHONY: run_tests
.PRECIOUS: tests

LDFLAGS_COVERAGE ?= -lgcov

CPPFLAGS	+= -DASM_LSW_EXCEPTIONS
CXXFLAGS	+= -fprofile-arcs -ftest-coverage
LDFLAGS		+= $(LDFLAGS_COVERAGE)

OBJECTS		=	bp_support_sparse_tests.o \
				k1_matcher_tests.o \
				kn_matcher_tests.o \
				map_adaptor_tests.o \
				matrix_tests.o \
				pool_allocator_tests.o \
				x_fast_trie_tests.o \
				x_fast_trie_compact_tests.o \
				x_fast_trie_compact_as_tests.o \
				y_fast_trie_tests.o \
				y_fast_trie_compact_tests.o \
				y_fast_trie_compact_as_tests.o \
				test_main.o

all: tests

coverage: run_tests
	$(MKDIR) -p coverage
	$(GCOVR) --root .. --print-summary --html --html-details -o coverage/index.html

run_tests: tests
	./tests

tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) tests $(OBJECTS) *.gcda *.gcno
	$(RM) -r coverage
