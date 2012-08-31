

VSQL?=/opt/vertica/bin/vsql
CPP=g++
INCLUDE=/opt/vertica/sdk/include/
LOCATION=$(shell pwd)

all: build/myudx.so
.PHONEY: install	

build/myudx.so: /opt/vertica/sdk/include/Vertica.cpp src/Email.cpp
	$(CPP) -D HAVE_LONG_LONG_INT_64 -g -Wall -Wno-unused-value -shared -fPIC -I $(INCLUDE) -o $@ /opt/vertica/sdk/include/Vertica.cpp src/Email.cpp


install: build/myudx.so
	$(VSQL) -c "CREATE LIBRARY EMAILLib as  '$(LOCATION)/build/myudx.so';"
	$(VSQL) -c "CREATE FUNCTION EMAIL as language 'C++' name 'EMAILFactory' library EMAILLib;"
	$(shell mkdir .hidden)
	$(shell touch .hidden/install)

uninstall: .hidden/install
	$(VSQL) -c "DROP LIBRARY EMAILLib cascade;"
	$(shell rm -rf .hidden)

clean:
	rm build/myudx.so
	
