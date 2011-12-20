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

THIRD_PARTY = $(shell pwd)/src/third-party
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include

# Add in your source files below
BUILD_FILES      = build/Vertica.o \
		   $(addprefix build/,SMS.o)

# Define the .so name here (and update the references in ddl/install.sql and ddl/uninstall.sql)
PACKAGE_LIBNAME   = lib/SMS.so
CXX=g++
CXXFLAGS=-g -D HAVE_LONG_LONG_INT_64 -c -I ../include -Wall -Wno-unused-value -fPIC -I $(VERTICA_SDK_INCLUDE) -I $(THIRD_PARTY_INCLUDE)
LDFLAGS=-shared -lcrypt -lnsl -lutil -lresolv -lcurl

# add optimization if not a debug build
# (make DEBUG=true" will make a non-optimized build)
ifndef DEBUG
CXXFLAGS+= -O3 -DNDEBUG
CFLAGS+= -O3 -DNDEBUG
endif

.PHONEY: $(PACKAGE_LIBNAME)
all: $(PACKAGE_LIBNAME)

# Main target that builds the package library
$(PACKAGE_LIBNAME): $(BUILD_FILES) 
	mkdir -p lib
	$(CXX) $(LDFLAGS) -o $@ $(BUILD_FILES) 

# rule to make build/XXX.so from src/XXX.so
build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@

# rule to compile symbols from the vertica SDK:
build/Vertica.o: $(VERTICA_SDK_INCLUDE)/Vertica.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(VERTICA_SDK_INCLUDE)/Vertica.cpp -o $@

# example rule to make build/XX.o from third-party/src/*.c
#build/%.o: $(THIRD_PARTY)/src/%.c
#	@mkdir -p build
#	$(CXX) $(CXXFLAGS) $< -o $@ 


# Targets to install and uninstall the library and functions
install: $(PACKAGE_LIBNAME) ddl/install.sql
	$(VSQL) -f ddl/install.sql
uninstall: ddl/uninstall.sql
	$(VSQL) -f ddl/uninstall.sql

# run examples
test:
	$(VSQL) -f examples/SMS.sql

clean:
	rm -rf build
	rm -rf lib
	rm -f vsim*
