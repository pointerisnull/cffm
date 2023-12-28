CC = gcc
CFLAGS = --ansi -lncurses -ltinfo -Wall -Werror -Wextra

SRC = main.c data.c display.c hash.c
BIN = cffm

all:
	$(CC) $(SRC) $(CFLAGS) -o $(BIN)

release:
	$(CC) $(SRC) --static $(CFLAGS) -o $(BIN)

check_leaks:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BIN)

install:
	sudo cp $(BIN) /usr/bin/

uninstall:
	sudo rm /usr/bin/$(BIN)
