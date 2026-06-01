###############################################################################
# Destination directories
###############################################################################
prefix=/usr/local
exec_prefix=$(prefix)
BINDIR=$(prefix)/bin

INSTALL=/usr/bin/install -c
INSTALL_PROGRAM=${INSTALL}


CC=cc

OBJ=sp.o spdisp.o inice.o
PWOBJ=../pw/specpr.o usleep.o

LDFLAGS=-L../lib 
LIBS=-lXfred -lX11 -lm

CFLAGS=-I.. -I../pw -I../lib

sp:	 $(OBJ) $(PWOBJ)
	$(CC) $(CFLAGS) $(OBJ) $(PWOBJ) -o $@ $(LDFLAGS) $(LIBS)

install:	sp
	$(INSTALL_PROGRAM) -d $(BINDIR) -m "u=rwx,go=rx" -c sp

clean:
		-rm -f $(OBJ) sp
