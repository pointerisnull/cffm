#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "display.h"

void handleInput(int key, Display *dis, Directory *dir, Directory **dirptr) {
  switch(key) {
    case 'h':
      if(strcmp(dir->path, "/") != 0) {
        if(chdir("..") == 0) {
          Directory *temp = dir;
          if(dir->parent->selected < dir->parent->folderCount && dir->parent->folders[dir->parent->selected].subdir == NULL) {
            dir->parent->folders[dir->parent->selected].subdir = temp;
          }
          temp = dir->parent;
          if(temp->parent == NULL) {
            temp->parent = malloc(sizeof(Directory));
            readDir("..", temp->parent);
          }
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
      if(dir->selected < dir->folderCount && dir->folders[dir->selected].subdir == NULL) {
        Directory *temp = malloc(sizeof(Directory));
        readDir(dir->folders[dir->selected].path, temp);
        if(temp->doNotUse == 0) {
          temp->parent = dir;
          dir->folders[dir->selected].subdir = temp;
        }
      //  dir->folders[dir->selected].subdir = malloc(sizeof(Directory));
      //  if(dir->folders[dir->selected].subdir->parent != NULL) {
       //   dir->folders[dir->selected].subdir->parent = dir;
       //   if(dir->folders[dir->selected].subdir->path == NULL) readDir(dir->folders[dir->selected].path, dir->folders[dir->selected].subdir);
       // }
      }
      break;
    case 'l':
      if(dir->selected < dir->folderCount) {
        
        if(dir->folders[dir->selected].subdir != NULL) {
          Directory *temp = dir;
          if(chdir(dir->folders[dir->selected].path) == 0) {
          //  dir->parent->folders[dir->parent->selected].subdir = temp;
          //}
            temp = dir->folders[dir->selected].subdir;
          //temp->parent = malloc(sizeof(Directory));
          //readDir("..", temp->parent);
            *dirptr = temp;
          }
        } 
      }
      break;
    default:
      break;
  }
}

void initDisplay(Display *dis, Directory *dir) { //WINDOW *leftWin, WINDOW *mainWin, WINDOW *rightWin) {
	initscr();			/* Start curses mode 		  */
  cbreak();
  curs_set(0);
  if(has_colors()) start_color();
  use_default_colors();
  init_pair(1, COLOR_GREEN, -1);//  COLOR_BLACK);
  init_pair(2, COLOR_WHITE, -1); //COLOR_BLACK);
  init_pair(3, COLOR_MAGENTA, -1); // COLOR_BLACK);
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
	//char filePath[4096];
  WINDOW *leftWin = dis->leftWin;
  WINDOW *mainWin = dis->mainWin;
  WINDOW *rightWin = dis->rightWin;
  Directory *top = dir->parent;
  werase(leftWin);
  werase(mainWin);
  werase(rightWin);
 
  attron(A_BOLD);
  attron(COLOR_PAIR(1));
  //if(getcwd(filePath, sizeof(filePath)) != NULL) 
  printw("%s", dir->path);
  
  wattron(mainWin, COLOR_PAIR(3));
  wattron(leftWin, COLOR_PAIR(3));
  wattron(rightWin, COLOR_PAIR(3));
  box(leftWin, 0, 0);
  box(mainWin, 0, 0);
  box(rightWin, 0, 0);
  wattroff(mainWin, COLOR_PAIR(3));
  wattroff(leftWin, COLOR_PAIR(3));
  wattroff(rightWin, COLOR_PAIR(3));
  /* display directories */
  wattron(mainWin, COLOR_PAIR(3));
  for(int i = 0; i < dir->folderCount && i < dis->height-3; i++) {
    mvwaddstr(mainWin, i+1, 1, dir->folders[i].name);
  } 
  wattroff(mainWin, COLOR_PAIR(3));

  //wattron(mainWin, COLOR_PAIR(2));
  for(int i = 0; i < dir->fileCount && i+dir->folderCount < dis->height-3; i++) {
    if(dir->files[i].type == 'e') wattron(mainWin, COLOR_PAIR(1));
    else wattron(mainWin, COLOR_PAIR(2));
    mvwaddstr(mainWin, dir->folderCount+i+1, 1, dir->files[i].name);
  }
  wattroff(mainWin, COLOR_PAIR(2));
  wattroff(mainWin, COLOR_PAIR(3));
  mvwchgat(mainWin, dir->selected+1, 1, dis->mainWinWidth-2, A_STANDOUT | A_BOLD, 3, NULL);
  
  wattron(leftWin, COLOR_PAIR(3));
  for(int i = 0; i < top->folderCount && i < dis->height-3; i++) {
    mvwaddstr(leftWin, i+1, 1, top->folders[i].name);
  } 
  wattroff(leftWin, COLOR_PAIR(3));

  wattron(leftWin, COLOR_PAIR(2));
  for(int i = 0; i < top->fileCount && i+top->folderCount < dis->height-3; i++) {
    mvwaddstr(leftWin, top->folderCount+i+1, 1, top->files[i].name);
  }
  wattroff(leftWin, COLOR_PAIR(2));
  mvwchgat(leftWin, top->selected+1, 1, dis->leftWinWidth-2, A_STANDOUT | A_BOLD, 3, NULL);
///*
  if(dir->selected < dir->folderCount && dir->folders[dir->selected].subdir != NULL) {
    int folderCount = dir->folders[dir->selected].subdir->folderCount; 
    int fileCount = dir->folders[dir->selected].subdir->fileCount;
    wattron(rightWin, COLOR_PAIR(3));
    for(int i = 0; i < folderCount && i < dis->height-3; i++) {
      mvwprintw(rightWin, i+1, 1, "%s", dir->folders[dir->selected].subdir->folders[i].name);
    } 
    wattroff(rightWin, COLOR_PAIR(3));
    
    wattron(rightWin, COLOR_PAIR(2));
    for(int i = 0; i < fileCount && i+folderCount < dis->height-3; i++) {
      mvwprintw(rightWin, folderCount+i+1, 1, "%s",  dir->folders[dir->selected].subdir->files[i].name);
    }
    wattroff(leftWin, COLOR_PAIR(2)); 
    mvwchgat(rightWin, dir->folders[dir->selected].subdir->selected+1, 1, dis->rightWinWidth-2, A_STANDOUT | A_BOLD, 3, NULL);
  }
//*/
  refresh();			/* Print it on to the real screen */
  wrefresh(leftWin);
  wrefresh(rightWin);
  wrefresh(mainWin);

  int key = wgetch(mainWin);			/* Wait for user input */
  handleInput(key, dis, dir, dirptr);

  attroff(COLOR_PAIR(1));
  attroff(A_BOLD);
 
  erase();
}

void killDisplay() {
	endwin();			/* End curses mode		  */
}
