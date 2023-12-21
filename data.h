#ifndef DATA_H
#define DATA_H

#define MAXFILENAME 256
#define MAXPATHNAME 4096
#define MAXPREVIEWSIZE 4096

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
  unsigned int ownerUID;
  long long bytesize;
  long long date;
};

struct Folder {
  char name[MAXFILENAME];
  char path[MAXPATHNAME];
  Directory *subdir;
};

struct Directory {
  Directory *parent;
  Folder *folders;
  File *files;
  char name[MAXFILENAME];
  char path[MAXPATHNAME];
  int folderCount;
  int fileCount;
  int selected;
  int broken;
};

Directory *init_directories(char *path);
void read_directory(const char *filePath, Directory *dir);

#endif
