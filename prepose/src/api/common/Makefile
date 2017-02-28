FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

ALLINC=  -I$(FILESINCS)
CFLAGS= -O $(ALLINC)

# All source files have associated object files      
LIBOFILES = progname.o

# all is the default rule
all: libbsd.a clean $(LIBOFILES)

$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@

# remove the old tapestry library and remake the new one
libbsd.a:$(LIBOFILES)
	rm -f $@
	ar cq $@ $(LIBOFILES)
	mv $@ $(LIBSPATH)/$@
clean:
	rm -f $(LIBOFILES)
