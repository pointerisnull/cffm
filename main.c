/*******************************\
* CONSOLE-FRIENDLY FILE MANAGER *
*                               *
*   -Bdon 2023                  *
\*******************************/
#define VERSION "v.0.2.4 (indev)"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include "data.h"
#include "display.h"
#include "config.h"

State state;
//WIP
int gen_default_config() {
 /* char *buffer[] = {
    "\[Settings\]",
    "SHIFTSIZE=16"
  }*/
  return 0;
}

void init() {
  state.isRunning = 1;
  state.hasPerformedAction = 0;
  state.showHidden = 0;
  state.showBorder = 0;
  state.shiftPos = 16;

}

int main(int argc,  char *argv[]) {
  char current_path[MAXPATHNAME] = {'\0'};
  if(argc > 1) {
    if((strncmp(argv[1], "-v", 3) == 0) || (strncmp(argv[1], "--version", 10) == 0))
      printf("CFFM %s\n", VERSION);
    else if(strncmp(argv[1], "--gen-config", 13) == 0) {
      printf("Generating a new config file...\n");
      if(gen_default_config() == 0) printf("Created a fresh config: ~/.config/cffm.conf\n");
      else printf("Error generating config! Exiting!\n");
    }
    else
      printf("CFFM Useage\n\nHelp:\n\t-h\n\t--help\nInfo:\n\t-v\n\t--version\nConfig:\n\t--gen-config\n\n");
    return 0;
  }
  
  if(current_path[0] == '\0') getcwd(current_path, MAXPATHNAME);
  init();
  Directory *directory = init_directories(current_path);
  
  Display *window = init_display(directory);

  while(state.isRunning) {
    get_updates(window);
    update_display(window, &directory);
  }

	kill_display(window);
  /*TODO: FREE DIRECTORY TREE PROPERLY*/
  return 0;
}
