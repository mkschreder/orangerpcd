ACLOCAL_AMFLAGS=-I m4
SUBDIRS=src
EXTRA_DIST=autogen.mk

node_modules: node_modules.tar.gz 
	@if [ ! -d node_modules ]; then tar -xzf node_modules.tar.gz; fi

all: node_modules
