FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib

ALLINC=  -I$(FILESINCS)
CFLAGS=-O $(ALLINC)

# all is the default rule
all: libdcsutil.a  $(LIBOFILES) clean

LIBOFILES = dcs_globals.o dcs_log.o dcs_shm_init.o dcs_sysconf.o dcs_sysstat.o 

$(LIBOFILES): *.c
	$(CC) -c -g $(ALLINC) $*.c -o $@

# remove the old tapestry library and remake the new one
libdcsutil.a:	$(LIBOFILES) 
	rm -f $@
	ar cq $@ $(LIBOFILES) 
clean:
	rm -f $(LIBOFILES) 
