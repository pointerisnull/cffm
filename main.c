/*******************************\
* CONSOLE-FRIENDLY FILE MANAGER *
*                               *
*   -Bdon 2023                  *
\*******************************/
#define VERSION "v.0.3.0 (indev)"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include "data.h"
#include "display.h"
#include "config.h"

void assign_settings();
void init_program_state();
State state;

int main(int argc,  char *argv[]) {
  char current_path[MAXPATHNAME] = {'\0'};
  if (argc > 1) {
    if ((strncmp(argv[1], "-v", 3) == 0) || (strncmp(argv[1], "--version", 10) == 0))
      printf("CFFM %s\n", VERSION);
    else if (access(argv[1], F_OK | R_OK) == 0) {
      if (argv[1][strlen(argv[1])-1] == '/' && strncmp(argv[1], "/", 2) != 0) 
        argv[1][strlen(argv[1])-1] = '\0'; 
      strncpy(current_path, argv[1], strlen(argv[1]));
      goto main;
    }
    else
      printf("CFFM Useage\n\nHelp:\n\t-h\n\t--help\nInfo:\n\t-v\n\t--version\nOpen:\n\tcffm /path/to/dir\n\n");
    return 0;
  }
  main:
  
  init_program_state();
  if (current_path[0] == '\0') getcwd(current_path, MAXPATHNAME);
  Directory *root = malloc(sizeof(Directory));
  Directory *directory = init_directories(current_path, root);
  
  Display *window = init_display(directory);

  while(state.isRunning) {
    get_updates(window);
    update_display(window, &directory);
  }

	kill_display(window);
  free(root->parent->files);
  free(root->parent);
  free_directory_tree(root);
  return 0;
}

void init_program_state() {
  state.isRunning = 1;
  state.hasPerformedAction = 0;
  assign_settings(); 

}

void assign_settings() {
  state.showHidden = SHOWHIDDENDEFAULT;
  state.showBorder = SHOWBORDERDEFAULT;
  state.shiftPos = SHIFTSIZE;

}
