#
# Makefile for talk client/server 
#
#       make: builds cs87_client and cs87_server execcutables
# make clean: remove all stuff that can be re-built:
#
# https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

CC = gcc
CFLAGS = -g -Wall -Wvla -Werror -Wno-error=unused-variable

BINDIR = bin
SRCDIR = src
INCDIR = include
OBJDIR = obj
LIBDIRS = -L./

LIBS = $(LIBDIRS) -lm -lreadline -lpthread

_DEPS = bencode.h hashtable.h shared.h #peer.h tracker.h
DEPS = $(patsubst %,$(INCDIR)/%,$(_DEPS))

_OBJ = bencode.o hashtable.o shared.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

CLIENT = bin/client
#PEER = bin/peer
TRACKER = bin/tracker

.PHONY: all clean

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCDIR)

all: $(CLIENT) $(TRACKER)

$(CLIENT): $(OBJ) $(OBJDIR)/seeder.o $(OBJDIR)/client.o #$(OBJDIR)/leecher.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(TRACKER): $(OBJ) $(OBJDIR)/tracker.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	$(RM) $(OBJDIR)/*.o $(BINDIR)/* core
