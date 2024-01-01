#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include "display.h"
#include "data.h"
#include "config.h"

/*main function defs*/
void get_updates(Display *dis);
Display *init_display(Directory *dir);
void update_display(Display *dis, Directory **dir);
void kill_display(Display *dis);
/*utility functions*/
void draw_window(WINDOW *win, int width, int height, Directory *dir, int mode, const char *buffer);
char *get_file_preview(char *filePath);
void read_selected(Directory *dir);
void cmd_window(WINDOW *win, int width, int height, Directory *current);
void exec_cmd(const char *buffer);
void favorites_window(Display *dis);

void handle_input(int key, Display *dis, Directory *dir, Directory **dirptr) {
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
        /*is file, execute it*/
      } else if (dir->selected < dir->folderCount + dir->fileCount) {
          char buffer[4096];
          strncpy(buffer, "mimeopen ", 10);
          strcat(buffer, dir->files[dir->selected-dir->folderCount].name);
          exec_cmd(buffer);
      }
      break;
    case key_show_border:
      state.show_border = state.show_border == 1 ? 0 : 1; 
      break;
    case key_quit:
      state.is_running = 0;
      break;
    case key_update:
      update_directory(dir);
      read_selected(dir);
      break;
    case key_cmd:
      cmd_window(dis->titleWin, dis->width, 1, dir);
      break;
    default:
      break;
  }
}

void exec_cmd(const char *buffer) {
  def_prog_mode();
  endwin();
  system(buffer);
  reset_prog_mode();
}

void cmd_window(WINDOW *win, int width, int height, Directory *current) {
  int key = 0;
  int i = 0;
  char buff[2048] = {'\0'};
  /*draw_window(win, width, height, NULL, CMD_MODE, NULL);*/
  while (key != '\n' && key != 27) {
    draw_window(win, width, height, NULL, CMD_MODE, buff);
    key = wgetch(win);
    if (key != ERR) {
      if (key != 127) {
        buff[i] = key;
        i++;
      } else if (i > 0) {
        i--;
        buff[i] = '\0';
      }
    }
  }
  if (key == '\n') {
    exec_cmd(buff);
    update_directory(current);
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
  int length;
  int i = 0;
  char c;
  char *buffer;

  if (fp == NULL) return NULL;

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (length > MAXPREVIEWSIZE) length = MAXPREVIEWSIZE;

  buffer = malloc(sizeof(char) * length+1);
  while ( (c = fgetc(fp)) != EOF && ftell(fp) < length ) {
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
  noecho();
  /*nodelay(dis->mainWin, TRUE);*/
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
  dis->previewWidth = COLS/2 - (2*state.show_border);
  
  dis->root = newwin(LINES, COLS, 0, 0);
  dis->leftWin = newwin(LINES-1, dis->leftWinWidth, 1, 0);
  dis->mainWin = newwin(LINES-1, dis->mainWinWidth, 1, dis->leftWinWidth);
  dis->rightWin = newwin(LINES-1, dis->rightWinWidth, 1, COLS/2);
  dis->titleWin = newwin(LINES-1, dis->width, 0, 0);
  dis->previewWin = newwin(LINES-1, dis->previewWidth, 1, COLS/2+state.show_border);
  
  wattron(dis->leftWin, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->mainWin, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->rightWin, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->titleWin, COLOR_PAIR(TITLECOLOR));
  wattron(dis->mainWin, A_BOLD);
  wattron(dis->leftWin, A_BOLD);
  wattron(dis->rightWin, A_BOLD);
  wattron(dis->titleWin, A_BOLD);

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
  
  /*favorites window*/
  strncpy(dis->pinned[0], "/home", 6);
  dis->pinc = 1;
  return dis;
}

void get_updates(Display *dis) {
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);
  /*resize window width/height if change in terminal size*/
  if (dis->width != w.ws_col || dis->height != w.ws_row) {
    dis->width = w.ws_col;
    if (state.show_border) dis->height = w.ws_row -1;
    else dis->height = w.ws_row - abs(1-state.show_border);
    dis->mainWinWidth = w.ws_col/2 - (w.ws_col/8);
    dis->leftWinWidth = w.ws_col/8;
    dis->rightWinWidth = w.ws_col/2;
    dis->previewWidth = COLS/2 - (2*state.show_border);
    dis->previewHeight = dis->height-(2*state.show_border);
    
    wresize(dis->leftWin, dis->height, dis->leftWinWidth);
    wresize(dis->mainWin, dis->height, dis->mainWinWidth);
    wresize(dis->rightWin, dis->height, dis->rightWinWidth);
    wresize(dis->titleWin, 1, dis->width);
    wresize(dis->previewWin, dis->previewHeight, dis->previewWidth);

    mvwin(dis->titleWin, 0, 0);
    mvwin(dis->leftWin, 1, 0);
    mvwin(dis->mainWin, 1, dis->leftWinWidth);
    mvwin(dis->rightWin, 1, w.ws_col/2);
    mvwin(dis->previewWin, 1+state.show_border, w.ws_col/2+state.show_border);
  }
}

void draw_window(WINDOW *win, int width, int height, Directory *dir, int mode, const char *line_buffer) {
  switch (mode) {
    case DIR_MODE: {
      /*draws the normal directory window*/
      int shiftview = 0;
      char buffer[MAXLINEBUFFER] = {0};
      werase(win);
      /* total files > window height */
      if ((dir->folderCount+dir->fileCount > height-1-2*state.show_border) 
      && (dir->selected+state.shift_pos > height-1-2*state.show_border-1)) 
        shiftview = dir->selected+state.shift_pos - height+1+2*state.show_border;
      if (width-1 > 0) {
        /*window folders*/
        int i;
        wattron(win, COLOR_PAIR(DIRCOLOR));
        for (i = 0; i < dir->folderCount; i++) {
          strncpy(buffer, dir->folders[i].name,width-1-state.show_border);
          buffer[strlen(buffer)] = '\0';
          mvwaddstr(win, i+state.show_border-shiftview, state.show_border, buffer);
          memset(buffer,0,MAXLINEBUFFER-1);
        } 
        wattroff(win, COLOR_PAIR(DIRCOLOR));
        /*window files*/
        wattron(win, COLOR_PAIR(FILECOLOR));
        for (i = 0; i < dir->fileCount; i++) {
          if (dir->files[i].type == 'e')
            wattron(win, COLOR_PAIR(EXECOLOR));
          else
            wattron(win, COLOR_PAIR(FILECOLOR));
          strncpy(buffer, dir->files[i].name,width-1-state.show_border);
          buffer[strlen(buffer)] = '\0';
          mvwaddstr(win, dir->folderCount+i+state.show_border-shiftview, 
                    state.show_border, buffer);
          memset(buffer,0,MAXLINEBUFFER-1);
        }
        wattroff(win, COLOR_PAIR(FILECOLOR));
        wattroff(win, COLOR_PAIR(EXECOLOR));
        if (dir->files[0].type == 'z')
          mvwchgat(win, dir->selected+state.show_border-shiftview, 
          state.show_border, width-1-state.show_border, A_STANDOUT | A_BOLD, ROOTCOLOR, NULL);
        else
          mvwchgat(win, dir->selected+state.show_border-shiftview, 
          state.show_border, width-1-state.show_border, A_STANDOUT | A_BOLD, CURSORCOLOR, NULL);
      } 
      if (state.show_border) {
        wattron(win, COLOR_PAIR(BORDERCOLOR));
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(BORDERCOLOR));
      }
      wrefresh(win);
      break;
    }
    case PREVIEW_MODE:
      /*display a file preview*/
      werase(win);
      if (dir->selected > dir->folderCount && dir->selected < dir->folderCount + dir->fileCount) {
        wattron(win, COLOR_PAIR(TEXTCOLOR));
        if (dir->files[dir->selected - dir->folderCount].preview == NULL) 
          dir->files[dir->selected - dir->folderCount].preview = get_file_preview(dir->files[dir->selected - dir->folderCount].path);
        if (dir->files[dir->selected - dir->folderCount].preview != NULL) 
          mvwprintw(win, 0, 0, "%s", dir->files[dir->selected - dir->folderCount].preview);
        else
          mvwprintw(win, 0, 0, "%s\nSize: %ld Bytes\nDate: %s\n", 
              dir->files[dir->selected - dir->folderCount].name,
              dir->files[dir->selected - dir->folderCount].bytesize,
              dir->files[dir->selected - dir->folderCount].date);
        wattroff(win, COLOR_PAIR(TEXTCOLOR));
      }
      wrefresh(win);
      break;
    case BOX_MODE:
      /*draw empty box*/
      werase(win);
      wattron(win, COLOR_PAIR(BORDERCOLOR));
      box(win, 0, 0);
      wattroff(win, COLOR_PAIR(BORDERCOLOR));
      wrefresh(win);
      break;
    case CMD_MODE: 
      werase(win);
      wattron(win, COLOR_PAIR(CMDCOLOR));
      mvwprintw(win, 0, 0, ":~$ %s_", line_buffer);
      wattroff(win, COLOR_PAIR(CMDCOLOR));
      wrefresh(win);
      break;
    case INFO_MODE:
      werase(win);
      attron(A_BOLD);
      attron(COLOR_PAIR(TITLECOLOR));
      mvwprintw(win, 0, 0, "%s", line_buffer);
      attroff(COLOR_PAIR(TITLECOLOR));
      attroff(A_BOLD);
      wrefresh(win);
      break;
    default:
      break;
  }
}

void update_display(Display *dis, Directory **dirptr) {
  Directory *dir = *dirptr;
  Directory *top = dir->parent;
  WINDOW *leftWin = dis->leftWin;
  WINDOW *mainWin = dis->mainWin;
  WINDOW *rightWin = dis->rightWin;
  WINDOW *titleWin = dis->titleWin;
  WINDOW *previewWin = dis->previewWin;
  int key;
  /*print currently selected directory path*/
  if (dir->selected < dir->folderCount)
    draw_window(titleWin, dis->width, 1, dir, INFO_MODE, dir->folders[dir->selected].path);
  else if (dir->selected < (dir->folderCount + dir->fileCount))
    draw_window(titleWin, dis->width, 1, dir, INFO_MODE, dir->files[dir->selected - dir->folderCount].path);
  /*refresh();*/
  /*draw windows*/
  draw_window(mainWin, dis->mainWinWidth, dis->height, dir, DIR_MODE, NULL);
  draw_window(leftWin, dis->leftWinWidth, dis->height, top, DIR_MODE, NULL);
  read_selected(dir);
  if (dir->selected < dir->folderCount && dir->folders[dir->selected].subdir != NULL)
    draw_window(rightWin, dis->rightWinWidth, dis->height, dir->folders[dir->selected].subdir, DIR_MODE, NULL);
  else {
    if (state.show_border) draw_window(rightWin, dis->rightWinWidth, dis->height, dir, BOX_MODE, NULL);
    draw_window(previewWin, dis->previewWidth, dis->previewHeight, dir, PREVIEW_MODE, NULL);
  }
  /*wait for user input*/
  key = wgetch(mainWin);
  if (key != ERR)
    handle_input(key, dis, dir, dirptr);
  
  erase();
}

void kill_display(Display *dis) {
  delwin(dis->root);
  delwin(dis->titleWin);
  delwin(dis->mainWin);
  delwin(dis->leftWin);
  delwin(dis->rightWin);
  delwin(dis->previewWin);

  free(dis);
	endwin();
}
