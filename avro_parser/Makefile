############################
# Vertica Analytic Database
#
# Makefile to build package directory
#
# Copyright 2011 Vertica Systems, an HP Company
############################

SDK?=/opt/vertica/sdk
VSQL?=vsql

VERSION ?= 0.1.0
VERTICA_SDK_INCLUDE = $(SDK)/include
VERTICA_SDK_HELPER = $(SDK)/examples/HelperLibraries

THIRD_PARTY         = $(shell pwd)/tparty
THIRD_PARTY_INCLUDE = build/include
THIRD_PARTY_LIB     = build/lib

# Add in your source files below
BUILD_FILES      = build/Vertica.o \
		   $(addprefix build/,CRStream.o CRReader.o AvroParser.o version.o)

# Define the .so name here (and update the references in ddl/install.sql and ddl/uninstall.sql)
PACKAGE_LIBNAME   = AvroParser.so
INSTALL_DIR ?= lib/

CXX=g++
CXXFLAGS=-c -I ../include -Wall -Wno-unused-value -shared -fno-strict-aliasing -fPIC -I $(VERTICA_SDK_INCLUDE) -I $(THIRD_PARTY_INCLUDE) -I $(THIRD_PARTY_INCLUDE)/avro -I $(VERTICA_SDK_HELPER)
LDFLAGS=-shared -L $(THIRD_PARTY_LIB) -lavrocpp_s

# add optimization if not a debug build
# (make DEBUG=true" will make a non-optimized build)
ifndef DEBUG
endif

export PATH := $(PATH):$(PWD)/build/bin

build : $(PACKAGE_LIBNAME) 

install: build
	$(VSQL) -c "CREATE LIBRARY AvroParserLib AS '`pwd`/lib/AvroParser.so';"
	$(VSQL) -c "CREATE PARSER AvroParser AS LANGUAGE 'C++' NAME 'AvroParserFactory' LIBRARY AvroParserLib;"

uninstall:
	$(VSQL) -c "DROP LIBRARY AvroParserLib CASCADE;"

test:
	$(VSQL) -f examples/example.sql > examples/tmp.out;diff -u examples/tmp.out examples/example.out

all: $(PACKAGE_LIBNAME)

# target to ensure third-party libraries are build
.PHONY: tparty install uninstall test build/version.c package

package: $(PACKAGE_LIBNAME)
	rsync -az ddl lib/ --exclude=\*.svn*
	rsync -az README.txt lib/ --exclude=\*.svn*
	cd lib; zip -rq avro-vertica.zip $(PACKAGE_LIBNAME) ddl/ README.txt

tparty: 
	$(MAKE) -C $(THIRD_PARTY)

# Main target that builds the package library
$(PACKAGE_LIBNAME): tparty $(BUILD_FILES) 
	mkdir -p lib
	$(CXX) -o $(INSTALL_DIR)/$@ $(BUILD_FILES) $(LDFLAGS) 

# rule to make build/XXX.so from src/XXX.so
build/%.o: src/%.cc tparty 
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

# rule to compile symbols from the vertica SDK:
build/Vertica.o: $(VERTICA_SDK_INCLUDE)/Vertica.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(VERTICA_SDK_INCLUDE)/Vertica.cpp -o $@

build/version.o: build/version.c
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

##
## on every build, record the working copy revision string
##
build/version.c: 
	echo -n 'namespace VAvro { const char* Version = "Version: ' > build/version.c
	echo -n '$(VERSION)'            >> build/version.c
	echo -n ' SVN Revision: '         >> build/version.c
	svnversion -n .                   >> build/version.c
	echo -n ' Date: '                 >> build/version.c
	date | tr '\n' ' '                 >> build/version.c
	echo '";}'      >> build/version.c

clean:
	$(MAKE) -C $(THIRD_PARTY) clean
	rm -rf build
	rm -rf lib
