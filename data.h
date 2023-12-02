#ifndef DATA_H
#define DATA_H

#define MAXFILENAME 255
#define MAXPATHNAME 4096

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
 * */
struct File {
  char name[MAXFILENAME];
  char type;
  char *preview;
  unsigned int ownerUID;
  long long byteSize;
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
  int doNotUse;
};

void readDir(char *filePath, Directory *dir);//char **files);

#endif
