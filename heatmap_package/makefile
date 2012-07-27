############################
# Vertica Analytic Database
#
# Makefile to build example user defined functions
#
# To run under valgrind:
#   make RUN_VALGRIND=1 run
#
# Copyright (c) 2005 - 2012 Vertica, an HP company
############################

## Set to the location of the SDK installation
SDK_HOME?=/opt/vertica/sdk/
SDK?=/opt/vertica/sdk/
VSQL?=vsql

VERTICA_SDK_INCLUDE = $(SDK)/include
SIMULATOR_PATH = $(SDK)/simulator

CXX=g++
CXXFLAGS=-I $(SDK_HOME)/include -I HelperLibraries -g -Wall -Wno-unused-value -shared -fPIC 

ifdef OPTIMIZE
## UDLs should be compiled with compiler optimizations in release builds
CXXFLAGS=$(CXXFLAGS) -O3 -ansi -pedantic -Wall -Wextra
endif

## Set to the desired destination directory for .so output files
BUILD_DIR?=lib

## Set to a valid temporary directory
TMPDIR?=/tmp

## Set to the path to 
BOOST_INCLUDE ?= /usr/include
CURL_INCLUDE ?= /usr/include
ZLIB_INCLUDE ?= /usr/include
BZIP_INCLUDE ?= /usr/include

.PHONEY: $(PACKAGE_LIBNAME)
all: $(PACKAGE_LIBNAME)

all: HeatMapLib

$(BUILD_DIR)/.exists:
	test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
	touch $(BUILD_DIR)/.exists

###
# Aggregate Functions
###
HeatMapLib: $(BUILD_DIR)/HeatMapLib.so

$(BUILD_DIR)/HeatMapLib.so: HeatMapLib/*.cpp $(SDK_HOME)/include/Vertica.cpp $(SDK_HOME)/include/BuildInfo.h $(BUILD_DIR)/.exists
	$(CXX) $(CXXFLAGS) -o $@ HeatMapLib/*.cpp $(SDK_HOME)/include/Vertica.cpp

# Targets to install and uninstall the library and functions
install: $(PACKAGE_LIBNAME) ddl/install.sql
	$(VSQL) -f ddl/install.sql
uninstall: ddl/uninstall.sql
	$(VSQL) -f ddl/uninstall.sql

# run examples
test:
	python HeatMapLib/RandGenerator.py 1 1000000 -100 100 -100 100 > examples/data.data
	$(VSQL) -f examples/HeatMapLib.sql


clean:
	rm -f examples/data.data
	rm -f $(BUILD_DIR)/*.so
	rm -f $(BUILD_DIR)/.exists
	rmdir $(BUILD_DIR) >/dev/null 2>&1 || true
