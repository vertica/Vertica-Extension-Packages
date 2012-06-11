##########################
# This makefile contains commands to build whatever third-party libraries your
# functions require. See web_package/src/third-party/makefile for an examplex
##########################

TAR = tar

THIRD_PARTY = $(shell pwd)
INSTALL_DIR = $(THIRD_PARTY)/../../build
PYTHON=python

.PHONY: all
all: stemmer jsoncpp

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
	@cp `find jsoncpp-src-0.5.0/libs -name "*.a" | head -n1 ` $(INSTALL_DIR)/lib/libjson.a
	@cp -r jsoncpp-src-0.5.0/include//* $(INSTALL_DIR)/include
	touch $@



clean:
	cd libstemmer_c; $(MAKE) clean
	@rm -rf jsoncpp-src-0.5.0
	@rm -rf curl-7.23.1
	@rm -rf $(INSTALL_DIR)/include
	@rm -rf $(INSTALL_DIR)/lib
	@rm -rf $(INSTALL_DIR)/bin
	@rm -rf $(INSTALL_DIR)/share
