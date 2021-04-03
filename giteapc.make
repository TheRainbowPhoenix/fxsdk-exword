# giteapc: version=1

# Parameters for custom configurations:
#   PREFIX           Install prefix
#   FXSDK_CONFIGURE  Configure options (see ./configure --help)

PREFIX ?= $(GITEAPC_PREFIX)

-include giteapc-config.make

configure:
	@ cmake -B build -DCMAKE_INSTALL_PREFIX="$(PREFIX)" $(FXSDK_CONFIGURE)

build:
	@ make -C build

install:
	@ make -C build install

uninstall:
	@ if [ -e build/install_manifest.txt ]; then \
	     xargs rm -f < build/install_manifest.txt; \
          fi

.PHONY: configure build install uninstall
