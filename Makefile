all: jucid; 

CFLAGS:=-g -Wall

jucid: juci_luaobject.c main.c
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ -lblobpack -lusys -lutype -llua5.2 -lubus2 

clean: 
	rm -f *.o jucid
