FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

ALLINC=  -I$(FILESINCS)
CFLAGS=-O $(ALLINC)
    
# all is the default rule
all: libdcs.a  $(LIBOFILES) clean

LIBOFILES = dcs_com.o  dcs_passmsg.o dcs_seqno.o dcs_sock.o  dcs_chkmsg.o   
$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@

# remove the old tapestry library and remake the new one
libdcs.a:	$(LIBOFILES)
	rm -f $@
	ar cq $@ $(LIBOFILES) 
#	mv $@ $(LIBSPATH)/$@   
clean:
	rm -f *.o