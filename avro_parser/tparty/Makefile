##########################
# This makefile contains commands to build whatever third-party libraries your
# functions require. See web_package/src/third-party/makefile for an examplex
##########################

TAR = tar
UNZIP = unzip

THIRD_PARTY = $(shell pwd)
INSTALL_DIR ?= $(THIRD_PARTY)/../build

ifndef DEBUG
CXXFLAGS+= -O3 -DNDEBUG 
CFLAGS+= -O3 -DNDEBUG
endif

.PHONY: all
all: cmake boost avro 

.PHONY: cmake
cmake: $(INSTALL_DIR)/src/cmake-2.8.8/.done

$(INSTALL_DIR)/src/cmake-2.8.8/.done: 
	@mkdir -p $(INSTALL_DIR)/lib
	@mkdir -p $(INSTALL_DIR)/include
	@mkdir -p $(INSTALL_DIR)/src
	@echo "Unpacking, configuring and installing cmake...";
	(cd $(INSTALL_DIR)/src; $(TAR) xzf $(THIRD_PARTY)/cmake-2.8.8.tar.gz)
	(cd $(INSTALL_DIR)/src/cmake-2.8.8; ./configure --prefix=$(INSTALL_DIR))
	(cd $(INSTALL_DIR)/src/cmake-2.8.8; gmake install)
	touch $@

.PHONY: boost
boost: $(INSTALL_DIR)/src/boost_1_50_0/.done

$(INSTALL_DIR)/src/boost_1_50_0/.done:
	@echo "Unpacking, configuring and installing boost...";
	(cd $(INSTALL_DIR)/src; $(TAR) xzf $(THIRD_PARTY)/boost_1_50_0.tar.gz)
	(cd $(INSTALL_DIR)/src/boost_1_50_0; ./bootstrap.sh --prefix=$(INSTALL_DIR) )
	(cd $(INSTALL_DIR)/src/boost_1_50_0; cxxflags=-fPIC ./b2 cxxflags="-fPIC" --exec-prefix=$(INSTALL_DIR) --link=static --libdir=$(INSTALL_DIR)/lib --includedir=$(INSTALL_DIR)/include --with-filesystem --with-program_options install) 
	touch $@

.PHONY: avro
avro: $(INSTALL_DIR)/src/avro-cpp-1.7.0/.done

$(INSTALL_DIR)/src/avro-cpp-1.7.0/.done:
	@mkdir -p $(INSTALL_DIR)/lib
	@mkdir -p $(INSTALL_DIR)/include
	@mkdir -p $(INSTALL_DIR)/src
	@echo "Unpacking, configuring and installing avro...";
	(cd $(INSTALL_DIR)/src; $(TAR) xzf $(THIRD_PARTY)/avro-cpp-1.7.0.tar.gz)
	(cd $(INSTALL_DIR)/src/avro-cpp-1.7.0; BOOST_ROOT=$(INSTALL_DIR) cmake -G "Unix Makefiles" -DCMAKE_CXX_FLAGS_RELEASE:STRING="-fPIC -Wall -O3 -DNDEBUG" ;make package;./avrocpp-1.7.0.sh --prefix=$(INSTALL_DIR) --skip-license --exclude-subdir)
	touch $@

real_clean:
	@rm -rf avro/.done
	@rm -rf $(INSTALL_DIR)/*
