#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "display.h"
#include "data.h"
#include "config.h"

/*utility functions*/
void draw_window(WINDOW *win, int width, int height, Directory *dir, int mode);
char *get_file_preview(char *filePath);
void read_selected(Directory *dir);

void handle_input(int key, Directory *dir, Directory **dirptr) {
  switch(key) {
    case key_left:
      if (strncmp(dir->path, "/", 2) != 0) {
        if (chdir("..") == 0) {
          Directory *temp = dir;
          if (dir->parent->selected < dir->parent->folderCount 
          && dir->parent->folders[dir->parent->selected].subdir == NULL) {
            dir->parent->folders[dir->parent->selected].subdir = temp;
          }
          temp = dir->parent;
          *dirptr = temp;
        }
      }
      break;
    case key_down:
      if (dir->selected < dir->folderCount+dir->fileCount-1) dir->selected++;
      if (dir->selected < dir->folderCount && dir->folders[dir->selected].subdir == NULL) {
        Directory *temp = malloc(sizeof(Directory));
        read_directory(dir->folders[dir->selected].path, temp);
        if (temp->broken == 0) {
          temp->parent = dir;
          dir->folders[dir->selected].subdir = temp;
        }
      }
      break;
    case key_up:
      if (dir->selected > 0) dir->selected--;
      if (dir->selected < dir->folderCount && (dir->folders[dir->selected].subdir == NULL)) {
        Directory *temp = malloc(sizeof(Directory));
        read_directory(dir->folders[dir->selected].path, temp);
        if (temp->broken == 0) {
          temp->parent = dir;
          dir->folders[dir->selected].subdir = temp;
        }
      }
      break;
    case key_right:
      if (dir->selected < dir->folderCount) {
        
        if (dir->folders[dir->selected].subdir != NULL) {
          Directory *temp = dir;
          if (chdir(dir->folders[dir->selected].path) == 0) {
            temp = dir->folders[dir->selected].subdir;
            *dirptr = temp;
          }
          read_selected(temp);
        } 
      }
      break;
    case key_show_border:
      state.showBorder = state.showBorder == 1 ? 0 : 1; 
      break;
    case key_quit:
      state.isRunning = 0;
      break;
    case key_update:
      update_directory(dir);
      read_selected(dir);
      break;
    default:
      break;
  }
}

void read_selected(Directory *temp) {
  /*if new dir, read next*/
  if (temp->selected < temp->folderCount 
  && (temp->folders[temp->selected].subdir == NULL)) {
    Directory *subtemp = malloc(sizeof(Directory));
    read_directory(temp->folders[temp->selected].path, subtemp);
    if (subtemp->broken == 0) {
      subtemp->parent = temp;
      temp->folders[temp->selected].subdir = subtemp;
    } else {
      subtemp = NULL;
      free(subtemp);
    }
  /*if only file, get preview*/
  } else if (temp->selected < (temp->fileCount + temp->folderCount) 
  && temp->files[temp->selected].preview == NULL) {
    temp->files[temp->selected].preview = get_file_preview(temp->files[temp->selected].path);
  }
}

char *get_file_preview(char *filePath) {
  FILE *fp = fopen(filePath, "r");
  if (fp == NULL) return NULL;

  fseek(fp, 0, SEEK_END);
  int length = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (length > MAXPREVIEWSIZE) length = MAXPREVIEWSIZE;

  char *buffer = malloc(sizeof(char) * length+1);
  char c;
  int i = 0;
  while( (c = fgetc(fp)) != EOF && ftell(fp) < length ) {
    buffer[i] = c;
    i++;
  }
  /*is the file a text file? If not, return NULL*/
  if (memchr(buffer, '\0', length-1) != NULL) {
    free(buffer);
    fclose(fp);
    return NULL;
  } 
  buffer[i] = '\0';
	
  fclose(fp);

  return buffer;
}

Display *init_display(Directory *dir) {	
  Display *dis = malloc(sizeof(Display));
  initscr();
  cbreak();
  curs_set(0);
  nodelay(dis->mainWin, TRUE);
  if (has_colors()) start_color();
  use_default_colors();
  /*init colors*/
  init_pair(RED, COLOR_RED, BACKGROUND);
  init_pair(GREEN, COLOR_GREEN, BACKGROUND);
  init_pair(BLUE, COLOR_BLUE, BACKGROUND);
  init_pair(CYAN, COLOR_CYAN, BACKGROUND);
  init_pair(PURPLE, COLOR_MAGENTA, BACKGROUND);
  init_pair(YELLOW, COLOR_YELLOW, BACKGROUND);
  init_pair(WHITE, COLOR_WHITE, BACKGROUND);
  init_pair(BLACK, COLOR_BLACK, BACKGROUND);

  dis->mainWinWidth = COLS/2 - (COLS/8);
  dis->leftWinWidth = COLS/8;
  dis->rightWinWidth = COLS/2;
  
  dis->root = newwin(LINES, COLS, 0, 0);
  dis->leftWin = newwin(LINES-1, dis->leftWinWidth, 1, 0);
  dis->mainWin = newwin(LINES-1, dis->mainWinWidth, 1, dis->leftWinWidth);
  dis->rightWin = newwin(LINES-1, dis->rightWinWidth, 1, COLS/2);
  
  keypad(dis->mainWin, true);

  wattron(dis->leftWin, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->mainWin, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->rightWin, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->mainWin, A_BOLD);
  wattron(dis->leftWin, A_BOLD);
  wattron(dis->rightWin, A_BOLD);

  if (dir->selected < dir->folderCount && dir->folders[dir->selected].subdir == NULL) {
    Directory *subtemp = malloc(sizeof(Directory));
    read_directory(dir->folders[dir->selected].path, subtemp);
    if (subtemp->broken == 0) {
      subtemp->parent = dir;
      dir->folders[dir->selected].subdir = subtemp;
    } else {
      subtemp = NULL;
      free(subtemp);
    }
  }
  return dis;
}

void get_updates(Display *dis) {
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);
  /*resize window width/height if change in terminal size*/
  if (dis->width != w.ws_col || dis->height != w.ws_row) {
    dis->width = w.ws_col;
    if (state.showBorder) dis->height = w.ws_row -1;
    else dis->height = w.ws_row - abs(1-state.showBorder);
    dis->mainWinWidth = w.ws_col/2 - (w.ws_col/8);
    dis->leftWinWidth = w.ws_col/8;
    dis->rightWinWidth = w.ws_col/2;
    wresize(dis->leftWin, dis->height, dis->leftWinWidth);
    wresize(dis->mainWin, dis->height, dis->mainWinWidth);
    wresize(dis->rightWin, dis->height, dis->rightWinWidth);

    mvwin(dis->leftWin, 1, 0);
    mvwin(dis->mainWin, 1, dis->leftWinWidth);
    mvwin(dis->rightWin, 1, w.ws_col/2);
  }
}

void draw_window(WINDOW *win, int width, int height, Directory *dir, int mode) {
  switch (mode) {
    case 0:
      char buffer[256];
      int shiftview = 0;
      /* total files > window height */
      if ((dir->folderCount+dir->fileCount > height-1-2*state.showBorder) 
      && (dir->selected+state.shiftPos > height-1-2*state.showBorder-1)) 
        shiftview = dir->selected+state.shiftPos - height+1+2*state.showBorder;
        /*printw("\tShift: %d\t", shiftview);*/
      if (width > 0) {
        /*main window folders*/
        wattron(win, COLOR_PAIR(DIRCOLOR));
        int i;
        for (i = 0; i < dir->folderCount; i++) {
          if ((int)strlen(dir->folders[i].name) >width-2*state.showBorder)
            strncpy(buffer, dir->folders[i].name,width-2*state.showBorder);
          else
            strncpy(buffer, dir->folders[i].name,width-2*state.showBorder);
          buffer[strlen(buffer)] = '\0';
          mvwaddstr(win, i+state.showBorder-shiftview, state.showBorder, buffer);
          memset(buffer,0,255);
        } 
        wattroff(win, COLOR_PAIR(DIRCOLOR));
        /*main window files*/
        wattron(win, COLOR_PAIR(FILECOLOR));
        for (i = 0; i < dir->fileCount; i++) {
          if (dir->files[i].type == 'e')
            wattron(win, COLOR_PAIR(EXECOLOR));
          else
            wattron(win, COLOR_PAIR(FILECOLOR));
 
          if ((int)strlen(dir->files[i].name) >width-2)
            strncpy(buffer, dir->files[i].name,width-2);
          else
            strncpy(buffer, dir->files[i].name,width-2);
          buffer[strlen(buffer)] = '\0';
          mvwaddstr(win, dir->folderCount+i+state.showBorder-shiftview, 
              state.showBorder, buffer);
          memset(buffer,0,255);
        }
        wattroff(win, COLOR_PAIR(FILECOLOR));
        wattroff(win, COLOR_PAIR(EXECOLOR));
        mvwchgat(win, dir->selected+state.showBorder-shiftview, 
        state.showBorder, width-1-state.showBorder, A_STANDOUT | A_BOLD, CURSORCOLOR, NULL);
      } 
      if (state.showBorder) {
        wattron(win, COLOR_PAIR(BORDERCOLOR));
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(BORDERCOLOR));
      }
  
      break;
    case 1:
      /* PREVIEW MODE */
      /*display currently selected file preview*/
      if (dir->selected > dir->folderCount && dir->selected < dir->folderCount + dir->fileCount) {
        wattron(win, COLOR_PAIR(TEXTCOLOR));
        if (dir->files[dir->selected - dir->folderCount].preview == NULL) 
          dir->files[dir->selected - dir->folderCount].preview = get_file_preview(dir->files[dir->selected - dir->folderCount].path);
        if (dir->files[dir->selected - dir->folderCount].preview != NULL) 
          mvwprintw(win, state.showBorder, state.showBorder, "%s", dir->files[dir->selected - dir->folderCount].preview);
        else
          mvwprintw(win, state.showBorder, state.showBorder, "%s\nOwner: %s\nSize: %lld Bytes\nDate: %s\n", 
              dir->files[dir->selected - dir->folderCount].name,
              dir->files[dir->selected - dir->folderCount].owner,
              dir->files[dir->selected - dir->folderCount].bytesize,
              dir->files[dir->selected - dir->folderCount].date);
        wattroff(win, COLOR_PAIR(TEXTCOLOR));
      }
      break;
    default:
      break;
  }
}

void update_display(Display *dis, Directory **dirptr) {
  Directory *dir = *dirptr;
  WINDOW *leftWin = dis->leftWin;
  WINDOW *mainWin = dis->mainWin;
  WINDOW *rightWin = dis->rightWin;
  Directory *top = dir->parent;
  
  werase(leftWin);
  werase(mainWin);
  werase(rightWin);
 
  attron(A_BOLD);
  attron(COLOR_PAIR(TITLECOLOR));
  /*print currently selected directory path*/
  if (dir->selected < dir->folderCount)
    printw("%s", dir->folders[dir->selected].path);
  else if (dir->selected < (dir->folderCount + dir->fileCount))
    printw("%s", dir->files[dir->selected - dir->folderCount].path);
  /*draw three windows*/
  draw_window(mainWin, dis->mainWinWidth, dis->height, dir, DIR_MODE);
  draw_window(leftWin, dis->leftWinWidth, dis->height, top, DIR_MODE);
  if (dir->selected < dir->folderCount && dir->folders[dir->selected].subdir != NULL)
    draw_window(rightWin, dis->rightWinWidth, dis->height, dir->folders[dir->selected].subdir, DIR_MODE);
  else if (dir->selected > dir->folderCount && dir->selected < dir->folderCount + dir->fileCount)
    draw_window(rightWin, dis->rightWinWidth, dis->height, dir, PREVIEW_MODE);

  refresh();  
  wrefresh(leftWin);
  wrefresh(rightWin);
  wrefresh(mainWin);
  /*wait for user input*/
  int key = wgetch(mainWin);
  if (key != ERR)
    handle_input(key, dir, dirptr);

  attroff(COLOR_PAIR(TITLECOLOR));
  attroff(A_BOLD);
 
  erase();
}

void kill_display(Display *dis) {
  free(dis);
	endwin();
}
