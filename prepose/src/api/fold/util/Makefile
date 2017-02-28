FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

ALLINC=  -I$(FILESINCS)
CFLAGS=-O $(ALLINC)

LIBOFILES = util_command.o util_conf.o util_misc.o util_signal.o util_common.o util_function.o

# all is the default rule
all: libutil.a clean $(LIBOFILES)

# All source files have associated object files
$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@ 

# remove the old tapestry library and remake the new one
libutil.a:	$(LIBOFILES)
	rm -f $@
	ar cq $@ $(LIBOFILES)
	mv libutil.a $(HOME)/prepose/src/api/fold/objs/libutil.a
clean:
	rm -f $(LIBOFILES)