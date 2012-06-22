
TMPDIR ?= /tmp
SDK_HOME ?= /opt/vertica/sdk
SHELL = /bin/bash
VSQL ?= /opt/vertica/bin/vsql

CXXFLAGS := $(CXXFLAGS) -I $(SDK_HOME)/include -I ../../HelperLibraries/ -I $(SDK_HOME)/examples/HelperLibraries/ -O0 -g -fPIC -shared

build: build/ExternalFilter.so build/ExternalSource.so
## See targets below for actual build logic

clean:
	rm build/ExternalFilter.so build/ExternalSource.so

install: build
	$(VSQL) -c "CREATE LIBRARY ExternalFilterLib AS '`pwd`/build/ExternalFilter.so';"
	$(VSQL) -c "CREATE FILTER ExternalFilter AS LANGUAGE 'C++' NAME 'ExternalFilterFactory' LIBRARY ExternalFilterLib;"
	$(VSQL) -c "CREATE LIBRARY ExternalSourceLib AS '`pwd`/build/ExternalSource.so';"
	$(VSQL) -c "CREATE SOURCE ExternalSource AS LANGUAGE 'C++' NAME 'ExternalSourceFactory' LIBRARY ExternalSourceLib;"

uninstall:
	$(VSQL) -c "DROP LIBRARY ExternalFilterLib CASCADE;"
	$(VSQL) -c "DROP LIBRARY ExternalSourceLib CASCADE;"

test:
	@$(MAKE) install
	diff -u <($(VSQL) -f examples/Tests.ExternalFilter.sql) examples/Tests.ExternalFilter.out
	diff -u <($(VSQL) -f examples/Tests.ExternalSource.sql) examples/Tests.ExternalSource.out
	@$(MAKE) uninstall
	@echo "*** Test passed! ***"

.PHONY: build clean install uninstall test


## Actual build targets

build/ExternalFilter.so: ExternalFilter.cpp $(SDK_HOME)/include/Vertica.cpp  $(SDK_HOME)/include/BuildInfo.h $(SDK_HOME)/examples/HelperLibraries/LoadArgParsers.h popen3.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(SDK_HOME)/include/Vertica.cpp ExternalFilter.cpp

build/ExternalSource.so: ExternalSource.cpp $(SDK_HOME)/include/Vertica.cpp  $(SDK_HOME)/include/BuildInfo.h $(SDK_HOME)/examples/HelperLibraries/LoadArgParsers.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(SDK_HOME)/include/Vertica.cpp ExternalSource.cpp
