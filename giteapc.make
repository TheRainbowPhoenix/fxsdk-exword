# giteapc: version=1

# Parameters for custom configurations:
#   PREFIX           Install prefix
#   FXSDK_CONFIGURE  Configure options (see ./configure --help)

PREFIX ?= $(GITEAPC_PREFIX)

-include giteapc-config.make

configure:
	@ ./configure --prefix=$(PREFIX) $(FXSDK_CONFIGURE)

build:
	@ make all

install:
	@ make install

uninstall:
	@ make uninstall

.PHONY: configure build install uninstall
