FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS)

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lpthread

CFLAGS=$(ALLINC)

.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE =  httpServerMgt httpServer httpServerCmd

all: $(EXE) cleanup

httpServerMgt:	http_server_mgt.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
httpServer:	http_server.o
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

httpServerCmd:	http_server_cmd.o 
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

cleanup:
	rm -f *.o
