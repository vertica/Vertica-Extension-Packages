
TMPDIR ?= /tmp
SDK_HOME ?= /opt/vertica/sdk
SHELL = /bin/bash
VSQL ?= /opt/vertica/bin/vsql

CXXFLAGS := $(CXXFLAGS) -I $(SDK_HOME)/include -I ../../HelperLibraries/ -I $(SDK_HOME)/examples/HelperLibraries/ -O0 -g -fPIC -shared

build: build/ExternalFilter.so build/ExternalSource.so
## See targets below for actual build logic

clean:
	rm -rf build

install: build
	$(VSQL) -c "CREATE LIBRARY ExternalFilterLib AS '`pwd`/build/ExternalFilter.so';"
	$(VSQL) -c "CREATE FILTER ExternalFilter AS LANGUAGE 'C++' NAME 'ExternalFilterFactory' LIBRARY ExternalFilterLib;"
	$(VSQL) -c "CREATE LIBRARY ExternalSourceLib AS '`pwd`/build/ExternalSource.so';"
	$(VSQL) -c "CREATE SOURCE ExternalSource AS LANGUAGE 'C++' NAME 'ExternalSourceFactory' LIBRARY ExternalSourceLib;"

uninstall:
	if $(VSQL) -c "SELECT lib_name FROM user_libraries;" -t | grep -q ExternalFilterLib; then $(VSQL) -c "DROP LIBRARY ExternalFilterLib CASCADE;"; else echo "ExternalFilterLib not installed."; fi
	if $(VSQL) -c "SELECT lib_name FROM user_libraries;" -t | grep -q ExternalSourceLib; then $(VSQL) -c "DROP LIBRARY ExternalSourceLib CASCADE;"; else echo "ExternalSourceLib not installed."; fi

test:
	@$(MAKE) uninstall
	@$(MAKE) install
	diff -u <(TMPDIR=$(TMPDIR) $(VSQL) -f examples/Tests.ExternalFilter.sql 2>&1       | perl -pe 's/(?<=\.cpp|\.sql):\d+/:x/g') examples/Tests.ExternalFilter.out
	diff -u <(TMPDIR=$(TMPDIR) $(VSQL) -f examples/Tests.ExternalFilterErrors.sql 2>&1 | perl -pe 's/(?<=\.cpp|\.sql):\d+/:x/g') examples/Tests.ExternalFilterErrors.out
	diff -u <(TMPDIR=$(TMPDIR) $(VSQL) -f examples/Tests.ExternalSource.sql 2>&1       | perl -pe 's/(?<=\.cpp|\.sql):\d+/:x/g') examples/Tests.ExternalSource.out
	diff -u <(TMPDIR=$(TMPDIR) $(VSQL) -f examples/Tests.ExternalSourceErrors.sql 2>&1 | perl -pe 's/(?<=\.cpp|\.sql):\d+/:x/g') examples/Tests.ExternalSourceErrors.out
	@$(MAKE) uninstall
	@echo "*** Test passed! ***"

.PHONY: build clean install uninstall test


## Actual build targets

build/ExternalFilter.so: ExternalFilter.cpp $(SDK_HOME)/include/Vertica.cpp  $(SDK_HOME)/include/BuildInfo.h $(SDK_HOME)/examples/HelperLibraries/LoadArgParsers.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(SDK_HOME)/include/Vertica.cpp ExternalFilter.cpp ProcessLaunchingPlugin.cpp popen3.c

build/ExternalSource.so: ExternalSource.cpp $(SDK_HOME)/include/Vertica.cpp  $(SDK_HOME)/include/BuildInfo.h $(SDK_HOME)/examples/HelperLibraries/LoadArgParsers.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(SDK_HOME)/include/Vertica.cpp ExternalSource.cpp ProcessLaunchingPlugin.cpp popen3.c
