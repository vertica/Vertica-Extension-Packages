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
VERTICA_SDK_INCLUDE = $(SDK)/include
SIMULATOR_PATH = $(SDK)/simulator

BUILD_PATH = $(shell pwd)
THIRD_PARTY = $(shell pwd)/third-party
THIRD_PARTY_INSTALL = $(THIRD_PARTY)/install
THIRD_PARTY_LIB = $(THIRD_PARTY_INSTALL)/lib
THIRD_PARTY_INCLUDE = $(THIRD_PARTY_INSTALL)/include
DEBUG_DATA_PATH = $(shell pwd)/test-data

PACKAGE_FUNCTIONS = WebPackage.so
PACKAGE_FUNCTIONS_TARGET = $(BUILD_PATH)/../lib/$(PACKAGE_FUNCTIONS)

CXX=g++
CXXFLAGS=  -D HAVE_LONG_LONG_INT_64  -I$(VERTICA_SDK_INCLUDE) -I$(THIRD_PARTY_INCLUDE) -g -Wall -Wno-unused-value -shared -fPIC -std=c++0x -O3 -DNDEBUG   

LDFLAGS=-L $(THIRD_PARTY_LIB) -Wl,-Bstatic -luriparser -Wl,-Bdynamic




#for third party libs 
CXXINCFLAGS =  
CXXLIBFLAGS = 

export PATH := $(THIRD_PARTY_LIB):$(PATH)

.PHONEY: $(PACKAGE_FUNCTIONS_TARGET)

all: third_party $(PACKAGE_FUNCTIONS_TARGET)

#build third-party libraries
third_party:
	 $(MAKE) -C $(THIRD_PARTY)

# build package functions
$(PACKAGE_FUNCTIONS_TARGET): src/*.cpp
	$(CXX) $(CXXFLAGS) $(CXXINCFLAGS) $(CXXLIBFLAGS) -o $@ src/*.cpp $(VERTICA_SDK_INCLUDE)/Vertica.cpp $(LDFLAGS)

# Run the simulator against the functons
sim_test: all simulator
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) ApacheParserFactory $(DEBUG_DATA_PATH)/apache.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) UriExtractorFactory $(DEBUG_DATA_PATH)/urls.txt

# build the simulator (in SIMULATOR_PATH) and simlink it here
simulator:
	$(MAKE) -C $(SIMULATOR_PATH)
	ln -f -s $(SIMULATOR_PATH)/vsim 

clean:
	$(MAKE) -C $(THIRD_PARTY) clean
	rm -f $(PACKAGE_FUNCTIONS_TARGET)
	rm -f ./*.so
	rm -f ./src/*.o
	rm -f vsim*

