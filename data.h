#ifndef DATA_H
#define DATA_H

#define MAXFILENAME 256
#define MAXPATHNAME 4096
#define MAXPREVIEWSIZE 4096

#include <stdint.h>

typedef struct File File;
typedef struct Folder Folder;
typedef struct Directory Directory;
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
  uint16_t ownerUID;
  uint32_t date_unix;
  uint64_t bytesize;
  char date[64];
};

struct Folder {
  char name[MAXFILENAME];
  char path[MAXPATHNAME];
  char date[64];
  Directory *subdir;
};

struct Directory {
  Directory *parent;
  Folder *folders;
  File *files;
  char name[MAXFILENAME];
  char path[MAXPATHNAME];
  uint16_t folderCount;
  uint16_t fileCount;
  uint16_t selected;
  uint16_t ht_index;
  int broken;
};

Directory *init_directories(char *path);
void read_directory(const char *filePath, Directory *dir);
void update_directory(Directory *dir);
void free_directory_tree(Directory **dir, int free_src_dir);

#endif
