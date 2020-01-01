CC=gcc
CFLAGS=-c
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
all: echo_serv client forwarder

echo_serv: echo_serv.o
	$(CC) $(LDFLAGS) echo_serv.o -o $@

client: client.o
	$(CC) $(LDFLAGS) client.o -o $@

forwarder: forwarder.o
	$(CC) $(LDFLAGS) forwarder.o -o $@

.c.o: $(SOURCES)
	$(CC) $(CFLAGS) $< -o $@
