#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include "config.h"
#include "data.h"
#include "hash.h"
/*main functions*/
Directory *init_directories(char *path);
void read_directory(const char *filepath, Directory *dir);
void update_directory(Directory *dir);
void free_directory_tree(Directory **dir, int free_src_dir);
/*utility functions*/
int open_and_read(const char *filepath, Directory *dir);
int is_directory(const char *path);
void get_file_stats(File *file, char *path);
void get_dir_counts(int *folderCount, int *fileCount, const char *fp);
void sort_folders(Folder *folders, int count);
void sort_files(File *files, int count);

void read_directory(const char *filepath, Directory *dir) {
  dir->broken = 1;
  if (filepath != NULL) {  
    memset(dir->path, '\0', MAXPATHNAME);
    strncpy(dir->path, filepath, MAXPATHNAME);
  } else {
    dir->selected = 0;
    dir->folderCount = 0;
    dir->fileCount = 1;
    dir->files = malloc(sizeof(File));
    dir->folders = NULL;
    strncpy(dir->files[0].name, "[ROOTFS]", 9);
    strncpy(dir->path, "[ROOTFS]", 9);
    ht_insert(&state.ht, dir, get_hash("[ROOTFS]"));
    dir->files[0].type = 'z';
    return;
  }
  dir->parent = NULL;
  dir->ht_index = get_hash(dir->path);
  ht_insert(&state.ht, dir, dir->ht_index);
 
  int fCount = 0;
  int dCount = 0;

  get_dir_counts(&dCount, &fCount, filepath);
  dir->folderCount = dCount;
  dir->fileCount = fCount;
 
  dir->folders = malloc(sizeof(Folder)*dir->folderCount+1);
  dir->files = malloc(sizeof(File)*dir->fileCount+1);

  if (open_and_read(filepath, dir) != 0) return;
    /*finishing touches*/
  sort_folders(dir->folders, dir->folderCount);
  sort_files(dir->files, dir->fileCount);
  dir->broken = 0;
  dir->selected = 0;
}

void update_directory(Directory *dir) {
  Directory *parent = dir->parent;
  int selected = dir->selected;
  char path[MAXPATHNAME] = {0};
  strncpy(path, dir->path, MAXPATHNAME);
  free_directory_tree(&dir, 0);
  read_directory(path, dir);
  if (strncmp(dir->path, "/", 2) != 0)
    parent->folders[parent->selected].subdir = dir;
  dir->parent = parent;
  dir->selected = selected;
}
/*frees everything in the source directory's tree*/
void free_directory_tree(Directory **dirptr, int free_src_dir) {
  if (*dirptr == NULL) return;
  int i;
  Directory *dir = *dirptr;
  for (i = 0; i < dir->folderCount; i++)
    if (dir->folders[i].subdir != NULL) 
      free_directory_tree(&dir->folders[i].subdir, 1);

  if (dir->folders != NULL) free(dir->folders);
  dir->folders = NULL;
  dir->folderCount = 0;
  if (dir->files != NULL) free(dir->files);
  dir->files = NULL;
  dir->fileCount = 0;
  /*if freeing the source directory itself, free it*/
  if (free_src_dir) { 
    if (dir->parent->folderCount > dir->parent->selected) /*if not root dir*/
      dir->parent->folders[dir->parent->selected].subdir = NULL;
    ht_delete_element(&state.ht, dir->path);
    free(*dirptr);
    *dirptr = NULL;
  }
}

/*populates the dir struct*/
int open_and_read(const char *filepath, Directory *dir) {
  /*open directory*/
  struct dirent *de = NULL;
  DIR *dr = opendir(filepath);
  if (dr == NULL) return -1;
  int d = 0; 
  int f = 0; 
  char readPath[MAXPATHNAME];
  memset(readPath, '\0', MAXPATHNAME);
  /*now read it*/
  int isroot = strncmp(filepath, "/", 2);
  if (state.showHidden == 0) {
    while ((de = readdir(dr)) != NULL) {
      if (de->d_name[0] != '.') {
        strncpy(readPath, filepath, MAXPATHNAME);
        if (isroot != 0) strncat(readPath, "/", 2);
        strncat(readPath, de->d_name, strlen(de->d_name));
        if (is_directory(readPath) == 1) {
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
      if (strncmp(de->d_name, ".", 2) != 0 && strncmp(de->d_name, "..", 3) != 0) {
        strncpy(readPath, filepath, MAXPATHNAME);
        if (isroot != 0) strncat(readPath, "/", 2);
        strncat(readPath, de->d_name, strlen(de->d_name));
        if (is_directory(readPath) == 1) {
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
  int i;
   for (i = 0; i < (int)strlen(path); i++) {
    if (path[i] == '/') lastIndex = i;
  }
  return lastIndex;
}
/*initializes directories backward toward root *
*   root<--dir<----dir<----current            */
Directory *init_directories(char *currentPath) {
  /*if the current path IS root
  if (strncmp(currentPath, "/", 2) == 0) {
    Directory *root = *rootdir;
    memset(root->path, '\0', MAXPATHNAME);
    strncpy(root->path, "/", 2);
    read_directory("/", root);
    root->parent = malloc(sizeof(Directory));
    read_directory(NULL, root->parent);
    return root;
  }*/
  Directory *current = malloc(sizeof(Directory));
  int slashCount = 0;
  int lastSlashIndex = last_slash_index(currentPath);
  int i;
  for (i = 0; i < (int)strlen(currentPath); i++) {
    if (currentPath[i] == '/') slashCount++;
  }
  /*start at current dir, progress back to root*/
  read_directory(currentPath, current);
  char temp[MAXPATHNAME];
  memset(temp, '\0', MAXPATHNAME);
  strncpy(temp, currentPath, lastSlashIndex);
  Directory *indexer = current;
  /*go backward one directory at a time*/
  for (i = slashCount; i > 0; i--) {
    if (slashCount > 1) {
      Directory *parent = malloc(sizeof(Directory));
      read_directory(temp, parent);
      int s;
      for (s = 0; s < parent->folderCount; s++) {
        if (strcmp(indexer->path, parent->folders[s].path) == 0) {
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
      int s;
      for (s = 0; s < root->folderCount; s++) {
        if (strcmp(indexer->path, root->folders[s].path) == 0) {
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
  char *p;
  for (p = s; *p; p++) 
    *p = tolower(*p);
  return s;
}
/*REPLACE WITH QUICKSORT ALGO EVENTUALLY*/
void sort_folders(Folder *folders, int count) {
  int i, j;
  for (i = 0; i < count-1; i++) {
    int min = i;
    for (j = i+1; j < count; j++) {
      char *jname = to_lowercase(folders[j].name);
      char *minname = to_lowercase(folders[min].name);
      if (strcmp(jname, minname) < 0) 
        min = j;
      free(jname);
      free(minname);
    }
    if (min != i) {
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
  int i, j;
  for (i = 0; i < count-1; i++) {
    int min = i;
    for (j = i+1; j < count; j++) {
      char *jname = to_lowercase(files[j].name);
      char *minname = to_lowercase(files[min].name);
      if (strcmp(jname, minname) < 0) 
        min = j;
      free(jname);
      free(minname);
    }
    if (min != i) {
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
  if (access(path, X_OK) == 0) file->type = 'e';
  file->preview = NULL;
  file->ownerUID = filestat.st_uid;
  file->bytesize = (long long) filestat.st_size;
  file->date_unix = filestat.st_mtime;

  struct passwd *pwd;
  pwd = getpwuid(file->ownerUID);
  if (pwd != NULL) strcpy(file->owner, pwd->pw_name);
  
  struct tm ts;
  ts = *localtime(&file->date_unix);
  strftime(file->date, sizeof(file->date), DATE_FORMAT, &ts);

}

int is_directory(const char *path)
{
    struct stat pathstat;
    stat(path, &pathstat);
    return S_ISDIR(pathstat.st_mode);
}
/*returns the number of folders and files in a directory*/
void get_dir_counts(int *folderCount, int *fileCount, const char *fp) {
  char filepath[MAXPATHNAME];
  memset(filepath, '\0', MAXPATHNAME);
  strncpy(filepath, fp, MAXPATHNAME);
  struct dirent *de = NULL;
  DIR *dir = NULL;
  dir = opendir(filepath);
  if (dir == NULL) return;
  if (strncmp(fp, "/", 2) != 0) strncat(filepath, "/", 2);
  char temp[MAXPATHNAME];
  if (state.showHidden == 0) {
    while ((de = readdir(dir)) != NULL) {
      if (de->d_name[0] != '.') {
        
        memset(temp, '\0', MAXPATHNAME);
        strncpy(temp, filepath, MAXPATHNAME);
        strcat(temp, de->d_name);
        if (is_directory(temp))
          *folderCount+=1;
        else
          *fileCount+=1;
      }
    }
  } else {
    while ((de = readdir(dir)) != NULL) {
      if (strncmp(de->d_name, ".", 2) != 0 && strncmp(de->d_name, "..", 3) != 0) {  
        memset(temp, '\0', MAXPATHNAME);
        strcpy(temp, filepath);
        strcat(temp, de->d_name);
        if (is_directory(temp)) {
          *folderCount+=1;
        } else {
          *fileCount+=1;
        }
      }
    }
  }
  closedir(dir);
}
