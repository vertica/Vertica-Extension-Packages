############################
# Vertica Analytic Database
#
# Makefile to build package directory
#
# Copyright 2011 Vertica Systems, an HP Company
############################

SDK?=/opt/vertica/sdk
VSQL?=vsql

VERTICA_SDK_INCLUDE = $(SDK)/include
SIMULATOR_PATH = $(SDK)/simulator

THIRD_PARTY         = $(shell pwd)/src/third-party
THIRD_PARTY_INCLUDE = build/include
THIRD_PARTY_LIB     = build/lib

# Add in your source files below
BUILD_FILES      = build/Vertica.o \
		   $(addprefix build/,ApacheParser.o UriExtractor.o w3cLogParser.o)

# Define the .so name here (and update the references in ddl/install.sql and ddl/uninstall.sql)
PACKAGE_LIBNAME   = lib/WebPackage.so

CXX=g++
CXXFLAGS=-g -D HAVE_LONG_LONG_INT_64 -c -I ../include -Wall -Wno-unused-value -fPIC -I $(VERTICA_SDK_INCLUDE) -I $(THIRD_PARTY_INCLUDE)
LDFLAGS=-shared -L $(THIRD_PARTY_LIB) -Wl,-Bstatic -luriparser -Wl,-Bdynamic

# add optimization if not a debug build
# (make DEBUG=true" will make a non-optimized build)
ifndef DEBUG
CXXFLAGS+= -O3 -DNDEBUG
CFLAGS+= -O3 -DNDEBUG
endif

.PHONEY: $(PACKAGE_LIBNAME)
all: $(PACKAGE_LIBNAME)

# target to ensure third-party libraries are build
.PHONY: tparty
tparty: 
	$(MAKE) -C $(THIRD_PARTY)

# Main target that builds the package library
$(PACKAGE_LIBNAME): tparty $(BUILD_FILES) 
	mkdir -p lib
	$(CXX) -o $@ $(BUILD_FILES) $(LDFLAGS) 

# rule to make build/XXX.so from src/XXX.so
build/%.o: src/%.cpp tparty 
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

# rule to compile symbols from the vertica SDK:
build/Vertica.o: $(VERTICA_SDK_INCLUDE)/Vertica.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(VERTICA_SDK_INCLUDE)/Vertica.cpp -o $@


# Targets to install and uninstall the library and functions
install: ddl/install.sql
	$(VSQL) -f ddl/install.sql
uninstall: ddl/uninstall.sql
	$(VSQL) -f ddl/uninstall.sql

# run examples
test:
	$(VSQL) -f examples/apache_parser.sql
	$(VSQL) -f examples/url_extractor.sql
	$(VSQL) -f examples/w3cLogParser.sql


clean:
	$(MAKE) -C $(THIRD_PARTY) clean
	rm -rf build
	rm -rf lib
	rm -f vsim*

##############
# Advanced upsage: use simulator to debug and test
##############

DEBUG_DATA_PATH = $(shell pwd)/test-data

# Run the simulator against the functons
# Run the simulator against the functons
sim_test: all simulator
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) ApacheParserFactory $(DEBUG_DATA_PATH)/apache.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) UriExtractorFactory $(DEBUG_DATA_PATH)/urls.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) w3cLogParserFactory $(DEBUG_DATA_PATH)/iis.txt

# build the simulator (in SIMULATOR_PATH) and simlink it here
simulator:
	$(MAKE) -C $(SIMULATOR_PATH)
	ln -f -s $(SIMULATOR_PATH)/vsim 

