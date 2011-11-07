all: uriparser 

.PHONY: uriparser


ICU_PLATFORM = Linux
# don't allow parallel make settings to be inherited
MAKE := make -j1
TAR = tar
AR = ar
PATCH = patch
YACC = yacc
RANLIB = ranlib
STRIP = strip
UNZIP = unzip


URIPARSER_DIR = $(THIRD_PARTY)/uriparser
URIPARSER_TGZ = $(URIPARSER_DIR)/uriparser-0.7.5.tar.gz
URIPARSER_SRC = $(URIPARSER_DIR)/uriparser-0.7.5

THIRD_PARTY = $(shell pwd)
INSTALL_DIR = $(THIRD_PARTY)/install

uriparser:    $(URIPARSER_DIR)/.done

# force PIC code in static libraries
CFLAGS+=-fPIC
export CFLAGS

$(URIPARSER_DIR)/.done:
	@mkdir -p $(INSTALL_DIR)
	@echo "Unpacking, configuring and installing uriparser...";
	(cd $(URIPARSER_DIR); $(TAR) xzf $(URIPARSER_TGZ))
	(cd $(URIPARSER_SRC); $(CONFIGURE_ENV) ./configure --prefix=$(INSTALL_DIR) --disable-test --exec-prefix=$(INSTALL_DIR))
	(cp $(URIPARSER_DIR)/doc_configure $(URIPARSER_SRC)/doc/configure; )
	(cd $(URIPARSER_SRC)/doc; ./configure --prefix=$(INSTALL_DIR) --enable-static=yes --exec-prefix=$(INSTALL_DIR))
	(cd $(URIPARSER_SRC); $(MAKE) )
	(cd $(URIPARSER_SRC)/doc/html; touch foo.gif)
	(cd $(URIPARSER_SRC); $(MAKE) install )
	touch $@

clean: 
	rm -f $(URIPARSER_DIR)/.done
	rm -rf $(URIPARSER_SRC)
	rm -rf $(INSTALL_DIR)