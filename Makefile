CFLAGS=-Wall
S=serwer.c
K=klient.c
E=err.c

all: serwer klient

serwer: $(S) $(E)
	gcc $(CFLAGS) $(S) $(E) -pthread -o serwer

klient: $(K) $(E)
	gcc $(CFLAGS) $(K) $(E) -o klient

clean:
	rm -f serwer klient
