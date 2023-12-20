#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "display.h"
#include "data.h"
#include "config.h"

char *getFilePreview(char *filePath);

void handleInput(int key, /*Display *dis,*/ Directory *dir, Directory **dirptr) {
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
      if(dir->selected < dir->folderCount+dir->fileCount-1/* && dir->selected < dis->height-4*/) dir->selected++;
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
          /*if new dir, read next*/
          if(temp->selected < temp->folderCount && (temp->folders[temp->selected].subdir == NULL)) {
            Directory *subtemp = malloc(sizeof(Directory));
            readDir(temp->folders[temp->selected].path, subtemp);
            if(subtemp->doNotUse == 0) {
              subtemp->parent = temp;
              temp->folders[temp->selected].subdir = subtemp;
            } else {
              subtemp = NULL;
              free(subtemp);
            }
          /*if only file, get preview*/
          } else if(temp->selected < temp->fileCount && temp->files[temp->selected].preview == NULL) {
            temp->files[temp->selected].preview = getFilePreview(temp->files[temp->selected].path);
          }
        } 
      }
      break;
    case 'b':
      state.showBorder = state.showBorder == 1 ? 0 : 1; 
      break;
    case 'q':
      state.isRunning = 0;
      break;
    case ':':

      break;
    default:
      break;
  }
}

char *getFilePreview(char *filePath) {
  FILE *fp = fopen(filePath, "r");
  if(fp == NULL) return NULL;

  fseek(fp, 0, SEEK_END);
  int length = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if(length > MAXPREVIEWSIZE) length = MAXPREVIEWSIZE;

  char *buffer = malloc(sizeof(char) * length+1);
  char c;
  int i = 0;
  while( (c = fgetc(fp)) != EOF && ftell(fp) < length ) {
    buffer[i] = c;
    i++;
  }
  /*is the file a text file? If not, return NULL*/
  if(memchr(buffer, '\0', length-1) != NULL) {
    /* do shit */
    //memset(buffer,'\0',length);
    free(buffer);
    fclose(fp);
    return NULL;
  } 
  buffer[i] = '\0';
	
  fclose(fp);

  return buffer;
}

Display *initDisplay(Directory *dir) {	
  Display *dis = malloc(sizeof(Display));
  initscr();
  cbreak();
  curs_set(0);
  nodelay(dis->mainWin, TRUE);
  if(has_colors()) start_color();
  use_default_colors();
  init_pair(1, COLOR_GREEN, -1);
  init_pair(2, COLOR_WHITE, -1);
  init_pair(3, COLOR_MAGENTA, -1);
  init_pair(4, COLOR_RED, -1);
  init_pair(5, COLOR_RED, -1);

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
    Directory *subtemp = malloc(sizeof(Directory));
    readDir(dir->folders[dir->selected].path, subtemp);
    if(subtemp->doNotUse == 0) {
      subtemp->parent = dir;
      dir->folders[dir->selected].subdir = subtemp;
    } else {
      subtemp = NULL;
      free(subtemp);
    }
  }
  return dis;
}

void checkUpdates(Display *dis) {
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);
  /*resize window width/height if change in terminal size*/
  if(dis->width != w.ws_col || dis->height != w.ws_row) {
    dis->width = w.ws_col;
    if(state.showBorder) dis->height = w.ws_row -1;
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
  /*print currently selected directory path*/
  if(dir->selected < dir->folderCount) printw("%s", dir->folders[dir->selected].path);
  else if(dir->selected < (dir->folderCount + dir->fileCount)) printw("%s", dir->files[dir->selected - dir->folderCount].path);

  /* display directories */
  char leftBuff[256];
  char mainBuff[256];
  char rightBuff[256];
  int mainShiftView = 0;
  //if(dir->selected+1 > dis->height-1-2*state.showBorder)
    /* total files > window height */
  if((dir->folderCount+dir->fileCount > dis->height-1-2*state.showBorder) && (dir->selected+state.shiftPos > dis->height-1-2*state.showBorder-1)) 
    mainShiftView = dir->selected+state.shiftPos - dis->height+1+2*state.showBorder;
  printw("\tShift: %d\t", mainShiftView);
  if(dis->mainWinWidth-2 > 0) {
    /*main window folders*/
    wattron(mainWin, COLOR_PAIR(3));
    for(int i = 0; i < dir->folderCount /*&& i < dis->height-1-2*state.showBorder*/; i++) {
      if((int)strlen(dir->folders[i].name) > dis->mainWinWidth-2*state.showBorder) strncpy(mainBuff, dir->folders[i].name, dis->mainWinWidth-2*state.showBorder);
      else strncpy(mainBuff, dir->folders[i].name, dis->mainWinWidth-2*state.showBorder);
      mainBuff[strlen(mainBuff)] = '\0';
      mvwaddstr(mainWin, i+state.showBorder-mainShiftView, state.showBorder, mainBuff);
      memset(mainBuff,0,255);
    } 
    wattroff(mainWin, COLOR_PAIR(3));
    /*main window files*/
    wattron(mainWin, COLOR_PAIR(2));
    for(int i = 0; i < dir->fileCount /*&& i+dir->folderCount < dis->height-state.showBorder*/; i++) {
      if(dir->files[i].type == 'e') wattron(mainWin, COLOR_PAIR(1));
      else wattron(mainWin, COLOR_PAIR(2));
 
      if((int)strlen(dir->files[i].name) > dis->mainWinWidth-2) strncpy(mainBuff, dir->files[i].name, dis->mainWinWidth-2);
      else strncpy(mainBuff, dir->files[i].name, dis->mainWinWidth-2);
      mainBuff[strlen(mainBuff)] = '\0';
      mvwaddstr(mainWin, dir->folderCount+i+state.showBorder-mainShiftView, state.showBorder, mainBuff);
      memset(mainBuff,0,255);
      /*display file info*/
      if(i == dir->selected - dir->folderCount) {
        printw("\t%lld Bytes", dir->files[dir->selected - dir->folderCount].byteSize);
      }

    }
    wattroff(mainWin, COLOR_PAIR(2));
    wattroff(mainWin, COLOR_PAIR(3));
    mvwchgat(mainWin, dir->selected+state.showBorder-mainShiftView, state.showBorder, dis->mainWinWidth-1-state.showBorder, A_STANDOUT | A_BOLD, 3, NULL);
  }
  /*left window folders*/
  if(dis->leftWinWidth-2 > 0) {
    wattron(leftWin, COLOR_PAIR(3));
    for(int i = 0; i < top->folderCount && i < dis->height-3; i++) {
      if((int)strlen(top->folders[i].name) > dis->leftWinWidth-2) strncpy(leftBuff, top->folders[i].name, dis->leftWinWidth-2); 
      else strncpy(leftBuff, top->folders[i].name, dis->leftWinWidth-2);
      leftBuff[strlen(leftBuff)] = '\0';
      mvwaddstr(leftWin, i+state.showBorder, state.showBorder, leftBuff);
      memset(leftBuff,0,255);
    }
    wattroff(leftWin, COLOR_PAIR(3));
    /*left window files*/
    wattron(leftWin, COLOR_PAIR(2));
    for(int i = 0; i < top->fileCount && i+top->folderCount < dis->height-3; i++) {
      if(top->files[i].type == 'e') wattron(leftWin, COLOR_PAIR(1));
      else wattron(leftWin, COLOR_PAIR(2));
 
      if((int)strlen(top->files[i].name) > dis->leftWinWidth-2) strncpy(leftBuff, top->files[i].name, dis->leftWinWidth-2); 
      else strncpy(leftBuff, top->files[i].name, dis->leftWinWidth-2);
      leftBuff[strlen(leftBuff)] = '\0';
      mvwaddstr(leftWin, top->folderCount+i+state.showBorder, state.showBorder, leftBuff);
      memset(leftBuff,0,255);
    }
    wattroff(leftWin, COLOR_PAIR(2));
    if(top->files[top->selected].type == 'z') mvwchgat(leftWin, top->selected+state.showBorder, state.showBorder, 
                                                          dis->leftWinWidth-1-state.showBorder, A_BOLD, 4, NULL); 
    else mvwchgat(leftWin, top->selected+state.showBorder, state.showBorder, dis->leftWinWidth-1-state.showBorder, 
                                                          A_STANDOUT | A_BOLD, 3, NULL);
  }
  if(dis->rightWinWidth-2 > 0) {
    /*folder is selected */
    /*display subdir folders*/
    if(dir->selected < dir->folderCount && dir->folders[dir->selected].subdir != NULL) {
      int folderCount = dir->folders[dir->selected].subdir->folderCount; 
      int fileCount = dir->folders[dir->selected].subdir->fileCount;
      wattron(rightWin, COLOR_PAIR(3));
      for(int i = 0; i < folderCount && i < dis->height-3; i++) {
        int nameLength = strlen(dir->folders[dir->selected].subdir->folders[i].name);
        if(nameLength > dis->rightWinWidth-2) strncpy(rightBuff, dir->folders[dir->selected].subdir->folders[i].name, dis->rightWinWidth-2); 
        else strncpy(rightBuff, dir->folders[dir->selected].subdir->folders[i].name, dis->rightWinWidth-2);
        rightBuff[nameLength] = '\0';
 
        mvwprintw(rightWin, i+state.showBorder, state.showBorder, "%s", rightBuff);
        memset(rightBuff,0,255);
      } 

      wattroff(rightWin, COLOR_PAIR(3));
      /*display subdir files*/
      wattron(rightWin, COLOR_PAIR(2));
      for(int i = 0; i < fileCount && i+folderCount < dis->height-3; i++) {
        if(dir->folders[dir->selected].subdir->files[i].type == 'e') wattron(rightWin, COLOR_PAIR(1));
        else wattron(rightWin, COLOR_PAIR(2));
 
        int nameLength = strlen(dir->folders[dir->selected].subdir->files[i].name);
        if(nameLength > dis->rightWinWidth-2) strncpy(rightBuff, dir->folders[dir->selected].subdir->files[i].name, dis->rightWinWidth-2); 
        else strncpy(rightBuff, dir->folders[dir->selected].subdir->files[i].name, dis->rightWinWidth-2);
        rightBuff[nameLength] = '\0';

        mvwprintw(rightWin, folderCount+i+state.showBorder, state.showBorder, "%s",  rightBuff);
        memset(rightBuff,0,255);
      }
      wattroff(leftWin, COLOR_PAIR(2)); 
      mvwchgat(rightWin, dir->folders[dir->selected].subdir->selected+state.showBorder, state.showBorder, dis->rightWinWidth-1-state.showBorder, A_STANDOUT | A_BOLD, 3, NULL);
    /*display currently selected file preview*/
    } else if(dir->selected > dir->folderCount && dir->selected < dir->folderCount + dir->fileCount) {
      if(dir->files[dir->selected - dir->folderCount].preview == NULL) 
        dir->files[dir->selected - dir->folderCount].preview = getFilePreview(dir->files[dir->selected - dir->folderCount].path);
      if(dir->files[dir->selected - dir->folderCount].preview != NULL) 
        mvwprintw(rightWin, state.showBorder, state.showBorder, "%s", dir->files[dir->selected - dir->folderCount].preview);
      else
        mvwprintw(rightWin, state.showBorder, state.showBorder, "%s\nOwner: %d\nSize: %lld Bytes\nDate: %lld\n", 
            dir->files[dir->selected - dir->folderCount].name,
            dir->files[dir->selected - dir->folderCount].ownerUID,
            dir->files[dir->selected - dir->folderCount].byteSize,
            dir->files[dir->selected - dir->folderCount].date);
    }
  }

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

  refresh();  
  wrefresh(leftWin);
  wrefresh(rightWin);
  wrefresh(mainWin);
  /*wait for user input*/
  int key = wgetch(mainWin);
  if(key != ERR) handleInput(key, /*dis,*/ dir, dirptr);

  attroff(COLOR_PAIR(1));
  attroff(A_BOLD);
 
  erase();
}

void killDisplay(Display *dis) {
  free(dis);
	endwin();
}
