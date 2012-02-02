##########################
# This makefile contains commands to build whatever third-party libraries your
# functions require. See web_package/src/third-party/makefile for an examplex
##########################

TAR = tar

THIRD_PARTY = $(shell pwd)
INSTALL_DIR = $(THIRD_PARTY)/../../build
PYTHON=python

.PHONY: all
all: stemmer jsoncpp libcurl

.PHONY: stemmer
stemmer:
	$(MAKE) -C libstemmer_c
	@mkdir -p $(INSTALL_DIR)/lib
	@mkdir -p $(INSTALL_DIR)/include
	@cp libstemmer_c/libstemmer.o $(INSTALL_DIR)/lib
	@cp -r libstemmer_c/include/* $(INSTALL_DIR)/include

.PHONY: jsoncpp
jsoncpp: jsoncpp-src-0.5.0/.done

jsoncpp-src-0.5.0/.done:
	@mkdir -p $(INSTALL_DIR)
	@echo "Unpacking, configuring and installing jsoncpp...";
	$(TAR) xzf jsoncpp-src-0.5.0.tar.gz
	(cd jsoncpp-src-0.5.0; unzip -n ../scons-local-2.1.0.zip)
	(cd jsoncpp-src-0.5.0; $(PYTHON) scons.py platform=linux-gcc)
	@cp jsoncpp-src-0.5.0/libs/linux-gcc-4.4.5/libjson_linux-gcc-4.4.5_libmt.a $(INSTALL_DIR)/lib
	@cp -r jsoncpp-src-0.5.0/include//* $(INSTALL_DIR)/include
	touch $@

.PHONY: libcurl
libcurl: curl-7.23.1/.done

curl-7.23.1/.done:
	mkdir -p $(INSTALL_DIR)
	@echo "Unpacking, configuring and installing libcurl...";
	$(TAR) xzf curl-7.23.1.tar.gz
	(cd curl-7.23.1; ./configure --prefix=$(INSTALL_DIR) --enable-shared=no)
	(cd curl-7.23.1; make; make install)
	touch $@

clean:
	cd libstemmer_c; $(MAKE) clean
	@rm -rf jsoncpp-src-0.5.0
	@rm -rf curl-7.23.1
	@rm -rf $(INSTALL_DIR)/include
	@rm -rf $(INSTALL_DIR)/lib
	@rm -rf $(INSTALL_DIR)/bin
	@rm -rf $(INSTALL_DIR)/share
