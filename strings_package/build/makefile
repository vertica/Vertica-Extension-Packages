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

PACKAGE_FUNCTIONS = StringsPackage.so
PACKAGE_FUNCTIONS_LIB = $(BUILD_PATH)/../lib
PACKAGE_FUNCTIONS_TARGET = $(PACKAGE_FUNCTIONS_LIB)/$(PACKAGE_FUNCTIONS)

CXX=g++
CXXFLAGS= -Wl,-rpath $(THIRD_PARTY_LIB) -D HAVE_LONG_LONG_INT_64  -I$(VERTICA_SDK_INCLUDE) -I$(THIRD_PARTY_INCLUDE) -Wall -Wno-unused-value -shared -fPIC -std=c++0x 

# if DEBUG is defined, don't add optimization 
ifdef DEBUG
CXXFLAGS+=-g
else
CXXFLAGS+=-O3 -DNDEBUG
endif


#for third party libs 
CXXINCFLAGS =  
CXXLIBFLAGS = 

export PATH := $(THIRD_PARTY_LIB):$(PATH)

.PHONEY: $(PACKAGE_FUNCTIONS_TARGET)

all: $(PACKAGE_FUNCTIONS_TARGET)


# build package functions
$(PACKAGE_FUNCTIONS_TARGET): src/*.cpp 
	(cd $(THIRD_PARTY); $(MAKE) all ) # build third-party libraries
	mkdir -p $(PACKAGE_FUNCTIONS_LIB)
	$(CXX) $(CXXFLAGS) $(CXXINCFLAGS) $(CXXLIBFLAGS) -o $@ src/*.cpp $(VERTICA_SDK_INCLUDE)/Vertica.cpp

# Run the simulator against the functons
sim_test: all simulator
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) EditDistanceFactory $(DEBUG_DATA_PATH)/words.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) StringTokenizerFactory $(DEBUG_DATA_PATH)/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) StringTokenizerDelimFactory $(DEBUG_DATA_PATH)/strings-delim.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) PorterStemmerFactory $(DEBUG_DATA_PATH)/words-porter.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) TwoGramsFactory $(DEBUG_DATA_PATH)/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) ThreeGramsFactory $(DEBUG_DATA_PATH)/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) FourGramsFactory $(DEBUG_DATA_PATH)/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) FiveGramsFactory $(DEBUG_DATA_PATH)/strings.txt

# build the simulator (in SIMULATOR_PATH) and simlink it here
simulator:
	$(MAKE) -C $(SIMULATOR_PATH)
	ln -f -s $(SIMULATOR_PATH)/vsim 

clean:
	rm -f $(PACKAGE_FUNCTIONS_TARGET)
	rm -f ./*.so
	rm -f ./src/*.o
	rm -f vsim*

