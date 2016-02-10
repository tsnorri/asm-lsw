include ../../local.mk
include ../../common.mk

.PHONY: run_tests
.PRECIOUS: tests

CPPFLAGS += -DASM_LSW_EXCEPTIONS
CXXFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS += -lgcov

OBJECTS	=	bp_support_sparse_tests.o \
			test_main.o \
			trie_tests.o

all: tests

coverage: run_tests
	mkdir -p coverage
	$(GCOVR) --root .. --print-summary --html --html-details -o coverage/index.html

run_tests: tests
	./tests

tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) tests $(OBJECTS) *.gcda *.gcno
	$(RM) -r coverage
