FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS) 

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lpthread

CFLAGS=$(ALLINC)
.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE =  httpClientSvr httpClient httpClientCmd 

all: $(EXE) $(LIBOBJ) cleanup

httpClientSvr:	http_clientSvr.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
httpClient:	httpClient.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

httpClientCmd:	httpClientCmd.o 
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

cleanup:
	rm -f *.o
