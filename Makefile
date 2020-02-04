#
# Student makefile for Cache Lab
# Note: requires a 64-bit x86-64 system 
#
CC = gcc
CFLAGS = -Wall -std=gnu99 -m64 -g

all: csim.c
	$(CC) $(CFLAGS) -o csim csim.c -lm 

#
# Clean the src dirctory
#
clean:
	rm -f csim
