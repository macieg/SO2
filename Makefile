CFLAGS=-Wall -std=c99
S=serwer.c
K=klient.c

all: serwer klient

serwer: $(S)
	gcc $(CFLAGS) $(S) -o serwer

klient: $(K)
	gcc $(CFLAGS) $(K) -o klient

clean:
	rm -f serwer klient
