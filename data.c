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
void get_stats(void *ptr, char *path, int is_file);
void get_dir_counts(int *folderc, int *filec, const char *fp);
void sort_folders(Folder *folders, int count);
void sort_files(File *files, int count);

void read_directory(const char *filepath, Directory *dir) {
  int fCount = 0;
  int dCount = 0;

  dir->broken = 1;
  if (filepath != NULL) {  
    memset(dir->path, '\0', MAXPATHNAME);
    strncpy(dir->path, filepath, MAXPATHNAME);
  } else {
    dir->selected = 0;
    dir->folderc = 0;
    dir->filec = 1;
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
  if (ht_get_element(&state.ht, dir->path) == NULL) 
    ht_insert(&state.ht, dir, dir->ht_index);

  get_dir_counts(&dCount, &fCount, filepath);
  dir->folderc = dCount;
  dir->filec = fCount;
 
  dir->folders = malloc(sizeof(Folder)*dir->folderc+1);
  dir->files = malloc(sizeof(File)*dir->filec+1);

  if (open_and_read(filepath, dir) != 0) return;
    /*finishing touches*/
  sort_folders(dir->folders, dir->folderc);
  sort_files(dir->files, dir->filec);
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
  int i;
  Directory *dir;
  if (*dirptr == NULL) return;
  
  dir = *dirptr;
  for (i = 0; i < dir->folderc; i++)
    if (dir->folders[i].subdir != NULL) 
      free_directory_tree(&dir->folders[i].subdir, 1);

  if (dir->folders != NULL) free(dir->folders);
  dir->folders = NULL;
  dir->folderc = 0;
  if (dir->files != NULL) {
    for (i = 0; i < dir->filec; i++)
      if (dir->files[i].preview != NULL) free(dir->files[i].preview);
    free(dir->files);
  }
  dir->files = NULL;
  dir->filec = 0;
  /*if freeing the source directory itself, free it*/
  if (free_src_dir) {
    /*remove directory from hashmap*/
    ht_delete_element(&state.ht, dir->path);

    if (dir->parent->folderc > dir->parent->selected) /*if not root dir*/
      dir->parent->folders[dir->parent->selected].subdir = NULL;
    free(*dirptr);
    *dirptr = NULL;
  }
}
/*populates the dir struct*/
int open_and_read(const char *filepath, Directory *dir) {
  int d = 0; 
  int f = 0;
  int isroot;
  char readPath[MAXPATHNAME];
  DIR *dr;
  /*open directory*/
  struct dirent *de = NULL;
  dr = opendir(filepath);
  if (dr == NULL) return -1;
  memset(readPath, '\0', MAXPATHNAME);
  /*now read it*/
  isroot = strncmp(filepath, "/", 2);
  if (state.show_hidden == 0) {
    while ((de = readdir(dr)) != NULL) {
      if (de->d_name[0] != '.') {
        strncpy(readPath, filepath, MAXPATHNAME);
        if (isroot != 0) strncat(readPath, "/", 2);
        strncat(readPath, de->d_name, strlen(de->d_name));
        if (is_directory(readPath) == 1) {
          strncpy(dir->folders[d].name, de->d_name, MAXFILENAME);
          strncpy(dir->folders[d].path, readPath, MAXPATHNAME);
          get_stats(&dir->folders[d], dir->folders[d].path, 0);
          dir->folders[d].subdir = NULL;
          d++;
        } else {
          get_stats(&dir->files[f], readPath, 1);
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
          get_stats(&dir->folders[d], dir->folders[d].path, 0);
          dir->folders[d].subdir = NULL;
          d++;
        } else {
          get_stats(&dir->files[f], readPath, 1);
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
  Directory *current;
  Directory *indexer;
  Directory *root;

  int slashCount;
  int lastSlashIndex;
  int i;
  char temp[MAXPATHNAME];
  /*if the current path IS root*/
  if (strncmp(currentPath, "/", 2) == 0) {
    Directory *root = malloc(sizeof(Directory));
    memset(root->path, '\0', MAXPATHNAME);
    strncpy(root->path, "/", 2);
    read_directory("/", root);
    root->parent = malloc(sizeof(Directory));
    read_directory(NULL, root->parent);
    return root;
  }
  current = malloc(sizeof(Directory));
  slashCount = 0;
  lastSlashIndex = last_slash_index(currentPath);
  for (i = 0; i < (int)strlen(currentPath); i++) {
    if (currentPath[i] == '/') slashCount++;
  }
  /*start at current dir, progress back to root*/
  read_directory(currentPath, current);
  memset(temp, '\0', MAXPATHNAME);
  strncpy(temp, currentPath, lastSlashIndex);
  indexer = current;
  /*go backward one directory at a time*/
  for (i = slashCount; i > 0; i--) {
    int s;
    if (slashCount > 1) {
      Directory *parent = malloc(sizeof(Directory));
      read_directory(temp, parent);
      for (s = 0; s < parent->folderc; s++) {
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
      int s;
      /*is root dir*/
      root = malloc(sizeof(Directory));
      memset(root->path, '\0', MAXPATHNAME);
      strncpy(root->path, "/", 2);
      read_directory("/", root);
      for (s = 0; s < root->folderc; s++) {
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
  char *p;
  strncpy(s, in, MAXFILENAME);
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

void get_stats(void *ptr, char *path, int is_file) {
  struct stat filestat;
  struct tm ts;
  time_t time;
  
  stat(path, &filestat);
  if (is_file) {
    File *file = (File *) ptr;
    if (access(path, X_OK) == 0) file->type = 'e';
    file->inode = filestat.st_ino;
    file->preview = NULL;
    file->ownerUID = filestat.st_uid;
    file->bytesize = (uint64_t) filestat.st_size;
    file->date_unix = filestat.st_mtime;

    time = (time_t) file->date_unix;
    ts = *localtime(&time);
    strftime(file->date, sizeof(file->date), DATE_FORMAT, &ts);
  } else if (is_file == 0) {
    Folder *dir = (Folder *) ptr;
    dir->inode = filestat.st_ino;
    dir->ownerUID = filestat.st_uid;
    /*file->bytesize = (uint64_t) filestat.st_size;*/
    dir->date_unix = filestat.st_mtime;

    time = (time_t) dir->date_unix;
    ts = *localtime(&time);
    strftime(dir->date, sizeof(dir->date), DATE_FORMAT, &ts);
  }
}

int is_directory(const char *path) {
    struct stat pathstat;
    stat(path, &pathstat);
    return S_ISDIR(pathstat.st_mode);
}
/*returns the number of folders and files in a directory*/
void get_dir_counts(int *folderc, int *filec, const char *fp) {
  char filepath[MAXPATHNAME];
  char temp[MAXPATHNAME];
  struct dirent *de = NULL;
  DIR *dir = NULL;
  
  memset(filepath, '\0', MAXPATHNAME);
  strncpy(filepath, fp, MAXPATHNAME);
  dir = opendir(filepath);
  
  if (dir == NULL) return;
  if (strncmp(fp, "/", 2) != 0) strncat(filepath, "/", 2);
  
  if (state.show_hidden == 0) {
    while ((de = readdir(dir)) != NULL) {
      if (de->d_name[0] != '.') {
        
        memset(temp, '\0', MAXPATHNAME);
        strncpy(temp, filepath, MAXPATHNAME);
        strcat(temp, de->d_name);
        if (is_directory(temp))
          *folderc+=1;
        else
          *filec+=1;
      }
    }
  } else {
    while ((de = readdir(dir)) != NULL) {
      if (strncmp(de->d_name, ".", 2) != 0 && strncmp(de->d_name, "..", 3) != 0) {  
        memset(temp, '\0', MAXPATHNAME);
        strcpy(temp, filepath);
        strcat(temp, de->d_name);
        if (is_directory(temp)) {
          *folderc+=1;
        } else {
          *filec+=1;
        }
      }
    }
  }
  closedir(dir);
}
