FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

ALLINC=  -I$(FILESINCS)
CFLAGS=-O $(ALLINC)

LIBOFILES = mbuf_access.o mbuf_alloc.o mbuf_create.o mbuf_func.o mbuf_lock.o mbuf_view.o

# all is the default rule
all: libbuf.a clean $(LIBOFILES)

# All source files have associated object files
$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@       

# remove the old tapestry library and remake the new one
libbuf.a:	$(LIBOFILES)
	rm -f $@
	ar cq $@ $(LIBOFILES)
	mv libbuf.a $(HOME)/prepose/src/api/fold/objs/libbuf.a 
clean:
	rm -f $(LIBOFILES)