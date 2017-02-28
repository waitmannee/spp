FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS) 

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lpthread

CFLAGS=$(ALLINC)

.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE =  shortClientSvr shortClient scClientCmd

all: $(EXE) cleanup

shortClientSvr:	short_clientSvr.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
shortClient:	shortClient.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

scClientCmd:	shortClientCmd.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

cleanup:
	rm -f *.o
