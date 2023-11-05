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
void getFileStats(File *file, const char *path);
void getDirectoryCounts(int *folderCount, int *fileCount, char *fp);
void sortFolders(Folder *folders, int count);
void sortFiles(File *files, int count);
int isDirectory(const char *path);
char *getHeaderDirPath(char *input);
char *adjustDirPath(char *input); 
/* main function */
void readDir(char *pathIn, Directory *dir) {
  struct dirent *de = NULL;
  char *filePath = adjustDirPath(pathIn);
  DIR *dr = NULL;
  dir->doNotUse = 1;
  dir->path = malloc(sizeof(char)*MAXPATHNAME);
  //if(dir == NULL) dir = malloc(sizeof(Directory));
  if(filePath != NULL)  dr = opendir(filePath);
  
  dir->parent = NULL;
  if (dr == NULL || filePath == NULL) {
    dir->selected = 0;
    dir->folderCount = 0;
    dir->fileCount = 1;
    dir->files = malloc(sizeof(File));
    dir->folders = NULL;
    if(filePath != NULL) strncpy(dir->path, filePath, strlen(filePath));
    if(strcmp(adjustDirPath("."), "/") == 0) strncpy(dir->files[0].name, "[rootfs]", 9);
    else strncpy(dir->files[0].name, "[EMPTY]", 8);
    if(filePath != NULL) printf("Could not open current directory: %s\n", filePath); 
    return; 
  }

  int fCount = 0;
  int dCount = 0;

  getDirectoryCounts(&dCount, &fCount, filePath);

  dir->folderCount = dCount;
  dir->fileCount = fCount;
 
  dir->folders = malloc(sizeof(Folder)*dir->folderCount+1);
  dir->files = malloc(sizeof(File)*dir->fileCount+1);
 
  strncpy(dir->path, filePath, strlen(filePath));
  if(strcmp(dir->path, filePath) != 0) strncpy(dir->path, filePath, strlen(filePath)+1);
  if(strcmp(filePath, "/") != 0) strncat(filePath, "/", 2);
  
  int d = 0; 
  int f = 0; 

  char *temp = (char *) malloc(sizeof(char) * MAXPATHNAME);//strlen(filePath) + 1);
  if(state.showHidden == 0) {
    while ((de = readdir(dr)) != NULL) {
      if(de->d_name[0] != '.') {
        strncpy(temp, filePath, strlen(filePath)+1);
        if(isDirectory(strncat(temp, de->d_name, strlen(de->d_name)+1)) == 1) {
          strncpy(dir->folders[d].name, de->d_name, MAXPATHNAME);
          strncpy(dir->folders[d].path, filePath, MAXPATHNAME);
          strncat(dir->folders[d].path, de->d_name, strlen(de->d_name)+3);
          dir->folders[d].subdir = NULL;
          d++;
        } else {
          getFileStats(&dir->files[f], temp);
          strncpy(dir->files[f].name, de->d_name, strlen(de->d_name)+1);
          f++;
        }
      }
    }
    closedir(dr);
  } else {
    while ((de = readdir(dr)) != NULL) {
      strcpy(temp, filePath);
      if(isDirectory(strcat(temp, de->d_name)) == 1) {
        strncpy(dir->folders[d].name, de->d_name, strlen(de->d_name));
        strncpy(dir->folders[d].path, filePath, strlen(filePath));
        strncat(dir->folders[d].path, de->d_name, strlen(de->d_name));
        dir->folders[d].subdir = NULL;
        d++;
      } else {
        getFileStats(&dir->files[f], temp);
        strncpy(dir->files[f].name, de->d_name, strlen(de->d_name)+1);
        f++;
      }
    }
    closedir(dr);
  }
  filePath = NULL; 
  free(filePath);
  /* sort folders & files*/
  sortFolders(dir->folders, dir->folderCount);
  sortFiles(dir->files, dir->fileCount);
  /* set selected for parent directory*/
  if(strcmp(pathIn, "..") == 0) {
    for(int i = 0; i < dir->folderCount; i++) {
      getcwd(temp, sizeof(char)*MAXPATHNAME);
      if(strcmp(temp, dir->folders[i].path) == 0)
        dir->selected = i;
    }
  } else {
    dir->selected = 0;
  }
  temp = NULL;
  free(temp);
  dir->doNotUse = 0;
}
/*utility definitions*/

char *toLower(char *in) {
  char *s = malloc(sizeof(char) * MAXFILENAME);
  strcpy(s, in);
  for(char *p = s; *p; p++) 
    *p=tolower(*p);
  return s;
}

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

void getFileStats(File *file, const char *path) {
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

char *getHeaderDirPath(char *input) {
  if(strcmp(input, "/") == 0) {
    printf("Nothing above root.\n");
    return input;
  }
  //get the index of the last '/'
  int lastSlashIndex = -1;
  for(int i = 0; i < strlen(input); i++)
    if(input[i] == '/') lastSlashIndex = i;
  char *ret = (char *) malloc(sizeof(char)*lastSlashIndex+1);
  strncpy(ret, input, lastSlashIndex+1);
  //for(int i = 0; i <= lastSlashIndex; i++)
   // ret[i] = input[i];
  /* safety measure incase abpve dir is root*/
  ret[lastSlashIndex+1] = '\0';
  free(input);
  return ret;
}

char *adjustDirPath(char *input) {
  char *temp = (char *) malloc(MAXPATHNAME);
  getcwd(temp, MAXPATHNAME);
  if(strcmp(".\0", input) == 0) {
    if(strcmp(temp, "/\0") == 0 ) return "/";
    return temp;
  }
  else if(strcmp("..\0", input) == 0) {
    //printf("%s\n", temp);
    if(strcmp(temp, "/\0") == 0 ) {
      //printf("%s\n", input);
      //printf("%s\n", temp);
      return NULL;
    } 
    temp = getHeaderDirPath(temp);
    //printf("%s\n", temp);
    if(strcmp(temp, "/\0") == 0) return temp;
    else temp[strlen(temp)-1] = '\0';
    //temp[strlen(temp)] = ' ';
    return temp;
  }
  int slashCount = 0;
  for(int i = 0; i < strlen(input); i++)
    if(input[i] == '/') slashCount++;
  if(slashCount != 0) {
    free(temp);
    char* newTemp = malloc(MAXPATHNAME);
    strncpy(newTemp, input, strlen(input));
    return newTemp;
  } else {
    strncpy(temp, "/", 2);
    strncat(temp, input, strlen(input));
    return temp;
  }
}

void getDirectoryCounts(int *folderCount, int *fileCount, char *fp) {

  char *filePath = (char *)malloc(strlen(fp)*sizeof(char));
  strcpy(filePath, fp);
  struct dirent *de;
  DIR *dir = opendir(filePath);
  strcat(filePath, "/");
  char *temp;
  if(state.showHidden == 0) {
    while ((de = readdir(dir)) != NULL) {
      if(de->d_name[0] != '.') {
        temp = (char *) malloc(sizeof(char)*strlen(filePath) + sizeof(char)*strlen(de->d_name) + 1); 
        strcpy(temp, filePath);
        strcat(temp, de->d_name);
        if(isDirectory(temp)) {
          *folderCount+=1;
        } else {
          *fileCount+=1;
        }
      }
    }
    closedir(dir);
  }else {
    while ((de = readdir(dir)) != NULL) {
      temp = (char *) malloc(sizeof(char)*strlen(filePath) + sizeof(char)*strlen(de->d_name) + 1); 
      strcpy(temp, filePath);
      strcat(temp, de->d_name);
      if(isDirectory(temp)) {
        *folderCount+=1;
      } else {
        *fileCount+=1;
      }
    }
    closedir(dir);
  }
  free(temp);
  free(filePath);

}
