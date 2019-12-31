CC=gcc
CFLAGS=
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)

echo_server: echo_serv
	$(CC) -o echo_serv echo_serv.c -w

client: client
	$(CC) -o client client.c -w

forwarder: forwarder
	$(CC) -o forwarder forwarder.c -w


