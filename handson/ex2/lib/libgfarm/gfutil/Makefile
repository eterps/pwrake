# $Id: Makefile 4550 2010-03-25 10:09:01Z tatebe $

top_builddir = ../../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

LIBRARY = libgfutil.la
LIBRARY_RESULT = # do not install

SRCS =	alloc.c \
	daemon.c \
	gfevent.c \
	gfnetdb.c \
	hash.c \
	hash_strptr.c \
	id_table.c \
	limit.c \
	logutil.c \
	lru_cache.c \
	send_no_sigpipe.c \
	timer.c \
	timeval.c
OBJS =	alloc.lo \
	daemon.lo \
	gfevent.lo \
	gfnetdb.lo \
	hash.lo \
	hash_strptr.lo \
	id_table.lo \
	limit.lo \
	logutil.lo \
	lru_cache.lo \
	send_no_sigpipe.lo \
	timer.lo \
	timeval.lo

CFLAGS = $(pthread_includes) $(COMMON_CFLAGS) -I$(GFUTIL_SRCDIR)

all: $(LIBRARY)

include $(top_srcdir)/makes/lib.mk
include $(top_srcdir)/makes/gflog.mk

###

$(OBJS): $(DEPGFARMINC)
daemon.lo: gfutil.h
gfevent.lo: $(INC_SRCDIR)/gfarm_misc.h gfutil.h gfevent.h
gfnetdb.lo: gfnetdb.h
id_table.lo: id_table.h
hash.lo: gfutil.h hash.h
hash_strptr.lo: hash.h
limit.lo: gfutil.h
logutil.lo: gfutil.h
lru_cache.lo: lru_cache.h
timer.lo: timer.h
timeval.lo: gfutil.h
