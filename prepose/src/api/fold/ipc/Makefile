FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

LIB_FILES = ipc_makekey.c ipc_msg.c ipc_sem.c ipc_shm.c ipc_fmap.c

ALLINC=  -I$(FILESINCS)
CFLAGS=-O $(ALLINC)


# all is the default rule
all: libipc.a clean $(LIBOFILES)

# All source files have associated object files      
LIBOFILES = ipc_makekey.o ipc_msg.o ipc_sem.o ipc_shm.o ipc_fmap.o

$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@

# remove the old tapestry library and remake the new one
libipc.a:	$(LIBOFILES)
	rm -f $@
	ar cq $@ $(LIBOFILES)
		mv libipc.a $(HOME)/prepose/src/api/fold/objs/libipc.a    
clean:
	rm -f $(LIBOFILES)