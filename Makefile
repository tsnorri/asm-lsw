include local.mk

.PHONY: all clean-all clean clean-dependencies dependencies

all: dependencies
	$(MAKE) -C asm-lsw

clean-all: clean clean-dependencies

clean:
	$(MAKE) -C asm-lsw clean

clean-dependencies:
	$(MAKE) -C lib/phf clean
	cd lib/sdsl/build && ./clean.sh

dependencies: lib/phf/libphf.a lib/sdsl/build/lib/libsdsl.a

lib/phf/libphf.a:
	CC="$(CC)" \
	CXX="$(CXX)" \
	CPPFLAGS="-DPHF_NO_LIBCXX -DPHF_NO_COMPUTED_GOTOS $(LOCAL_CPPFLAGS)" \
	CFLAGS="$(LOCAL_CFLAGS)" \
	CXXFLAGS="$(LOCAL_CXXFLAGS)" \
	LDFLAGS="$(LOCAL_LDFLAGS)" \
	$(MAKE) -C lib/phf libphf.a

lib/sdsl/build/lib/libsdsl.a:
	cd lib/sdsl/build && ./clean.sh && \
	CC="$(CC)" \
	CXX="$(CXX)" \
	CFLAGS="$(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS)" \
	CXXFLAGS="$(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS)" \
	LDFLAGS="$(LOCAL_LDFLAGS)" \
	cmake ..
	$(MAKE) -C lib/sdsl/build
