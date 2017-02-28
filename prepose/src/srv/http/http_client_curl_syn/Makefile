FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS) 

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lpthread -lcurl

CFLAGS= $(ALLINC)

.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE =  lsynHttpCMgt lsynHttpC lsynHttpCCmd

all: $(EXE) cleanup

lsynHttpCMgt:	syn_httpc_mgt.c
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
lsynHttpC:	syn_httpc.c
		$(CC) -g -o $@ $(CFLAGS)  $^ $(LIB)
		mv $@ $(BINSPATH)/$@

lsynHttpCCmd:	syn_httpc_cmd.c 
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

cleanup:
	rm -f *.o
