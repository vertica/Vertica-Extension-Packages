############################
# Vertica Analytic Database
#
# Makefile to build package directory
#
# Copyright 2011 Vertica Systems, an HP Company
############################

ifdef RUN_VALGRIND
VALGRIND=valgrind --leak-check=full
endif

SDK?=/opt/vertica/sdk
VSQL?=vsql

VERTICA_SDK_INCLUDE = $(SDK)/include
SIMULATOR_PATH = $(SDK)/simulator
DEBUG_DATA_PATH = $(shell pwd)/test-data



THIRD_PARTY = $(shell pwd)/src/third-party
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include


# Required object files
BUILD_FILES      = build/Vertica.o \
		   $(addprefix build/,rijndael.o my_aes.o) \
		   $(addprefix build/,AESEncrypt.o AESDecrypt.o) 

PACKAGE_LIBNAME   = lib/Encryption.so

CXX=g++
CXXFLAGS=-D HAVE_LONG_LONG_INT_64 -c -I ../include -Wall -Wno-unused-value -fPIC -I $(VERTICA_SDK_INCLUDE) -I $(THIRD_PARTY_INCLUDE)
LDFLAGS=-shared

# add optimization if not a debug build
ifndef DEBUG
CXXFLAGS+= -O3 -DNDEBUG
CFLAGS+= -O3 -DNDEBUG
endif

export PATH := $(THIRD_PARTY_LIB):$(PATH)

.PHONEY: $(PACKAGE_LIBNAME)

all: $(PACKAGE_LIBNAME)

# build package functions
$(PACKAGE_LIBNAME): $(BUILD_FILES) 
	mkdir -p lib
	$(CXX) $(LDFLAGS) -o $@ $(BUILD_FILES) 

# rule to make build/XXX.so from src/XXX.so
build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

build/Vertica.o: $(VERTICA_SDK_INCLUDE)/Vertica.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(VERTICA_SDK_INCLUDE)/Vertica.cpp -o $@

# rule to make build/XX.o from third-party/src/*.c
build/%.o: $(THIRD_PARTY)/src/%.c
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@ 

install: $(PACKAGE_LIBNAME) ddl/install.sql
	$(VSQL) -f ddl/install.sql
uninstall: ddl/uninstall.sql
	$(VSQL) -f ddl/uninstall.sql

# run examples
test:
	$(VSQL) -f examples/samples.sql


# Run the simulator against the functons
sim_test: all simulator
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) FunctionName $(DEBUG_DATA_PATH)/testdata.txt

# build the simulator (in SIMULATOR_PATH) and simlink it here
simulator:
	$(MAKE) -C $(SIMULATOR_PATH)
	ln -f -s $(SIMULATOR_PATH)/vsim 

clean:
	rm -rf build
	rm -rf lib
	rm -f vsim*

