############################
# Vertica Analytic Database
#
# Makefile to build package directory
#
# Copyright 2011 Vertica Systems, an HP Company
#
#
# Portions of this software Copyright (c) 2011 by Vertica, an HP
# Company.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#   - Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#   - Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
############################

SDK?=/opt/vertica/sdk
VSQL?=/opt/vertica/bin/vsql -U dbadmin

VERTICA_SDK_INCLUDE = $(SDK)/include
SIMULATOR_PATH = $(SDK)/simulator

THIRD_PARTY_INCLUDE = build/include
THIRD_PARTY_LIB     = build/lib

# Add in your source files below
BUILD_FILES      = build/Vertica.o \
		   $(addprefix build/,GoogleAnalytics.o VerticaProperties.o Curl.o XMLFunctions.o Google.o JulianDate.o)

# Define the .so name here (and update the references in ddl/install.sql and ddl/uninstall.sql)
PACKAGE_LIBNAME   = lib/XMLPackage.so

CXX=g++
CXXFLAGS=-g -D HAVE_LONG_LONG_INT_64 -c -I ../include -Wall -Wno-unused-value -fPIC -I $(VERTICA_SDK_INCLUDE) -I $(THIRD_PARTY_INCLUDE) \
	`xml2-config --cflags` `xslt-config --cflags`
LDFLAGS=-shared -L $(THIRD_PARTY_LIB) -Wl,-Bstatic -Wl,-Bstatic -Wl,-Bdynamic `curl-config --libs` \
	`xml2-config --libs` `xslt-config --libs`

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

# run examples
test:
	$(VSQL) -f examples/vertica_properties.sql
	$(VSQL) -f examples/xml.sql
	$(VSQL) -f examples/google_analytics.sql


clean:
	rm -rf build
	rm -rf lib

