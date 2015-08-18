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

# Add in your source files below
BUILD_FILES      = build/Vertica.o \
		   $(addprefix build/,NthValue.o)

# Define the .so name here (and update the references in ddl/install.sql and ddl/uninstall.sql)
PACKAGE_LIBNAME   = lib/NthValue.so

CXX=g++
CXXFLAGS=-g -D HAVE_LONG_LONG_INT_64 -c -I ../include -Wall -Wno-unused-value -fPIC -I $(VERTICA_SDK_INCLUDE)
LDFLAGS=-shared -Wl,-Bstatic -Wl,-Bstatic -Wl,-Bdynamic

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
	$(CXX) -o $@ $(BUILD_FILES) $(LDFLAGS)

# rule to make build/XXX.so from src/XXX.so
build/%.o: src/%.cpp
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

clean:
	rm -rf build
	rm -rf lib
