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
void getFileStats(File *file, char *path);
void getDirectoryCounts(int *folderCount, int *fileCount, const char *fp);
void sortFolders(Folder *folders, int count);
void sortFiles(File *files, int count);
int isDirectory(const char *path);

void readDir(const char *filePath, Directory *dir) {
  dir->doNotUse = 1;
  if(filePath != NULL) {  
    memset(dir->path, '\0', MAXPATHNAME);
    strncpy(dir->path, filePath, MAXPATHNAME);
  } else if(filePath == NULL) {
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

  getDirectoryCounts(&dCount, &fCount, filePath);
  dir->folderCount = dCount;
  dir->fileCount = fCount;
 
  dir->folders = malloc(sizeof(Folder)*dir->folderCount+1);
  dir->files = malloc(sizeof(File)*dir->fileCount+1);
  /*open directory*/
  struct dirent *de = NULL;
  DIR *dr = opendir(filePath);
  if(dr == NULL) return;
  int d = 0; 
  int f = 0; 
  char readPath[MAXPATHNAME];
  memset(readPath, '\0', MAXPATHNAME);
  /*now read it*/
  int isRootDir = strncmp(filePath, "/", 2);
  if(state.showHidden == 0) {
    while ((de = readdir(dr)) != NULL) {
      if(de->d_name[0] != '.') {
        strncpy(readPath, filePath, MAXPATHNAME);
        if(isRootDir != 0) strncat(readPath, "/", 2);
        strncat(readPath, de->d_name, strlen(de->d_name));
        if(isDirectory(readPath) == 1) {
          strncpy(dir->folders[d].name, de->d_name, MAXFILENAME);
          strncpy(dir->folders[d].path, readPath, MAXPATHNAME);
          dir->folders[d].subdir = NULL;
          d++;
        } else {
          getFileStats(&dir->files[f], readPath);
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
        if(isRootDir != 0) strncat(readPath, "/", 2);
        strncat(readPath, de->d_name, strlen(de->d_name));
        if(isDirectory(readPath) == 1) {
          strncpy(dir->folders[d].name, de->d_name, MAXFILENAME);
          strncpy(dir->folders[d].path, readPath, MAXPATHNAME);
          dir->folders[d].subdir = NULL;
          d++;
        } else {
          getFileStats(&dir->files[f], readPath);
          strncpy(dir->files[f].name, de->d_name, strlen(de->d_name)+1);
          strncpy(dir->files[f].path, readPath, MAXPATHNAME);
          f++;
        }
      }
    }
    closedir(dr);
  }
  /*finishing touches*/
  sortFolders(dir->folders, dir->folderCount);
  sortFiles(dir->files, dir->fileCount);
  dir->doNotUse = 0;
  dir->selected = 0;
}

int getLastSlashIndex(char *path) {
  int lastIndex = 0;
   for(int i = 0; i < (int)strlen(path); i++) {
    if(path[i] == '/') lastIndex = i;
  }
  return lastIndex;
}
/*initializes directories backward toward root *
*   root<--dir<----dir<----current            */
Directory *initDirectories(char *currentPath) {
  Directory *current = malloc(sizeof(Directory));
  //char currentPath[MAXPATHNAME];
  //getcwd(currentPath, MAXPATHNAME);
  int slashCount = 0;
  int lastSlashIndex = getLastSlashIndex(currentPath);
  for(int i = 0; i < (int)strlen(currentPath); i++) {
    if(currentPath[i] == '/') slashCount++;
  }
  /*start at current dir, progress back to root*/
  readDir(currentPath, current);
  char temp[MAXPATHNAME];
  memset(temp, '\0', MAXPATHNAME);
  strncpy(temp, currentPath, lastSlashIndex);
  Directory *indexer = current;
  /*go backward one directory at a time*/
  for(int i = slashCount; i > 0; i--) {
    if(slashCount > 1) {
      Directory *parent = malloc(sizeof(Directory));
      readDir(temp, parent);
      for(int s = 0; s < parent->folderCount; s++) {
        if(strcmp(indexer->path, parent->folders[s].path) == 0) {
          parent->folders[s].subdir = indexer;
          parent->selected = s;
        } 
      }
      indexer->parent = parent;
      indexer = parent;
      
      lastSlashIndex = getLastSlashIndex(indexer->path);
      memset(temp, '\0', MAXPATHNAME);
      strncpy(temp, indexer->path, lastSlashIndex);
      slashCount--;
    } else {
      /*is root dir*/
      Directory *root = malloc(sizeof(Directory));
      memset(root->path, '\0', MAXPATHNAME);
      strncpy(root->path, "/", 2);
      readDir("/", root);
      for(int s = 0; s < root->folderCount; s++) {
        if(strcmp(indexer->path, root->folders[s].path) == 0) {
          root->folders[s].subdir = indexer;
          root->selected = s;
        } 
      }
      indexer->parent = root;
      root->parent = malloc(sizeof(Directory));
      readDir(NULL, root->parent);
    }
  }

  return current;
}
/*utility definitions*/
char *toLower(char *in) {
  char *s = malloc(sizeof(char) * MAXFILENAME);
  strcpy(s, in);
  for(char *p = s; *p; p++) 
    *p=tolower(*p);
  return s;
}
/*REPLACE WITH QUICKSORT ALGO EVENTUALLY*/
void sortFolders(Folder *folders, int count) {
  for(int i = 0; i < count-1; i++) {
    int min = i;
    for(int j = i+1; j < count; j++) {
      char *jname = toLower(folders[j].name);
      char *minname = toLower(folders[min].name);
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
void sortFiles(File *files, int count) {
  for(int i = 0; i < count-1; i++) {
    int min = i;
    for(int j = i+1; j < count; j++) {
      char *jname = toLower(files[j].name);
      char *minname = toLower(files[min].name);
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

void getFileStats(File *file, char *path) {
  struct stat filestat;
  stat(path, &filestat);
  if(access(path, X_OK) == 0) file->type = 'e';
  file->preview = NULL;
  file->ownerUID = filestat.st_uid;
  file->byteSize = (long long) filestat.st_size;
  file->date = filestat.st_mtime;
}

int isDirectory(const char *path)
{
    struct stat pathstat;
    stat(path, &pathstat);
    return S_ISDIR(pathstat.st_mode);
}

void getDirectoryCounts(int *folderCount, int *fileCount, const char *fp) {
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
        if(isDirectory(temp))
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
        if(isDirectory(temp)) {
          *folderCount+=1;
        } else {
          *fileCount+=1;
        }
      }
    }
  }
  closedir(dir);
}
