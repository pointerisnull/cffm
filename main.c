/*******************************\
* CONSOLE-FRIENDLY FILE MANAGER *
*                               *
*   -Bdon 2023-2024             *
\*******************************/
#define VERSION "v.1.0.2"

#include "config.h"
#include "hash.h"
#include "data.h"
#include "display.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void assign_settings();
void init_program_state();
void free_data();
State state;

int main(int argc,  char *argv[]) {
  Directory *directory;
  Display *window;
  int i;
  
  char current_path[MAXPATHNAME] = {'\0'};
  if (argc > 1) {
    if ((strncmp(argv[1], "-v", 3) == 0) || (strncmp(argv[1], "--version", 10) == 0))
      printf("Console-Friendly File Manager %s\n", VERSION);
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
  main: ;

  init_program_state();
  if (current_path[0] == '\0') getcwd(current_path, MAXPATHNAME);
  chdir(current_path);

  directory = init_directories(current_path);
  window = init_display(directory);
  
  while (state.is_running) {
    get_updates(window);
    update_display(window, &directory);
  }
  
	kill_display(window);
  
  printf("%s\n\n", directory->path);
  for (i = 0; i < HASHTABLE_SIZE; i++) {
    if (state.ht.entries[i].dir == NULL) {
      printf("%d: %s\n", i, "____");
    } else {
      ht_entry *current = &state.ht.entries[i];
      printf("%d: ", i);
      while (current != NULL) {
        Directory *dir = current->dir;
        printf("%s ->", dir->path);
        current = current->next;
      }
      if (current == NULL) printf(" %s", "end");
      printf("\n");
 
    }
  }
  printf("Total HashTable Collisions: %d\n", state.ht.collisions);

  free_data();
  printf("CFFM shutdown successfully.\n");
  return 0;
}

void free_directory(Directory *dir) {
  if (dir == NULL) return;
  if (dir->folders != NULL) free(dir->folders);
  if (dir->files != NULL) {
    int i;
    for (i = 0; i < dir->filec; i++)
      if (dir->files[i].preview != NULL) free(dir->files[i].preview);
    free(dir->files);
  }
  if (dir != NULL) free(dir);
}

void free_data() {
  /*free all directories*/
  Table *ht = &state.ht;
  int i;
  for (i = 0; i < HASHTABLE_SIZE; i++) {
    if (ht->entries[i].dir == NULL) /*most likely*/
      continue;
    else if (ht->entries[i].next == NULL) { /*likely*/
      free_directory(ht->entries[i].dir);
      ht->entries[i].dir = NULL;
    }
    else { /*worst case, values are in a chain*/
      ht_entry *current = &ht->entries[i];
      while (current != NULL) {
        free_directory(current->dir);
        current->dir = NULL;
        current = current->next;
      }
    }
  }
  /*free the hash table itself*/
  ht_free(&state.ht);
}

void init_program_state() {
  state.is_running = 1;
  assign_settings(); 
  ht_init(&state.ht);
  state.visual_mode = 0;
  memset(state.cb.folderptr, 0, MAXSELECTED-1);
  memset(state.cb.fileptr, 0, MAXSELECTED-1);
  state.cb.filec = 0;
  state.cb.folderc = 0;
  state.cutting = 0;
  state.copying = 0;
  state.deleting = 0;
}

void assign_settings() {
  state.show_hidden = SHOWHIDDENDEFAULT;
  state.show_border = SHOWBORDERDEFAULT;
  state.shift_pos = SHIFTSIZE;
}
