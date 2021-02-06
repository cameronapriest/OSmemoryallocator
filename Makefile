# makefile

CC = gcc
CFLAGS = -Wall

allocator: 
	$(CC) $(CFLAGS) allocator.c -o allocator

clean:
	rm -rf allocator

all: 
	$(CC) $(CFLAGS) allocator.c -o allocator
