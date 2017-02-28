FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS) 

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lpthread

CFLAGS= $(ALLINC)
.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE =  synHttpMgt synHttp synHttpCmd

all: $(EXE) cleanup

synHttpMgt:	syn_http_mgt.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
synHttp:	syn_http.o
		$(CC) -g -o $@ $(CFLAGS)  $^ $(LIB)
		mv $@ $(BINSPATH)/$@

synHttpCmd:	syn_http_cmd.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

cleanup:
	rm -f *.o
