CC = gcc
CFLAGS = -Wall -g -Iinclude -fdiagnostics-color=always
LDFLAGS =
SRCDIR = src
OBJDIR = obj
BINDIR = bin
SERVER_OBJS = $(OBJDIR)/orchestrator.o $(OBJDIR)/queue.o $(OBJDIR)/task.o
CLIENT_OBJS = $(OBJDIR)/client.o

all: folders orchestrator client

orchestrator: bin/orchestrator
client: bin/client

folders:
	@mkdir -p src include obj bin tmp

bin/orchestrator: $(SERVER_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

bin/client: obj/client.o
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/orchestrator.o: $(SRCDIR)/orchestrator.c $(SRCDIR)/queue.c $(SRCDIR)/task.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/*
