include ../../local.mk
include ../../common.mk

.PHONY: all clean

all:
	./create-smoke-test.sh
	-$(MAKE) -k -f tests.mk
	./check-results.sh

clean:
	-$(MAKE) -f tests.mk clean
