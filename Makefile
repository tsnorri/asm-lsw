include local.mk
include common.mk

.PHONY: all clean-all clean clean-dependencies dependencies cloc

all: dependencies
	$(MAKE) -C asm-lsw

clean-all: clean clean-dependencies

clean:
	$(MAKE) -C asm-lsw clean

clean-dependencies:
	$(MAKE) -C lib/phf clean
	cd lib/sdsl/build && ./clean.sh

dependencies: lib/phf/libphf.a lib/sdsl/build/lib/libsdsl.a

cloc:
	$(CLOC) \
		asm-lsw/include \
		lib/sdsl/include/sdsl/csa_rao*.hpp \
		lib/sdsl/include/sdsl/elias_inventory.hpp \
		lib/sdsl/include/sdsl/isa_lsw.hpp \
		lib/sdsl/include/sdsl/isa_simple.hpp \
		lib/sdsl/include/sdsl/psi_k_support*.hpp

lib/phf/libphf.a:
	CC="$(CC)" \
	CXX="$(CXX)" \
	CPPFLAGS="-DPHF_NO_LIBCXX -DPHF_NO_COMPUTED_GOTOS $(LOCAL_CPPFLAGS)" \
	CFLAGS="$(LOCAL_CFLAGS) $(OPT_FLAGS)" \
	CXXFLAGS="$(LOCAL_CXXFLAGS) $(OPT_FLAGS)" \
	LDFLAGS="$(LOCAL_LDFLAGS) $(OPT_FLAGS)" \
	$(MAKE) -C lib/phf libphf.a

lib/sdsl/build/lib/libsdsl.a:
	cd lib/sdsl/build && ./clean.sh && \
	CC="$(CC)" \
	CXX="$(CXX)" \
	CFLAGS="$(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) $(OPT_FLAGS)" \
	CXXFLAGS="$(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(OPT_FLAGS)" \
	LDFLAGS="$(LOCAL_LDFLAGS) $(OPT_FLAGS)" \
	cmake -DCMAKE_BUILD_TYPE=RelWithDebAndAssert ..
	$(MAKE) -C lib/sdsl/build VERBOSE=1
