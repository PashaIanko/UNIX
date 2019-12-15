CC=gcc
CFLAGS=-c
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
all: server client

server: server.o
	$(CC) $(LDFLAGS) server.o -o $@

client: client.o
	$(CC) $(LDFLAGS) client.o -o $@

.c.o: $(SOURCES)
	$(CC) $(CFLAGS) $< -o $@
