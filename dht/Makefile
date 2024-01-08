CFLAGS = -g -Wall
LDFLAGS = -L/usr/lib/x86_64-linux-gnu
LDLIBS = -lcrypt

dht-example: dht-example.o dht.o

all: dht-example

clean:
	-rm -f dht-example dht-example.o dht-example.id dht.o *~ core
