all:
	$(MAKE) -C smoke-test
	$(MAKE) -C src
	$(MAKE) -C tests coverage
	$(MAKE) -C aligner

clean:
	$(MAKE) -C smoke-test clean
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(MAKE) -C aligner clean
