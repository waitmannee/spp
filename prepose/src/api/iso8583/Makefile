FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
ALLINC=  -I $(FILESINCS)

# all is the default rule
all: libiso8583.a clean  $(LIBOFILES)

LIBOFILES = iso8583_conf.o iso_8583.o iso_chkmsg.o
$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@ 
# remove the old tapestry library and remake the new one
libiso8583.a:	$(LIBOFILES)
	rm -f $@
	ar cq $@ $(LIBOFILES)
	mv $@ $(LIBSPATH)/$@
clean:
	rm -f $(LIBOFILES)
	