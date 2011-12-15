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

THIRD_PARTY = $(shell pwd)/src/third-party
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include

# Add in your source files below
BUILD_FILES      = build/Vertica.o \
		   $(addprefix build/, EditDist.o PorterStemmer.o StringTokenizerDelim.o NGrams.o StringTokenizer.o WordCount.o AnagramLib.o GroupConcat.o)

# Define the .so name here (and update the references in ddl/install.sql and ddl/uninstall.sql)
PACKAGE_LIBNAME   = lib/StringsLib.so

CXX=g++
CXXFLAGS=-g -D HAVE_LONG_LONG_INT_64 -c -I ../include -Wall -Wno-unused-value -fPIC -I $(VERTICA_SDK_INCLUDE) -I $(THIRD_PARTY_INCLUDE)
LDFLAGS=-shared

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
	$(VSQL) -f examples/removespace.sql
	$(VSQL) -f examples/edit_distance.sql
	$(VSQL) -f examples/stemmer.sql
	$(VSQL) -f examples/string_tokenizer.sql
	$(VSQL) -f examples/ngrams.sql
	$(VSQL) -f examples/string_delim_tokenizer.sql
	$(VSQL) -f examples/word_count.sql
	$(VSQL) -f examples/gen_anagram.sql

clean:
	rm -rf build
	rm -rf lib
	rm -f vsim*
	rm -f test-data/wordlist_header.txt
##############
# Advanced upsage: use simulator to debug and test
##############

# rule to make anagram data file with appropriate header for simulator
test-data/wordlist_header.txt:
	echo "word:string" > test-data/wordlist_header.txt
	head test-data/wordlist.txt >> test-data/wordlist_header.txt

# Run the simulator against the functons
sim_test: all simulator test-data/wordlist_header.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) EditDistanceFactory         test-data/words.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) StringTokenizerFactory      test-data/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) StringTokenizerDelimFactory test-data/strings-delim.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) PorterStemmerFactory        test-data/words-porter.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) TwoGramsFactory             test-data/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) ThreeGramsFactory           test-data/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) FourGramsFactory            test-data/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) FiveGramsFactory            test-data/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) WordCountFactory            test-data/strings.txt
	$(VALGRIND) ./vsim $(PACKAGE_LIBNAME) AnagramFactory              test-data/wordlist_header.txt

# build the simulator (in SIMULATOR_PATH) and simlink it here
simulator:
	$(MAKE) -C $(SIMULATOR_PATH)
	ln -f -s $(SIMULATOR_PATH)/vsim 

