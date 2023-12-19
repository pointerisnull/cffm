CC = gcc
CFLAGS = --static -lncurses -ltinfo -Wall

SRC = main.c data.c display.c
BIN = cffm

all:
	$(CC) $(SRC) $(CFLAGS) -o $(BIN)

install:
	sudo cp $(BIN) /usr/bin/
