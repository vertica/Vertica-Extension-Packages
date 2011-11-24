all: uriparser  pcre



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


THIRD_PARTY = $(shell pwd)
INSTALL_DIR = $(THIRD_PARTY)/../../build

URIPARSER_DIR = $(THIRD_PARTY)/uriparser
URIPARSER_TGZ = $(URIPARSER_DIR)/uriparser-0.7.5.tar.gz
URIPARSER_SRC = $(URIPARSER_DIR)/uriparser-0.7.5

.PHONY: uriparser pcre
uriparser:    $(URIPARSER_DIR)/.done


PCRE_DIR = $(THIRD_PARTY)/pcre
PCRE_TBZ = $(PCRE_DIR)/pcre-8.10.tar.bz2
PCRE_SRC = $(PCRE_DIR)/pcre-8.10

.PHONY: pcre
pcre:         $(PCRE_DIR)/.done


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

$(PCRE_DIR)/.done:
	mkdir -p $(INSTALL_DIR)
	@echo "Unpacking, configuring and installing pcre...";
	(cd $(PCRE_DIR); $(TAR) xjf $(PCRE_TBZ) )
	(cd $(PCRE_SRC); ./configure --prefix=$(INSTALL_DIR) )
	(cd $(PCRE_SRC); make; make install)
	touch $@

# download pcre from sourceforge
pcre-8.10.tar.bz2:
	@echo "Downloading pcre from sourceforge..."
	wget http://downloads.sourceforge.net/project/pcre/pcre/8.10/pcre-8.10.tar.bz2

clean: 
	rm -f $(URIPARSER_DIR)/.done
	rm -rf $(URIPARSER_SRC)
	rm -f $(PCRE_DIR)/.done
	rm -rf $(PCRE_SRC)
	rm -rf $(INSTALL_DIR)