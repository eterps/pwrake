.SUFFIXES:
.SUFFIXES: .c .o

INCLUDE_DIR = 	../Montage_v3.3/lib/include
LIB_DIR = 	../Montage_v3.3/lib
CC     =	gcc
MPICC  =
CFLAGS =	-g -I. -I$(INCLUDE_DIR) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I/usr/include/cfitsio -I/usr/include/wcs
LIBS   =	-L$(LIB_DIR) -lwcs -lcfitsio -lnsl -lm

SBINS = reduce_match

BINS   = 	$(SBINS)

.c.o:
		$(CC) $(CFLAGS)  -c  $*.c

all:		$(BINS)

reduce_match: reduce_match.o
	$(CC) -o reduce_match reduce_match.o $(LIBS)

clean:
	rm -f *.o $(BINS)
