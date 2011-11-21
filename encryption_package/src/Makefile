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

#BUILD_PATH = $(shell pwd)
BUILD_PATH = .
THIRD_PARTY = $(shell pwd)/third-party
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include
THIRD_PARTY_OBJS = rijndael.o my_aes.o
DEBUG_DATA_PATH = $(shell pwd)/test-data

THIRD_PARTY_OBJ = $(THIRD_PARTY)/src/rijndael.o $(THIRD_PARTY)/src/my_aes.o
BUILD_FILES = functions/AESEncrypt.o functions/AESDecrypt.o functions/Vertica.o

PACKAGE_FUNCTIONS = Encryption.so
PACKAGE_FUNCTIONS_LIB = $(BUILD_PATH)/build
PACKAGE_FUNCTIONS_TARGET = $(PACKAGE_FUNCTIONS_LIB)/$(PACKAGE_FUNCTIONS)

CXX=g++
CXXFLAGS=-D HAVE_LONG_LONG_INT_64 -c -I ../include -Wall -Wno-unused-value -fPIC -I $(VERTICA_SDK_INCLUDE) -I $(THIRD_PARTY_INCLUDE)
LDFLAGS=-shared
VSQL=vsql

export PATH := $(THIRD_PARTY_LIB):$(PATH)

.PHONEY: $(PACKAGE_FUNCTIONS_TARGET)

all: $(PACKAGE_FUNCTIONS_TARGET)

# build package functions
$(PACKAGE_FUNCTIONS_TARGET): $(BUILD_FILES) $(THIRD_PARTY_OBJ)
	mkdir -p $(PACKAGE_FUNCTIONS_LIB)
	$(CXX) $(LDFLAGS) -o $@ $(BUILD_FILES) $(THIRD_PARTY_OBJ)

functions/Vertica.o:	$(VERTICA_SDK_INCLUDE)/Vertica.cpp
	$(CXX) $(CXXFLAGS) $(VERTICA_SDK_INCLUDE)/Vertica.cpp -o $@
functions/AESEncrypt.o: functions/AESEncrypt.cpp
	$(CXX) $(CXXFLAGS) functions/AESEncrypt.cpp -o $@
functions/AESDecrypt.o: functions/AESDecrypt.cpp
	$(CXX) $(CXXFLAGS) functions/AESDecrypt.cpp -o $@
$(THIRD_PARTY)/src/rijndael.o: $(THIRD_PARTY)/src/rijndael.c
	$(CXX) $(CXXFLAGS) $(THIRD_PARTY)/src/rijndael.c -o $@ 
$(THIRD_PARTY)/src/my_aes.o: $(THIRD_PARTY)/src/my_aes.c
	$(CXX) $(CXXFLAGS) $(THIRD_PARTY)/src/my_aes.c -o $@ 

install: $(PACKAGE_FUNCTIONS_TARGET) ddl/install.sql
	$(VSQL) -f ddl/install.sql
uninstall: ddl/uninstall.sql
	$(VSQL) -f ddl/uninstall.sql
	

# Run the simulator against the functons
sim_test: all simulator
	$(VALGRIND) ./vsim $(PACKAGE_FUNCTIONS_TARGET) FunctionName $(DEBUG_DATA_PATH)/testdata.txt

# build the simulator (in SIMULATOR_PATH) and simlink it here
simulator:
	$(MAKE) -C $(SIMULATOR_PATH)
	ln -f -s $(SIMULATOR_PATH)/vsim 

clean:
	rm -rf $(PACKAGE_FUNCTIONS_TARGET)
	rm -f ./*.so
	rm -f ./functions/*.o
	rm -f vsim*
	rm -f third-party/src/*.o

