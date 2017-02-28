FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

ALLINC=  -I$(FILESINCS)
CFLAGS=-O $(ALLINC)

# all is the default rule
all: libfolder.a  $(LIBOFILES) clean

LIBOFILES = fold_attr.o fold_create.o fold_error.o fold_globals.o fold_init.o fold_locate.o fold_lock.o fold_log.o fold_read.o fold_request.o fold_view.o fold_write.o fold.o 
$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@


# remove the old tapestry library and remake the new one
libfolder.a:	$(LIBOFILES)
	rm -f $@
	ar cq $@ $(LIBOFILES)
	mv libfolder.a $(HOME)/prepose/src/api/fold/objs/libfolder.a
clean:
	rm -f $(LIBOFILES)
