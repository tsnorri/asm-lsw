all:
	CPPFLAGS="-DPHF_NO_LIBCXX -DPHF_NO_COMPUTED_GOTOS" $(MAKE) -C lib/phf libphf.a
	$(MAKE) -C asm-lsw

clean:
	$(MAKE) -C lib/phf clean
	$(MAKE) -C asm-lsw clean
