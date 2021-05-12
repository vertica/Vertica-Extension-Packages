all: jsoncpp

ICU_PLATFORM = Linux
# don't allow parallel make settings to be inherited
MAKE := make -j1
TAR = tar
AR = ar
PATCH = patch
YACC = yacc
RANLIB = ranlib
STRIP = strip
UNZIP = unzip

THIRD_PARTY = $(shell pwd)
INSTALL_DIR = $(THIRD_PARTY)/../../build

JSONCPP_DIR = $(THIRD_PARTY)
JSONCPP_TGZ = $(JSONCPP_DIR)/jsoncpp-src-0.5.0.tar.gz
JSONCPP_SRC = $(JSONCPP_DIR)/jsoncpp-src-0.5.0
SCONS_TGZ = $(JSONCPP_DIR)/scons-local-2.3.1.tar.gz

.PHONY: jsoncpp
jsoncpp:    jsoncpp-src-0.5.0.tar.gz scons $(JSONCPP_DIR)/.done

# force PIC code in static libraries
CFLAGS+=-fPIC
export CFLAGS

$(JSONCPP_DIR)/.done:
	@mkdir -p $(INSTALL_DIR)
	@echo "Unpacking, configuring and installing JSONCPP...";
	(cd $(JSONCPP_DIR); $(TAR) xzf $(JSONCPP_TGZ); )
	(cd $(JSONCPP_SRC); $(TAR) xzf $(SCONS_TGZ); )
#add -fPIC
	(cd $(JSONCPP_SRC); sed -i 's/CCFLAGS = "-Wall"/CCFLAGS = "-Wall -fPIC"/g;' SConstruct )
	(cd $(JSONCPP_SRC); python ./scons.py platform=linux-gcc )
	touch $@

# download jsoncpp from sourceforge
jsoncpp-src-0.5.0.tar.gz:
	@echo "Downloading jsoncpp from sourceforge..."
	rm -f jsoncpp-src-0.5.0.tar.gz
	wget http://downloads.sourceforge.net/project/jsoncpp/jsoncpp/0.5.0/jsoncpp-src-0.5.0.tar.gz

scons:
	@echo "Downloading SCons from sourceforge..."
	rm -f scons-local-2.3.1.tar.gz
	wget http://downloads.sourceforge.net/project/scons/scons-local/2.3.1/scons-local-2.3.1.tar.gz

clean: 
	rm -f jsoncpp-src-0.5.0.tar.gz scons-local-2.3.1.tar.gz
	rm -f $(JSONCPP_DIR)/.done
	rm -rf $(JSONCPP_SRC)
	rm -rf $(INSTALL_DIR)
