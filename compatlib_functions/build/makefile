############################
# Vertica Analytic Database
#
# Makefile to build example user defined functions that use third party libraries
#
# To run under valgrind:
#   make RUN_VALGRIND=1 run
#
# Copyright 2011 Vertica Systems of Billerica Massachusetts, 2011
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

COMPATLIB_FUNCTIONS = CompatLibFunctions.so
COMPATLIB_FUNCTIONS_TARGET = $(BUILD_PATH)/../lib/$(COMPATLIB_FUNCTIONS)

CXX=g++
CXXFLAGS= -Wl,-rpath $(THIRD_PARTY_LIB) -D HAVE_LONG_LONG_INT_64  -I$(VERTICA_SDK_INCLUDE) -I$(THIRD_PARTY_INCLUDE) -g -Wall -Wno-unused-value -shared -fPIC -std=c++0x -O3 -DNDEBUG
# For debugging:
#CXXFLAGS= -Wl,-rpath $(THIRD_PARTY_LIB) -D HAVE_LONG_LONG_INT_64  -I$(VERTICA_SDK_INCLUDE) -I$(THIRD_PARTY_INCLUDE) -g -Wall -Wno-unused-value -shared -fPIC -std=c++0x
#for third party libs 
CXXINCFLAGS =  
CXXLIBFLAGS = 


export PATH := $(THIRD_PARTY_LIB):$(PATH)

#.PHONEY: third_party $(STANDARD_FUNCTIONS_TARGET)
.PHONEY: $(COMPATLIB_FUNCTIONS_TARGET)

#all: third_party $(STANDARD_FUNCTIONS_TARGET)
all: $(COMPATLIB_FUNCTIONS_TARGET)

#build third-party libraries
third_party:
	(cd $(THIRD_PARTY); $(MAKE) all )

#build hive functions
$(COMPATLIB_FUNCTIONS_TARGET):
	$(CXX) $(CXXFLAGS) $(CXXINCFLAGS) $(CXXLIBFLAGS) -o $@ src/*.cpp $(VERTICA_SDK_INCLUDE)/Vertica.cpp

# Run the simulator against scalar functions and transform functions
sim_test: all simulator
	$(VALGRIND) ./vsim $(COMPATLIB_FUNCTIONS_TARGET) ConnectByFactory $(DEBUG_DATA_PATH)/connect_by_path.txt
	$(VALGRIND) ./vsim $(COMPATLIB_FUNCTIONS_TARGET) TransposeFactory $(DEBUG_DATA_PATH)/transpose.txt

# build the simulator (in SIMULATOR_PATH) and simlink it here
simulator:
	$(MAKE) -C $(SIMULATOR_PATH)
	ln -f -s $(SIMULATOR_PATH)/vsim 

clean:
	rm -f $(COMPATLIB_FUNCTIONS_TARGET)
	rm -f ./*.so
	rm -f ./src/*.o

