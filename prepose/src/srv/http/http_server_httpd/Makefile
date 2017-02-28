FILESINCS = $(HOME)/prepose/src/include
LIBSPATH = $(HOME)/prepose/src/temp/lib
BINSPATH = $(HOME)/prepose/src/temp/bin

ALLINC= -I $(FILESINCS)  -I .

LIB= -L$(LIBSPATH)  -ldcs  -ldcsutil -lfold -lpthread

CFLAGS=$(ALLINC)

# all is the default rule

.c.o:*.c
	$(CC) -c $(CFLAGS) $*.c -o $@


EXE = httpServerHttpdMgt httpServerHttpd

all: $(EXE) clean

httpServerHttpdMgt:	http_server_httpd_mgt.o
		$(CC) -g -o $@ $(CFLAGS) $^  $(LIB)
		mv $@ $(BINSPATH)/$@
		
httpServerHttpd: http_server_httpd.o protocol.o api.o version.o ip_acl.o ember.o 
		$(CC) -g -o $@ $(CFLAGS) $^ $(LIB)
		mv $@ $(BINSPATH)/$@

clean:
	rm -f $(LIBOFILES)
	