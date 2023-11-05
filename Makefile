compile:
	gcc --static main.c display.c data.c -o cffm -lncurses -ltinfo -Wall
