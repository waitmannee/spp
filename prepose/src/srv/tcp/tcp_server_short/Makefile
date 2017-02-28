FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS) 

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lpthread

CFLAGS= $(ALLINC)
.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE =  shortSvr shortTcp scServerCmd

all: $(EXE) $(LIBOBJ) cleanup

shortSvr:	short_connect_server.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
shortTcp:	shortconntcp.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@
scServerCmd:	shortserverCmd.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@
cleanup:
	rm -f *.o
