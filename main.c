/*******************************
*COMPUTER-FRIENDLY FILE MANAGER*
*******************************/
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

  state.showHidden = 0;

  Directory *directory = malloc(sizeof(Directory));

  readDir(".", directory); //directory);
  directory->parent = malloc(sizeof(Directory)); //malloc(sizeof(Directory));
  readDir("..", directory->parent);//directory->parent);
  if(directory->parent->folders != NULL) directory->parent->folders[directory->parent->selected].subdir = directory;
  
  //if(directory->parent->folders != NULL) directory->parent->folders[directory->parent->selected].subdir = NULL; //*directory;
  
  initDisplay(window, directory);
  for(;;) {
    checkUpdates(window);
    display(window, &directory);
  }
	killDisplay();
  //printf("%s\n", directory->path);
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
	
  return 0;
}
