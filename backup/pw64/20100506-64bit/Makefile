###############################################################################
# Destination directories
###############################################################################
prefix=/usr/local
exec_prefix=$(prefix)
BINDIR=$(prefix)/bin
LIBDIR=$(prefix)/lib
mandir=$(prefix)/man/cat1

INSTALL=/usr/bin/install -c
INSTALL_PROGRAM=${INSTALL}
INSTALL_DATA=${INSTALL} -m 644

X_INCLUDES=
X_LIBS=

DEFS=-DHAVE_CONFIG_H
CPPFLAGS= $(DEFS) $(X_INCLUDES)
CFLAGS=-g -O2 -I/usr/include/X11
LIBS=-lnsl -lXm -lXext -lXt -lX11 -lm -lz  $(X_LIBS) -lXfred -lX11 -lm

INSTALL_WHAT=install_pw_only

###############################################################################
#
# You shouldn't have to change anything below here.
#
###############################################################################

PWDIR=../pw

all:	pw_src sp_source

pw_src:	lib_source iomedley_src
	@cd pw && $(MAKE)

lib_source:
	@cd lib && $(MAKE)

iomedley_src:
	@cd iomedley && $(MAKE)

sp_source: pw_src
	@cd sp && $(MAKE)

# The following applies when specpr reader is external to pw
install_both:	pw_src sp_source
	@cd pw && $(MAKE) install
	@cd sp && $(MAKE) install
	
# The following applies when we have a built-in specpr reader
install_pw_only:	pw_src
	@cd pw && $(MAKE) install

install:	$(INSTALL_WHAT)
	@# cd pw && $(MAKE) install
	@# cd sp && $(MAKE) install
	@# $(INSTALL_PROGRAM) -d $(BINDIR) -m "u=rwx,go=rx" -c pw/pw
	@# $(INSTALL_PROGRAM) -d $(BINDIR) -m "u=rwx,go=rx" -c sp/sp
	@# nroff manual/pw.manual > $(mandir)/pw.1

dist:	clean
	echo pw-`head -1 pw/version.h | sed -e 's/.*#//;s/".*//'` > .ver
	rm -rf `cat .ver`
	mkdir `cat .ver` 
	(cd `cat .ver` && mkdir pw lib sp iomedley && ln -s ../bitmaps .)
	ln README Makefile `cat .ver`
	ln pw/*.[ch] pw/Makefile 	 `cat .ver`/pw
	ln lib/*.[ch] lib/Makefile 	 `cat .ver`/lib
	ln iomedley/*.[ch] iomedley/Makefile 	 `cat .ver`/iomedley
	ln sp/*.[ch] sp/Makefile	 `cat .ver`/sp
	tar cohf `cat .ver`.tar `cat .ver`
	compress `cat .ver`.tar
	rm -rf `cat .ver`

move_dist:  dist
	compress -c pw.tar > pw-`head -1 pw/version.h | sed -e 's/.*#//;s/".*//'`.tar.Z

mirror:
	-mkdir lib pw sp
	(cd lib && ln -s ../$(PWDIR)/lib/*.[ch] ../$(PWDIR)/lib/Makefile .)
	(cd pw && ln -s ../$(PWDIR)/pw/*.[ch] ../$(PWDIR)/pw/Makefile .)
	(cd sp && ln -s ../$(PWDIR)/sp/*.[ch] ../$(PWDIR)/sp/Makefile .)
	-ln -s $(PWDIR)/bitmaps .
	-cp $(PWDIR)/Makefile .

clean:
	cd lib && $(MAKE) clean
	cd iomedley && $(MAKE) clean
	cd pw && $(MAKE) clean
	cd sp && $(MAKE) clean
