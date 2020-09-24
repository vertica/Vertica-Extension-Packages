
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
	# install ODBCLoader
	@$(VSQL) -f ddl/install.sql

uninstall:
	# uninstall ODBCLoader
	@$(VSQL) -f ddl/uninstall.sql

test:
	@# Try uninstalling first, just in case we have a stale version around
	-@$(MAKE) -s uninstall >/dev/null 2>&1
	@$(MAKE) --no-print-dir install
	@# Use bash's "trap" to uninstall and still return an error
	@trap '$(MAKE) --no-print-dir uninstall' EXIT; $(MAKE) --no-print-dir test_impl

test_impl:
	# run tests
	@$(VSQL) -f examples/sample_usage.sql >Tests.actual 2>&1
	@# filter out variable messages (i.e., mariadb vs mysql)
	@diff -u Tests.out <(perl -pe 's/^vsql:[\/_:\w\.]* /vsql: /; \
	              s/\[ODBC[^\]]*\]/[...]/g; \
		      s/\[mysql[^\]]*\]/[...]/g; \
		      s/(Error parsing .* )\(.*\)$$/$$1(...)/; \
		      s/mariadb/MySQL/ig; ' Tests.actual)
	@echo ALL TESTS SUCCESSFUL

.PHONY: build clean install uninstall test


## Actual build target
build/ODBCLoader.so: ODBCLoader.cpp $(SDK_HOME)/include/Vertica.cpp $(SDK_HOME)/include/BuildInfo.h
	mkdir -p build
	$(CXX) $(ALL_CXXFLAGS) -o $@ $(SDK_HOME)/include/Vertica.cpp ODBCLoader.cpp -lodbc
