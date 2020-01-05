CC=gcc
CFLAGS=-c
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
all: echo_serv client client2 forwarder demult

echo_serv: echo_serv.o
	$(CC) $(LDFLAGS) echo_serv.o -o $@

client: client.o
	$(CC) $(LDFLAGS) client.o -o $@

client2: client2.o
	$(CC) $(LDFLAGS) client2.o -o $@

forwarder: forwarder.o
	$(CC) $(LDFLAGS) forwarder.o -o $@

demult: demult.o
	$(CC) $(LDFLAGS) demult.o -o $@

.c.o: $(SOURCES)
	$(CC) $(CFLAGS) $< -o $@
