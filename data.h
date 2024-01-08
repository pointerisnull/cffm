#ifndef DATA_H
#define DATA_H

#define MAXFILENAME 256
#define MAXPATHNAME 4096
#define MAXPREVIEWSIZE 4096
#define MAXSELECTED 256

#include <stdint.h>
#include "hash.h"

typedef struct File File;
typedef struct Folder Folder;
typedef struct Directory Directory;
typedef struct State State;
/*
 *TYPES
 *a = archive
 *b = binary
 *e = executable
 *s = script
 *t = text
 *z = special, display only
 * */
struct File {
  char name[MAXFILENAME];
  char path[MAXPATHNAME];
  char type;
  char *preview;
  uint32_t inode;
  uint16_t ownerUID;
  uint32_t date_unix;
  uint64_t bytesize;
  char date[64];
};

struct Folder {
  char name[MAXFILENAME];
  char path[MAXPATHNAME];
  char date[64];
  uint32_t inode;
  uint16_t ownerUID;
  uint32_t date_unix;
  Directory *subdir;
};

struct Directory {
  Directory *parent;
  Folder *folders;
  File *files;
  char path[MAXPATHNAME];
  uint16_t folderc;
  uint16_t filec;
  uint16_t selected;
  uint16_t ht_index;
  int broken;
};

typedef struct ClipBoard {
  Folder *folderptr[MAXSELECTED];
  File *fileptr[MAXSELECTED];
  int folderc;
  int filec;
} ClipBoard;

struct State {
  uint8_t is_running;
  uint8_t current_tab; /* to be implemented */
  uint8_t visual_mode;
  uint8_t cutting;
  uint8_t copying;
  uint8_t deleting;
  /*settings*/
  uint8_t show_hidden;
  uint8_t show_border;
  uint8_t shift_pos;
  /*directory hashtable*/
  Table ht;

  ClipBoard cb;
};

extern State state;

Directory *init_directories(char *path);
void read_directory(const char *filePath, Directory *dir);
void update_directory(Directory *dir);
void free_directory_tree(Directory **dir, int free_src_dir);

#endif
