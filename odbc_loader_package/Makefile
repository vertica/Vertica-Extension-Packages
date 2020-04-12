
TMPDIR ?= /tmp
SDK_HOME ?= /opt/vertica/sdk
SHELL = /bin/bash
VSQL ?= /opt/vertica/bin/vsql

ALL_CXXFLAGS := $(CXXFLAGS) -I $(SDK_HOME)/include -I $(SDK_HOME)/examples/HelperLibraries -fPIC -shared -Wall -g -std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0

build: build/ODBCLoader.so
## See targets below for actual build logic

clean:
	rm build/ODBCLoader.so

install: build
	$(VSQL) -f ddl/install.sql

uninstall:
	$(VSQL) -f ddl/uninstall.sql

test:
	## Try uninstalling first, just in case we have a stale version around
	-@$(MAKE) uninstall
	@$(MAKE) install
	-diff -u Tests.out <($(VSQL) -f examples/sample_usage.sql 2>&1)
	@$(MAKE) uninstall

.PHONY: build clean install uninstall test


## Actual build target
build/ODBCLoader.so: ODBCLoader.cpp $(SDK_HOME)/include/Vertica.cpp $(SDK_HOME)/include/BuildInfo.h
	mkdir -p build
	$(CXX) $(ALL_CXXFLAGS) -o $@ $(SDK_HOME)/include/Vertica.cpp ODBCLoader.cpp -lodbc
