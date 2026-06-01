###############################################################################
# Destination directories
###############################################################################
prefix=/usr/local
exec_prefix=$(prefix)
BINDIR=$(prefix)/bin

INSTALL=/usr/bin/install -c
INSTALL_PROGRAM=${INSTALL}


CC=gcc

OBJ=sp.o spdisp.o inice.o
PWOBJ=../pw/specpr.o usleep.o

LDFLAGS=-L../lib  -L/opt/X11/lib -L/opt/X11/lib
LIBS=-lXfred -lX11 -lm

CFLAGS=-std=gnu89 -Wno-return-type -Wno-implicit-function-declaration -Wno-incompatible-function-pointer-types -Wno-int-conversion -I.. -I../pw -I../lib -I/opt/X11/include

sp:	 $(OBJ) $(PWOBJ)
	$(CC) $(CFLAGS) $(OBJ) $(PWOBJ) -o $@ $(LDFLAGS) $(LIBS)

install:	sp
	$(INSTALL_PROGRAM) -d $(BINDIR) -m "u=rwx,go=rx" -c sp

clean:
		-rm -f $(OBJ) sp
