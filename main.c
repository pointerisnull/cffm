/*******************************
*COMPUTER-FRIENDLY FILE MANAGER*
*******************************/
#define VERSION "0.1.1 alpha"

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "data.h"
#include "display.h"
#include "config.h"

State state;

int main() {
  Display *window = malloc(sizeof(Display));

  state.isRunning = 1;
  state.showHidden = 0;
  state.showBorder = 1;

  Directory *directory = initDirectories();
  printf("Directory Path: %s\n", directory->path);
  
  initDisplay(window, directory);
  while(state.isRunning) {
    checkUpdates(window);
    display(window, &directory);
  }
	killDisplay();
  /*
  directory->parent->files = NULL;
  free(directory->parent->files);
  directory->parent->folders = NULL;
  free(directory->parent->folders);
  directory->parent = NULL;
  free(directory->parent);
  directory->files = NULL;
  free(directory->files);
  directory->folders = NULL;
  free(directory->folders);
  free(directory);
	*/
  return 0;
}
