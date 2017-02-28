LIBS=$(LIB) -L$(HOME)/prepose/src/temp/lib  -ldcsutil -ltimer -liso8583 -lfold  -lpthread `xml2-config --libs`
BIN =$(HOME)/prepose/src/temp/bin
CFLAGS=-I $(HOME)/prepose/src/include  -I. `xml2-config --cflags`
OBJ =moniXpep.o moniXpepConsume.o moniEwp.o tuihuo.o chexiao.o moniConsumer_returns.o moniConsumer_revoked.o moniQuirty.o moniBalance.o moniPos.o TransferRepay.o userAuth.o moniSale.o 

EXE = moniXpep moniXpepConsume moniEwp tuihuo chexiao moniConsumer_returns moniConsumer_revoked moniQuirty moniBalance moniPos TransferRepay userAuth moniSale 

.c.o:
#	$(CC) $(CDEPEND) -c $(CFLAGS) $*.c
	$(CC)  -c $(CFLAGS) $*.c
exe:    $(EXE) clean
all:    $(OBJ) $(EXE) clean
	
moniXpep: moniXpep.o
		$(CC) -o moniXpep  $(CFLAGS) moniXpep.o $(LIBS) 
		strip moniXpep 2>/dev/null

userAuth: userAuth.o
		$(CC) -o userAuth  $(CFLAGS) userAuth.o $(LIBS) 
		strip userAuth 2>/dev/null
		
moniXpepConsume: moniXpepConsume.o
		$(CC) -o moniXpepConsume  $(CFLAGS) moniXpepConsume.o $(LIBS) 
		strip moniXpepConsume 2>/dev/null

tuihuo: tuihuo.o
		$(CC) -o tuihuo  $(CFLAGS) tuihuo.o $(LIBS) 
		strip tuihuo 2>/dev/null
		
chexiao: chexiao.o
		$(CC) -o chexiao  $(CFLAGS) chexiao.o $(LIBS) 
		strip chexiao 2>/dev/null

moniEwp: moniEwp.o
		$(CC) -o moniEwp  $(CFLAGS) moniEwp.o $(LIBS) 
		strip moniEwp 2>/dev/null
		
moniConsumer_returns: moniConsumer_returns.o
		$(CC) -o moniConsumer_returns  $(CFLAGS) moniConsumer_returns.o $(LIBS) 
		strip moniConsumer_returns 2>/dev/null
		
moniConsumer_revoked: moniConsumer_revoked.o
		$(CC) -o moniConsumer_revoked  $(CFLAGS) moniConsumer_revoked.o $(LIBS) 
		strip moniConsumer_revoked 2>/dev/null
		
moniQuirty: moniQuirty.o
		$(CC) -o moniQuirty  $(CFLAGS) moniQuirty.o $(LIBS) 
		strip moniQuirty 2>/dev/null
		
moniBalance: moniBalance.o
		$(CC) -o moniBalance  $(CFLAGS) moniBalance.o $(LIBS) 
		strip moniBalance 2>/dev/null

TransferRepay: TransferRepay.o
		$(CC) -o TransferRepay  $(CFLAGS) TransferRepay.o $(LIBS) 
		strip TransferRepay 2>/dev/null
		
moniPos: moniPos.o
		$(CC) -o moniPos  $(CFLAGS) moniPos.o $(LIBS) 
		strip moniPos 2>/dev/null
moniSale:moniSale.o
		$(CC) -g -o $@  $(CFLAGS) $^ $(LIBS) 
		strip moniSale 2>/dev/null
		
clean:
		rm -f *.o
	