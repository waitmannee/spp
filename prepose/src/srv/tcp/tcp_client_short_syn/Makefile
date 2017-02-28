FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS)  -I .

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -liso8583 -lpthread

CFLAGS= $(ALLINC)
.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@

EXE =  synTcpMgt synTcp synTcpCmd

all: $(EXE) cleanup

synTcpMgt:syn_tcp_mgt.o
	$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
	mv $@ $(BINSPATH)/$@
		
synTcp:	syn_tcp.o
	$(CC) -g -o $@ $(CFLAGS)  $^ $(LIB)
	mv $@ $(BINSPATH)/$@

synTcpCmd:syn_tcp_cmd.o 
	$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
	mv $@ $(BINSPATH)/$@

cleanup:
	rm -f *.o
