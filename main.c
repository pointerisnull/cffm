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

int read_config_file();
int gen_default_config();
void use_defaults();
void init();
State state;

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
    else if(access(argv[1], F_OK | R_OK) == 0) {
      if(argv[1][strlen(argv[1])-1] == '/' && strncmp(argv[1], "/", 2) != 0) 
        argv[1][strlen(argv[1])-1] = '\0'; 
      strncpy(current_path, argv[1], strlen(argv[1]));
      goto main;
    }
    else
      printf("CFFM Useage\n\nHelp:\n\t-h\n\t--help\nInfo:\n\t-v\n\t--version\nConfig:\n\t--gen-config\n\n");
    return 0;
  }
  main:
  
  init();
  if(current_path[0] == '\0') getcwd(current_path, MAXPATHNAME);
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

void init() {
  state.isRunning = 1;
  state.hasPerformedAction = 0;

}

int read_config_file() {
	char *filepath;
	char *filename = "/.config/cffm.conf";
	filepath = malloc(strlen(getenv("HOME")) + strlen(filename) + 1);
	strcpy(filepath, getenv("HOME"));
	strcat(filepath, filename);
  
  FILE *fp = fopen(filepath, "r");
  free(filepath);
  if(fp == NULL) return 1;

  fclose(fp);
  return 0;
}

int gen_default_config() {
  char *buffer[] = {
    "[Settings]\n",
    "shift_size=16\n",
    "show_border=0\n",
    "show_hidden=0\n",
    "\n",
    "[Colors]\n",
    "border=PURPLE\n",
    "cursor=PURPLE\n",
    "title=GREEN\n",
    "directory=PURPLE\n",
    "root_label=RED\n",
    "file_generic=WHITE\n",
    "file_text=WHITE\n",
    "file_executable=RED\n",
    "file_image=CYAN\n",
    "file_media=CYAN\n",
    "\n",
    "[Key Bindings]\n" //todo
  };

	char *filepath;
	char *filename = "/.config/cffm.conf";
	filepath = malloc(strlen(getenv("HOME")) + strlen(filename) + 1);
	strcpy(filepath, getenv("HOME"));
	strcat(filepath, filename);
  
  FILE *fp = fopen(filepath, "w");
  free(filepath);
  if(fp == NULL) return 1;

  size_t size = sizeof(buffer)/sizeof(buffer[0]);
  for(int i = 0; i < (int)size; i++) {
    fwrite(buffer[i], sizeof(char), strlen(buffer[i]), fp);
  }
  fclose(fp);

  return 0;
}
/*if any file io problems, or if not using config*/
void use_defaults() {
  /*settings*/
  state.showHidden = SHOWHIDDENDEFAULT;
  state.showBorder = SHOWBORDERDEFAULT;
  state.shiftPos = SHIFTSIZE;
  /*colors*/
  state.bgColor = DEFAULTBACKGROUND;
  state.mainBorderColor = BORDERCOLOR;
  state.leftBorderColor = BORDERCOLOR;
  state.rightBorderColor = BORDERCOLOR;
  state.mainCursorColor = CURSORCOLOR;
  state.leftCursorColor = CURSORCOLOR;
  state.rightCursorColor = CURSORCOLOR;
  state.titleColor = TITLECOLOR;
  state.dirColor = DIRCOLOR;
  state.rootDirColor = ROOTCOLOR;
  /*file colors*/
  state.fileColor = FILECOLOR;
  state.textColor = TEXTCOLOR;
  state.exeColor = EXECOLOR;
  state.txtColor = TEXTCOLOR;
  state.imageColor = IMAGECOLOR;
  state.mediaColor = MEDIACOLOR;
}
