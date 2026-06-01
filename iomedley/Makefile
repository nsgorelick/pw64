prefix=$(DESTDIR)/usr/local

LIBTIFF = libtiff.a
LIBTIFFDIR = libtiff

LIBJPEG = libjpeg.a
LIBJPEGDIR = libjpeg

LIBGIF = libgif.a
LIBGIFDIR = libgif

LIBPNG = libpng.a
LIBPNGOBJ = libpng.a
LIBPNGDIR = libpng-1.2.3

CC=gcc
DEFS=-DHAVE_CONFIG_H
CPPFLAGS=-I. -I$(LIBTIFFDIR)/libtiff -I$(LIBJPEGDIR) -I$(LIBGIFDIR) 
# X_CFLAGS=@X_CFLAGS@
# X_PRE_LIBS=@X_PRE_LIBS@
# X_LIBS=@X_LIBS@
# X_EXTRA_LIBS=@X_EXTRA_LIBS@
CFLAGS=-g -O2 -Ilibpng-1.2.3 $(X_CFLAGS) -g
# LDLIBS= -Llibpng-1.2.3  -lpng $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)
LDLIBS= -Llibpng-1.2.3  -lpng
RANLIB=ranlib

LIBIOMEDLEY = libiomedley.a

OBJS= \
	io_aviris.o \
	io_goes.o \
	io_grd.o \
	io_imath.o \
	io_isis.o \
	io_lablib3.o \
	io_gif.o \
	io_png.o \
	io_jpeg.o \
	io_tiff.o \
	io_pnm.o \
	io_bmp.o \
	io_vicar.o \
	io_ers.o \
	io_raw.o \
	io_envi.o \
	iomedley.o \
	tools.o

all: $(LIBIOMEDLEY)

test: test.c $(LIBIOMEDLEY)
	$(CC) -g test.c -L. -ljpeg -liomedley -lz -lm -o $@

$(LIBIOMEDLEY):	$(OBJS) $(LIBTIFF) $(LIBJPEG) $(LIBGIF) $(LIBPNGOBJ)
	ar -ruv $@ $(OBJS)
	./mungelibs
	$(RANLIB) $@

.c.o:
	$(CC) -c $(DEFS) $(CPPFLAGS) $(CFLAGS) $<

$(LIBGIF):
	cd $(LIBGIFDIR) && $(MAKE)

$(LIBTIFF):
	cd $(LIBTIFFDIR) && $(MAKE)

$(LIBPNG):
	cd $(LIBPNGDIR) && $(MAKE) $(LIBPNG)

$(LIBJPEG):
	cd $(LIBJPEGDIR) && $(MAKE) $(LIBJPEG)

clean:
	rm -f *.o libiomedley.a
	cd $(LIBTIFFDIR) && make clean && rm -f $(LIBTIFF) &&  cd ..
	cd $(LIBPNGDIR) && make clean && cd ..
	cd $(LIBJPEGDIR) && make clean && cd ..
	cd $(LIBGIFDIR) && make clean && cd ..

install: all
	install -m 644 $(LIBIOMEDLEY) $(prefix)/lib

io_aviris.o: io_aviris.c iomedley.h io_lablib3.h toolbox.h
io_envi.o: io_envi.c io_envi.h iomedley.h io_lablib3.h toolbox.h
io_ers.o: io_ers.c iomedley.h io_lablib3.h toolbox.h
io_goes.o: io_goes.c iomedley.h io_lablib3.h toolbox.h
io_grd.o: io_grd.c iomedley.h io_lablib3.h toolbox.h
io_imath.o: io_imath.c iomedley.h io_lablib3.h toolbox.h
io_isis.o: io_isis.c iomedley.h io_lablib3.h toolbox.h
io_lablib3.o: io_lablib3.c header.h tools.h io_lablib3.h toolbox.h
io_gif.o: io_gif.c iomedley.h io_lablib3.h toolbox.h
io_png.o: io_png.c iomedley.h io_lablib3.h toolbox.h
io_jpeg.o: io_jpeg.c iomedley.h io_lablib3.h toolbox.h
io_tiff.o: io_tiff.c iomedley.h io_lablib3.h toolbox.h
io_pnm.o: io_pnm.c iomedley.h io_lablib3.h toolbox.h
io_bmp.o: io_bmp.c iomedley.h io_lablib3.h toolbox.h
io_raw.o: io_raw.c iomedley.h io_lablib3.h toolbox.h
io_vicar.o: io_vicar.c iomedley.h io_lablib3.h toolbox.h
iomedley.o: iomedley.c iomedley.h io_lablib3.h toolbox.h
tools.o: tools.c tools.h
