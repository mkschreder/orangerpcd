all: jucid; 

CFLAGS+=-Wall

-include config.mk

jucid: juci_luaobject.c main.c
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ $(LDFLAGS) -lblobpack -lusys -lutype -lubus2 

clean: 
	rm -f *.o jucid config.h
