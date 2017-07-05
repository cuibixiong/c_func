#
# Makefile for nandprog
#

CROSS	= mipsel-linux-
CC	= $(CROSS)gcc
LD 	= $(CROSS)ld


SOURCES	+= $(wildcard *.c)
OBJS	:= $(addsuffix .o , $(basename $(notdir $(SOURCES))))

CFLAGS := -I./ -O2 -Wall 

all: nandprog

nandprog : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

install:


clean:
	rm -f *.o nandprog

