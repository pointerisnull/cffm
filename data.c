#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <assert.h>
#include "data.h"
#include "config.h"
/* utility functions */
int open_and_read(const char *filePath, Directory *dir);
int is_directory(const char *path);
void get_file_stats(File *file, char *path);
void get_directory_counts(int *folderCount, int *fileCount, const char *fp);
void sort_folders(Folder *folders, int count);
void sort_files(File *files, int count);

void read_directory(const char *filePath, Directory *dir) {
  dir->broken = 1;
  if(filePath != NULL) {  
    memset(dir->path, '\0', MAXPATHNAME);
    strncpy(dir->path, filePath, MAXPATHNAME);
  } else {
    dir->selected = 0;
    dir->folderCount = 0;
    dir->fileCount = 1;
    dir->files = malloc(sizeof(File));
    dir->folders = NULL;
    strncpy(dir->files[0].name, "[ROOTFS]", 9);
    dir->files[0].type = 'z';
    return;
  }
  dir->parent = NULL;
 
  int fCount = 0;
  int dCount = 0;

  get_directory_counts(&dCount, &fCount, filePath);
  dir->folderCount = dCount;
  dir->fileCount = fCount;
 
  dir->folders = malloc(sizeof(Folder)*dir->folderCount+1);
  dir->files = malloc(sizeof(File)*dir->fileCount+1);

  if(open_and_read(filePath, dir) == 0) {
    /*finishing touches*/
    sort_folders(dir->folders, dir->folderCount);
    sort_files(dir->files, dir->fileCount);
    dir->broken = 0;
    dir->selected = 0;
  }
}
/*populates the dir struct*/
int open_and_read(const char *filePath, Directory *dir) {
  /*open directory*/
  struct dirent *de = NULL;
  DIR *dr = opendir(filePath);
  if(dr == NULL) return -1;
  int d = 0; 
  int f = 0; 
  char readPath[MAXPATHNAME];
  memset(readPath, '\0', MAXPATHNAME);
  /*now read it*/
  int isroot = strncmp(filePath, "/", 2);
  if(state.showHidden == 0) {
    while ((de = readdir(dr)) != NULL) {
      if(de->d_name[0] != '.') {
        strncpy(readPath, filePath, MAXPATHNAME);
        if(isroot != 0) strncat(readPath, "/", 2);
        strncat(readPath, de->d_name, strlen(de->d_name));
        if(is_directory(readPath) == 1) {
          strncpy(dir->folders[d].name, de->d_name, MAXFILENAME);
          strncpy(dir->folders[d].path, readPath, MAXPATHNAME);
          dir->folders[d].subdir = NULL;
          d++;
        } else {
          get_file_stats(&dir->files[f], readPath);
          strncpy(dir->files[f].name, de->d_name, strlen(de->d_name)+1);
          strncpy(dir->files[f].path, readPath, MAXPATHNAME);
          f++;
        }
      }
    }
    closedir(dr);
  } else {
    while ((de = readdir(dr)) != NULL) {
      if(strncmp(de->d_name, ".", 2) != 0 && strncmp(de->d_name, "..", 3) != 0) {
        strncpy(readPath, filePath, MAXPATHNAME);
        if(isroot != 0) strncat(readPath, "/", 2);
        strncat(readPath, de->d_name, strlen(de->d_name));
        if(is_directory(readPath) == 1) {
          strncpy(dir->folders[d].name, de->d_name, MAXFILENAME);
          strncpy(dir->folders[d].path, readPath, MAXPATHNAME);
          dir->folders[d].subdir = NULL;
          d++;
        } else {
          get_file_stats(&dir->files[f], readPath);
          strncpy(dir->files[f].name, de->d_name, strlen(de->d_name)+1);
          strncpy(dir->files[f].path, readPath, MAXPATHNAME);
          f++;
        }
      }
    }
    closedir(dr);
  }
  return 0;
}

int last_slash_index(char *path) {
  int lastIndex = 0;
   for(int i = 0; i < (int)strlen(path); i++) {
    if(path[i] == '/') lastIndex = i;
  }
  return lastIndex;
}
/*initializes directories backward toward root *
*   root<--dir<----dir<----current            */
Directory *init_directories(char *currentPath) {
  Directory *current = malloc(sizeof(Directory));
  int slashCount = 0;
  int lastSlashIndex = last_slash_index(currentPath);
  for(int i = 0; i < (int)strlen(currentPath); i++) {
    if(currentPath[i] == '/') slashCount++;
  }
  /*start at current dir, progress back to root*/
  read_directory(currentPath, current);
  char temp[MAXPATHNAME];
  memset(temp, '\0', MAXPATHNAME);
  strncpy(temp, currentPath, lastSlashIndex);
  Directory *indexer = current;
  /*go backward one directory at a time*/
  for(int i = slashCount; i > 0; i--) {
    if(slashCount > 1) {
      Directory *parent = malloc(sizeof(Directory));
      read_directory(temp, parent);
      for(int s = 0; s < parent->folderCount; s++) {
        if(strcmp(indexer->path, parent->folders[s].path) == 0) {
          parent->folders[s].subdir = indexer;
          parent->selected = s;
        } 
      }
      indexer->parent = parent;
      indexer = parent;
      
      lastSlashIndex = last_slash_index(indexer->path);
      memset(temp, '\0', MAXPATHNAME);
      strncpy(temp, indexer->path, lastSlashIndex);
      slashCount--;
    } else {
      /*is root dir*/
      Directory *root = malloc(sizeof(Directory));
      memset(root->path, '\0', MAXPATHNAME);
      strncpy(root->path, "/", 2);
      read_directory("/", root);
      for(int s = 0; s < root->folderCount; s++) {
        if(strcmp(indexer->path, root->folders[s].path) == 0) {
          root->folders[s].subdir = indexer;
          root->selected = s;
        } 
      }
      indexer->parent = root;
      root->parent = malloc(sizeof(Directory));
      read_directory(NULL, root->parent);
    }
  }

  return current;
}
/*utility definitions*/
char *to_lowercase(char *in) {
  char *s = malloc(sizeof(char) * MAXFILENAME);
  strncpy(s, in, MAXFILENAME);
  for(char *p = s; *p; p++) 
    *p = tolower(*p);
  return s;
}
/*REPLACE WITH QUICKSORT ALGO EVENTUALLY*/
void sort_folders(Folder *folders, int count) {
  for(int i = 0; i < count-1; i++) {
    int min = i;
    for(int j = i+1; j < count; j++) {
      char *jname = to_lowercase(folders[j].name);
      char *minname = to_lowercase(folders[min].name);
      if(strcmp(jname, minname) < 0) 
        min = j;
      free(jname);
      free(minname);
    }
    if(min != i) {
      Folder *temp = malloc(sizeof(Folder));
      *temp = folders[i];
      folders[i] = folders[min];
      folders[min] = *temp;
      free(temp);
    }
  }
}
/*REPLACE WITH QUICKSORT ALGO EVENTUALLY*/
void sort_files(File *files, int count) {
  for(int i = 0; i < count-1; i++) {
    int min = i;
    for(int j = i+1; j < count; j++) {
      char *jname = to_lowercase(files[j].name);
      char *minname = to_lowercase(files[min].name);
      if(strcmp(jname, minname) < 0) 
        min = j;
      free(jname);
      free(minname);
    }
    if(min != i) {
      File *temp = malloc(sizeof(File));
      *temp = files[i];
      files[i] = files[min];
      files[min] = *temp;
      free(temp);
    }
  }
}

void get_file_stats(File *file, char *path) {
  struct stat filestat;
  stat(path, &filestat);
  if(access(path, X_OK) == 0) file->type = 'e';
  file->preview = NULL;
  file->ownerUID = filestat.st_uid;
  file->bytesize = (long long) filestat.st_size;
  file->date = filestat.st_mtime;
}

int is_directory(const char *path)
{
    struct stat pathstat;
    stat(path, &pathstat);
    return S_ISDIR(pathstat.st_mode);
}
/*returns the number of folders and files in a directory*/
void get_directory_counts(int *folderCount, int *fileCount, const char *fp) {
  char filePath[MAXPATHNAME];
  memset(filePath, '\0', MAXPATHNAME);
  strncpy(filePath, fp, MAXPATHNAME);
  struct dirent *de = NULL;
  DIR *dir = NULL;
  dir = opendir(filePath);
  if(dir == NULL) return;
  if(strncmp(fp, "/", 2) != 0) strncat(filePath, "/", 2);
  char temp[MAXPATHNAME];
  if(state.showHidden == 0) {
    while ((de = readdir(dir)) != NULL) {
      if(de->d_name[0] != '.') {
        
        memset(temp, '\0', MAXPATHNAME);
        strncpy(temp, filePath, MAXPATHNAME);
        strcat(temp, de->d_name);
        if(is_directory(temp))
          *folderCount+=1;
        else
          *fileCount+=1;
      }
    }
  } else {
    while ((de = readdir(dir)) != NULL) {
      if(strncmp(de->d_name, ".", 2) != 0 && strncmp(de->d_name, "..", 3) != 0) {  
        memset(temp, '\0', MAXPATHNAME);
        strcpy(temp, filePath);
        strcat(temp, de->d_name);
        if(is_directory(temp)) {
          *folderCount+=1;
        } else {
          *fileCount+=1;
        }
      }
    }
  }
  closedir(dir);
}
