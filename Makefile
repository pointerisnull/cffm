CC = gcc
CFLAGS = --ansi -lncurses -ltinfo -Wall -Werror -Wextra

SRC = main.c data.c display.c
BIN = cffm

all:
	$(CC) $(SRC) $(CFLAGS) -o $(BIN)

release:
	$(CC) $(SRC) --static $(CFLAGS) -o $(BIN)

install:
	sudo cp $(BIN) /usr/bin/

uninstall:
	sudo rm /usr/bin/$(BIN)
