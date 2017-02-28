FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS)

LIB= -L $(LIBSPATH)   -ldcs -ldcsutil -liso8583 -lfold -lnsl -lpthread 
CFLAGS=$(ALLINC)

#LIBOBJ=  dcmhstsrv.o dcmcmd.o dcmhsttcp.o utility.o
.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@


EXE =  dcxhstsv dcxCmd dcxhstcp 
 
all: $(EXE) $(LIBOBJ) cleanup

dcxhstsv: dcmhstsrv.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@

dcxCmd:	dcmcmd.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@
		
dcxhstcp: dcmhsttcp.o utility.o 
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@		
cleanup:
	rm -f *.o
