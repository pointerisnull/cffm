/*******************************\
* CONSOLE-FRIENDLY FILE MANAGER *
*                               *
*   v.0.2.0 (indev)             *
*                               *
*   -Bdon 2023                  *
\*******************************/

#include <stdlib.h>
#include <ncurses.h>
#include "data.h"
#include "display.h"
#include "config.h"

State state;

int main(int argc,  char *argv[]) {
  Display *window = malloc(sizeof(Display));

  state.isRunning = 1;
  state.hasPerformedAction = 0;
  state.showHidden = 1;
  state.showBorder = 1;
  state.shiftPos = 16;

  Directory *directory = initDirectories();
  
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
