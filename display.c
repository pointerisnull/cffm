#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "display.h"
#include "config.h"

void handleInput(int key, Display *dis, Directory *dir, Directory **dirptr) {
  switch(key) {
    case 'h':
      if(strncmp(dir->path, "/", 2) != 0) {
        if(chdir("..") == 0) {
          Directory *temp = dir;
          if(dir->parent->selected < dir->parent->folderCount && dir->parent->folders[dir->parent->selected].subdir == NULL) {
            dir->parent->folders[dir->parent->selected].subdir = temp;
          }
          temp = dir->parent;
          *dirptr = temp;
        }
      }
      break;
    
    case 'j':
      if(dir->selected < dir->folderCount+dir->fileCount-1 && dir->selected < dis->height-4) dir->selected++;
      if(dir->selected < dir->folderCount && dir->folders[dir->selected].subdir == NULL) {
        Directory *temp = malloc(sizeof(Directory));
        readDir(dir->folders[dir->selected].path, temp);
        if(temp->doNotUse == 0) {
          temp->parent = dir;
          dir->folders[dir->selected].subdir = temp;
        }
      }
      break;
    case 'k':
      if(dir->selected > 0) dir->selected--;
      if(dir->selected < dir->folderCount && (dir->folders[dir->selected].subdir == NULL)) {
        Directory *temp = malloc(sizeof(Directory));
        readDir(dir->folders[dir->selected].path, temp);
        if(temp->doNotUse == 0) {
          temp->parent = dir;
          dir->folders[dir->selected].subdir = temp;
        }
      }
      break;
    case 'l':
      if(dir->selected < dir->folderCount) {
        
        if(dir->folders[dir->selected].subdir != NULL) {
          Directory *temp = dir;
          if(chdir(dir->folders[dir->selected].path) == 0) {
            temp = dir->folders[dir->selected].subdir;
            *dirptr = temp;
          }
          //if new dir, read next
          if(temp->selected < temp->folderCount && (temp->folders[temp->selected].subdir == NULL)) {
            Directory *subtemp = malloc(sizeof(Directory));
            readDir(temp->folders[temp->selected].path, subtemp);
            if(subtemp->doNotUse == 0) {
              subtemp->parent = temp;
              temp->folders[temp->selected].subdir = subtemp;
            }
          }
          //
        } 
      }
      break;
    case 'b':
      state.showBorder = state.showBorder == 1 ? 0 : 1; 
      break;
    case 'q':
      state.isRunning = 0;
      break;
    default:
      break;

  }
}

void initDisplay(Display *dis, Directory *dir) {	
  initscr();
  cbreak();
  curs_set(0);
  nodelay(dis->mainWin, TRUE);
  if(has_colors()) start_color();
  use_default_colors();
  init_pair(1, COLOR_GREEN, -1);
  init_pair(2, COLOR_WHITE, -1);
  init_pair(3, COLOR_MAGENTA, -1);
  //assume_default_colors(3);
  dis->mainWinWidth = COLS/2 - (COLS/8);
  dis->leftWinWidth = COLS/8;
  dis->rightWinWidth = COLS/2;
  
  dis->root = newwin(LINES, COLS, 0, 0);
  dis->leftWin = newwin(LINES-1, dis->leftWinWidth, 1, 0);
  dis->mainWin = newwin(LINES-1, dis->mainWinWidth, 1, dis->leftWinWidth);
  dis->rightWin = newwin(LINES-1, dis->rightWinWidth, 1, COLS/2);
  
  keypad(dis->mainWin, true);

  wattron(dis->leftWin, COLOR_PAIR(3));
  wattron(dis->mainWin, COLOR_PAIR(3));
  wattron(dis->rightWin, COLOR_PAIR(3));
  wattron(dis->mainWin, A_BOLD);
  wattron(dis->leftWin, A_BOLD);
  wattron(dis->rightWin, A_BOLD);

  if(dir->selected < dir->folderCount && dir->folders[dir->selected].subdir == NULL) {
    dir->folders[dir->selected].subdir = malloc(sizeof(Directory));
    dir->folders[dir->selected].subdir->parent = dir;
    readDir(dir->folders[dir->selected].path, dir->folders[dir->selected].subdir);
  }
 
}

void checkUpdates(Display *dis) {
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);

  if(dis->width != w.ws_col || dis->height != w.ws_row) {
    dis->width = w.ws_col;
    dis->height = w.ws_row;
    dis->mainWinWidth = w.ws_col/2 - (w.ws_col/8);
    dis->leftWinWidth = w.ws_col/8;
    dis->rightWinWidth = w.ws_col/2;
    wresize(dis->leftWin, w.ws_row-1, dis->leftWinWidth);
    wresize(dis->mainWin, w.ws_row-1, dis->mainWinWidth);
    wresize(dis->rightWin, w.ws_row-1, dis->rightWinWidth);
    
    mvwin(dis->leftWin, 1, 0);
    mvwin(dis->mainWin, 1, dis->leftWinWidth);
    mvwin(dis->rightWin, 1, w.ws_col/2);
  }
}

void display(Display *dis, Directory **dirptr) {
  Directory *dir = *dirptr;
  WINDOW *leftWin = dis->leftWin;
  WINDOW *mainWin = dis->mainWin;
  WINDOW *rightWin = dis->rightWin;
  Directory *top = dir->parent;
  
  werase(leftWin);
  werase(mainWin);
  werase(rightWin);
 
  attron(A_BOLD);
  attron(COLOR_PAIR(1));

  if(dir->selected < dir->folderCount) printw("%s", dir->folders[dir->selected].path);
  else if(dir->selected < (dir->folderCount + dir->fileCount)) printw("%s", dir->files[dir->selected - dir->folderCount].path);

  if(state.showBorder) {
    wattron(mainWin, COLOR_PAIR(3));
    wattron(leftWin, COLOR_PAIR(3));
    wattron(rightWin, COLOR_PAIR(3));
    box(leftWin, 0, 0);
    box(mainWin, 0, 0);
    box(rightWin, 0, 0);
    wattroff(mainWin, COLOR_PAIR(3));
    wattroff(leftWin, COLOR_PAIR(3));
    wattroff(rightWin, COLOR_PAIR(3)); 
  }
 
  /* display directories */

  char leftBuff[256];
  char mainBuff[256];
  char rightBuff[256];

  if(dis->mainWinWidth-2 > 0) {
    wattron(mainWin, COLOR_PAIR(3));
    for(int i = 0; i < dir->folderCount && i < dis->height-3; i++) {
      if(strlen(dir->folders[i].name) > dis->mainWinWidth-2) strncpy(mainBuff, dir->folders[i].name, dis->mainWinWidth-2);
      else strncpy(mainBuff, dir->folders[i].name, dis->mainWinWidth-2);
      mainBuff[strlen(mainBuff)] = '\0';
      mvwaddstr(mainWin, i+1, 1, mainBuff);
      memset(mainBuff,0,255);
    } 
    wattroff(mainWin, COLOR_PAIR(3));
    wattron(mainWin, COLOR_PAIR(2));
    for(int i = 0; i < dir->fileCount && i+dir->folderCount < dis->height-3; i++) {
      if(dir->files[i].type == 'e') wattron(mainWin, COLOR_PAIR(1));
      else wattron(mainWin, COLOR_PAIR(2));
    
      if(strlen(dir->files[i].name) > dis->mainWinWidth-2) strncpy(mainBuff, dir->files[i].name, dis->mainWinWidth-2);
      else strncpy(mainBuff, dir->files[i].name, dis->mainWinWidth-2);
      mainBuff[strlen(mainBuff)] = '\0';
      mvwaddstr(mainWin, dir->folderCount+i+1, 1, mainBuff);
      memset(mainBuff,0,255);

    }
    wattroff(mainWin, COLOR_PAIR(2));
    wattroff(mainWin, COLOR_PAIR(3));
    mvwchgat(mainWin, dir->selected+1, 1, dis->mainWinWidth-1-state.showBorder, A_STANDOUT | A_BOLD, 3, NULL);
  }
  if(dis->leftWinWidth-2 > 0) {
    wattron(leftWin, COLOR_PAIR(3));
    for(int i = 0; i < top->folderCount && i < dis->height-3; i++) {
      if(strlen(top->folders[i].name) > dis->leftWinWidth-2) strncpy(leftBuff, top->folders[i].name, dis->leftWinWidth-2); 
      else strncpy(leftBuff, top->folders[i].name, dis->leftWinWidth-2);
      leftBuff[strlen(leftBuff)] = '\0';
      mvwaddstr(leftWin, i+1, 1, leftBuff);
      memset(leftBuff,0,255);
    }
    wattroff(leftWin, COLOR_PAIR(3));

    wattron(leftWin, COLOR_PAIR(2));
    for(int i = 0; i < top->fileCount && i+top->folderCount < dis->height-3; i++) {
      if(strlen(top->files[i].name) > dis->leftWinWidth-2) strncpy(leftBuff, top->files[i].name, dis->leftWinWidth-2); 
      else strncpy(leftBuff, top->files[i].name, dis->leftWinWidth-2);
      leftBuff[strlen(leftBuff)] = '\0';
      mvwaddstr(leftWin, top->folderCount+i+1, 1, leftBuff);
      memset(leftBuff,0,255);
    }
    wattroff(leftWin, COLOR_PAIR(2));
    mvwchgat(leftWin, top->selected+1, 1, dis->leftWinWidth-1-state.showBorder, A_STANDOUT | A_BOLD, 3, NULL);
  }
  
  if(dis->rightWinWidth-2 > 0) {
    if(dir->selected < dir->folderCount && dir->folders[dir->selected].subdir != NULL) {
      int folderCount = dir->folders[dir->selected].subdir->folderCount; 
      int fileCount = dir->folders[dir->selected].subdir->fileCount;
      wattron(rightWin, COLOR_PAIR(3));
      for(int i = 0; i < folderCount && i < dis->height-3; i++) {
        int nameLength = strlen(dir->folders[dir->selected].subdir->folders[i].name);
        if(nameLength > dis->rightWinWidth-2) strncpy(rightBuff, dir->folders[dir->selected].subdir->folders[i].name, dis->rightWinWidth-2); 
        else strncpy(rightBuff, dir->folders[dir->selected].subdir->folders[i].name, dis->rightWinWidth-2);
        rightBuff[nameLength] = '\0';
 
        mvwprintw(rightWin, i+1, 1, "%s", rightBuff);
        memset(rightBuff,0,255);
      } 
    
      wattroff(rightWin, COLOR_PAIR(3));
    
      wattron(rightWin, COLOR_PAIR(2));
      for(int i = 0; i < fileCount && i+folderCount < dis->height-3; i++) {
        int nameLength = strlen(dir->folders[dir->selected].subdir->files[i].name);
        if(nameLength > dis->rightWinWidth-2) strncpy(rightBuff, dir->folders[dir->selected].subdir->files[i].name, dis->rightWinWidth-2); 
        else strncpy(rightBuff, dir->folders[dir->selected].subdir->files[i].name, dis->rightWinWidth-2);
        rightBuff[nameLength] = '\0';

        mvwprintw(rightWin, folderCount+i+1, 1, "%s",  rightBuff);
        memset(rightBuff,0,255);
      }
      wattroff(leftWin, COLOR_PAIR(2)); 
      mvwchgat(rightWin, dir->folders[dir->selected].subdir->selected+1, 1, dis->rightWinWidth-1-state.showBorder, A_STANDOUT | A_BOLD, 3, NULL);
    }
  }

  refresh();  
  wrefresh(leftWin);
  wrefresh(rightWin);
  wrefresh(mainWin);
  /*wait for user input*/
  int key = wgetch(mainWin);
  if(key != ERR) handleInput(key, dis, dir, dirptr);

  attroff(COLOR_PAIR(1));
  attroff(A_BOLD);
 
  erase();
}

void killDisplay() {
	endwin();
}
