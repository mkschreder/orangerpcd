all: jucid; 

CFLAGS:=-g -Wall

jucid: main.c
	$(CC) $(CFLAGS) -std=gnu99 -o $@ $^ -lblobpack -lusys -lubus2 

clean: 
	rm -f *.o jucid
