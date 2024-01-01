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
char *get_file_preview(File *file);
void read_selected(Directory *dir);
int cmd_window(WINDOW *win, int width, int height, Directory *current, int MODE);
void exec_cmd(const char *buffer);
void favorites_window(Display *dis);
char *get_selection_buffer();
int is_selected(ClipBoard *cb, void *ptr, int is_file);

void handle_input(int key, Display *dis, Directory *dir, Directory **dirptr) {
  switch(key) {
    case key_left:
      if (strncmp(dir->path, "/", 2) != 0) {
        if (chdir("..") == 0) {
          Directory *temp = dir;
          if (dir->parent->selected < dir->parent->folderc 
          && dir->parent->folders[dir->parent->selected].subdir == NULL) {
            dir->parent->folders[dir->parent->selected].subdir = temp;
          }
          temp = dir->parent;
          *dirptr = temp;
        }
      }
    break;
    case key_down:
      if (state.visual_mode) handle_input(key_space, dis, dir, NULL);
      if (dir->selected < dir->folderc+dir->filec-1) dir->selected++;
      if (dir->selected < dir->folderc && dir->folders[dir->selected].subdir == NULL) {
        Directory *temp = malloc(sizeof(Directory));
        read_directory(dir->folders[dir->selected].path, temp);
        if (temp->broken == 0) {
          temp->parent = dir;
          dir->folders[dir->selected].subdir = temp;
        }
      }
    break;
    case key_up:
      if (state.visual_mode) handle_input(key_space, dis, dir, NULL);
      if (dir->selected > 0) dir->selected--;
      if (dir->selected < dir->folderc && (dir->folders[dir->selected].subdir == NULL)) {
        Directory *temp = malloc(sizeof(Directory));
        read_directory(dir->folders[dir->selected].path, temp);
        if (temp->broken == 0) {
          temp->parent = dir;
          dir->folders[dir->selected].subdir = temp;
        }
      }
    break;
    case key_right:
      if (dir->selected < dir->folderc) {
        
        if (dir->folders[dir->selected].subdir != NULL) {
          Directory *temp = dir;
          if (chdir(dir->folders[dir->selected].path) == 0) {
            temp = dir->folders[dir->selected].subdir;
            *dirptr = temp;
          }
          read_selected(temp);
        }
        /*is file, execute it*/
      } else if (dir->selected < dir->folderc + dir->filec) {
          char buffer[4096];
          strncpy(buffer, "mimeopen ", 10);
          strcat(buffer, dir->files[dir->selected-dir->folderc].name);
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
      cmd_window(dis->titlew, dis->width, 1, dir, CMD_MODE);
    break;
    case key_rename:
      cmd_window(dis->titlew, dis->width, 1, dir, RN_MODE);
    break;
    case key_esc:
      state.visual_mode = 0;
      memset(state.cb.folderptr, 0, MAXSELECTED-1);
      memset(state.cb.fileptr, 0, MAXSELECTED-1);
      state.cb.folderc = 0;
      state.cb.filec = 0;
      state.cutting = 0;
      state.copying = 0;
      state.deleting = 0;
    break;
    case key_space: {
      if (dir->selected < dir->folderc) { 
        if (is_selected(&state.cb, &dir->folders[dir->selected], 0) == 1) {
          state.cb.folderptr[state.cb.folderc] = &dir->folders[dir->selected];
          state.cb.folderc++;
        }
      } else if (dir->selected < dir->folderc + dir->filec) {  
        if (is_selected(&state.cb, &dir->files[dir->selected - dir->folderc], 1) == 1) {
          state.cb.fileptr[state.cb.filec] = &dir->files[dir->selected - dir->folderc];
          state.cb.filec++;
        }
      }
    } 
    break;
    case key_visual_mode:
      state.visual_mode = (state.visual_mode) ? 0 : 1;
      if (!state.visual_mode) handle_input(key_space, dis, dir, NULL);
    break;
    case key_cut:
      state.cutting = 1;
      state.copying = 0;
    break;
    case key_copy:
      state.cutting = 0;
      state.copying = 1;
    break;
    case key_paste: {
      char *buffer = get_selection_buffer();
      if (buffer != NULL) {
        strncat(buffer, dir->path, strlen(dir->path));
        system(buffer);
        free(buffer);
        update_directory(dir);
        read_selected(dir);
      }
    }
    handle_input(key_esc, dis, dir, NULL);
    break;
    case key_delete:
      state.deleting = 1;
      state.cutting = 0;
      state.copying = 0;
      if (ALLOW_DELETE && cmd_window(dis->titlew, dis->width, 1, dir, CONFIRM_MODE) == 1) {
        char *buffer = get_selection_buffer();
        if (buffer != NULL) {
          system(buffer);
          free(buffer);
          update_directory(dir);
          read_selected(dir);
        }
        handle_input(key_esc, dis, dir, NULL);
      } else state.deleting = 0;
    break;
    default:
    break;
  }
}

char *get_selection_buffer() {
  int i;
  size_t fs = sizeof(char) * (MAXPATHNAME * (state.cb.folderc+state.cb.filec + 2));
  char *buffer = NULL;
  if (state.cb.folderc + state.cb.filec == 0) return NULL;
  buffer = malloc(fs);
  memset(buffer, '\0', fs);
  if (state.copying) strncat(buffer, "cp -r ", 7);
  else if (state.cutting) strncat(buffer, "mv ", 4);
  else if (ALLOW_DELETE && state.deleting) strncat(buffer, "rm -r ", 7);
  else strncat(buffer, "echo ", 6); /*undefined, so harmless command? :)*/
  /*start with folders*/
  if (state.cb.folderc > 0) {
    for (i = 0; i < state.cb.folderc; i++) {
      if (state.cb.folderptr[i] != NULL) {
        strncat(buffer, state.cb.folderptr[i]->path, strlen(state.cb.folderptr[i]->path));
        strncat(buffer, " ", 2);
      } 
    }
  }
  if (state.cb.filec > 0) {
    for (i = 0; i < state.cb.filec; i++) {
      if (state.cb.fileptr[i] != NULL) {
        strncat(buffer, state.cb.fileptr[i]->path, strlen(state.cb.fileptr[i]->path));
        strncat(buffer, " ", 2);
      }
    }
  }
  return buffer;
}

void exec_cmd(const char *buffer) {
  def_prog_mode();
  endwin();
  system(buffer);
  reset_prog_mode();
}

int cmd_window(WINDOW *win, int width, int height, Directory *current, int MODE) {
  int key = 0;
  int i = 0;
  char buff[2048] = {'\0'};
  while (key != '\n' && key != 27) {
    draw_window(win, width, height, NULL, MODE, buff);
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
  /*if not exiting early*/
  if (key == '\n') {
    switch (MODE) {
      case CMD_MODE:
        exec_cmd(buff);
        break;
      case RN_MODE:
        buff[i-1] = '\0'; /*removes \n character from name*/
        if (current->selected < current->folderc)
          rename(current->folders[current->selected].name, buff);
        else if (current->selected < current->folderc + current->filec)
          rename(current->files[current->selected-current->folderc].name, buff);
        break;
      case CONFIRM_MODE:
        if (strncmp(buff, "y", 1) == 0) return 1; 
        break;
      default: 
        break;
    }
    update_directory(current);
  }
  return 0;
}

void read_selected(Directory *temp) {
  /*if new dir, read next*/
  if (temp->selected < temp->folderc 
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
  } else if (temp->folderc == 0 && temp->filec > 0 
  && temp->files[temp->selected].preview == NULL) {
    temp->files[temp->selected].preview = get_file_preview(&temp->files[temp->selected]);
  }
}

char *get_file_preview(File *file) {
  FILE *fp = NULL;
  uint64_t length;
  int i = 0;
  char c;
  char *buffer;

  if (file->path != NULL) fp = fopen(file->path, "r");
  if (fp == NULL) return NULL;

  length = file->bytesize;
  if (length > MAXPREVIEWSIZE) length = MAXPREVIEWSIZE;

  buffer = malloc(sizeof(char) * length+1);
  while ((c = fgetc(fp)) != EOF && (uint64_t) i < length) {
    buffer[i] = c;
    i++;
  }
  /*is the file a text file? If not, return NULL*/
  if (memchr(buffer, '\0', length-1) != NULL) {
    free(buffer);
    fclose(fp);
    return NULL;
  }
  fclose(fp);
  buffer[i] = '\0';

  return buffer;
}

Display *init_display(Directory *dir) {	
  Display *dis = malloc(sizeof(Display));

  initscr();
  cbreak();
  curs_set(0);
  noecho();
  /*nodelay(dis->mainw, TRUE);*/
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

  dis->mainw_width = COLS/2 - (COLS/8);
  dis->leftw_width = COLS/8;
  dis->rightw_width = COLS/2;
  dis->preview_width = COLS/2 - (2*state.show_border);
  
  dis->root = newwin(LINES, COLS, 0, 0);
  dis->leftw = newwin(LINES-1, dis->leftw_width, 1, 0);
  dis->mainw = newwin(LINES-1, dis->mainw_width, 1, dis->leftw_width);
  dis->rightw = newwin(LINES-1, dis->rightw_width, 1, COLS/2);
  dis->titlew = newwin(LINES-1, dis->width, 0, 0);
  dis->previeww = newwin(LINES-1, dis->preview_width, 1, COLS/2+state.show_border);
  
  wattron(dis->leftw, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->mainw, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->rightw, COLOR_PAIR(BORDERCOLOR));
  wattron(dis->titlew, COLOR_PAIR(TITLECOLOR));
  wattron(dis->mainw, A_BOLD);
  wattron(dis->leftw, A_BOLD);
  wattron(dis->rightw, A_BOLD);
  wattron(dis->titlew, A_BOLD);

  if (dir->selected < dir->folderc && dir->folders[dir->selected].subdir == NULL) {
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
    if (state.show_border) dis->height = w.ws_row -1;
    else dis->height = w.ws_row - abs(1-state.show_border);
    dis->mainw_width = w.ws_col/2 - (w.ws_col/8);
    dis->leftw_width = w.ws_col/8;
    dis->rightw_width = w.ws_col/2;
    dis->preview_width = COLS/2 - (2*state.show_border);
    dis->preview_height = dis->height-(2*state.show_border);
    
    wresize(dis->leftw, dis->height, dis->leftw_width);
    wresize(dis->mainw, dis->height, dis->mainw_width);
    wresize(dis->rightw, dis->height, dis->rightw_width);
    wresize(dis->titlew, 1, dis->width);
    wresize(dis->previeww, dis->preview_height, dis->preview_width);

    mvwin(dis->titlew, 0, 0);
    mvwin(dis->leftw, 1, 0);
    mvwin(dis->mainw, 1, dis->leftw_width);
    mvwin(dis->rightw, 1, w.ws_col/2);
    mvwin(dis->previeww, 1+state.show_border, w.ws_col/2+state.show_border);
  }
}

int is_selected(ClipBoard *cb, void *ptr, int isfile) {
  int i;
  if (!isfile) {
    for (i = 0; i < cb->folderc; i++)
      if (ptr == cb->folderptr[i])
        return 0; /*yes, the folder is selected*/
  } else {
    for (i = 0; i < cb->filec; i++)
      if (ptr == cb->fileptr[i])
        return 0; /*yes, the folder is selected*/
  }
  return 1;
}

void draw_window(WINDOW *win, int width, int height, Directory *dir, int mode, const char *line_buffer) {
  switch (mode) {
    case DIR_MODE: {
      /*draws the normal directory window*/
      int shiftview = 0;
      char buffer[MAXLINEBUFFER] = {0};
      werase(win);
      /* total files > window height */
      if ((dir->folderc+dir->filec > height-1-2*state.show_border) 
      && (dir->selected+state.shift_pos > height-1-2*state.show_border-1)) 
        shiftview = dir->selected+state.shift_pos - height+1+2*state.show_border;
      if (width-1 > 0) {
        /*window folders*/
        int i;
        wattron(win, COLOR_PAIR(DIRCOLOR));
        for (i = 0; i < dir->folderc; i++) {
          strncpy(buffer, dir->folders[i].name,width-1-state.show_border);
          buffer[strlen(buffer)] = '\0';
          mvwaddstr(win, i+state.show_border-shiftview, state.show_border, buffer);
          memset(buffer,0,MAXLINEBUFFER-1);
          /*highlight selected folders*/
          if (state.cb.folderc + state.cb.filec > 0 && 
          is_selected(&state.cb, &dir->folders[i], 0) == 0)
            mvwchgat(win, i+state.show_border-shiftview, 
            state.show_border, width-1-state.show_border, A_STANDOUT | A_BOLD, VMCOLOR, NULL);
        } 
        wattroff(win, COLOR_PAIR(DIRCOLOR));
        /*window files*/
        wattron(win, COLOR_PAIR(FILECOLOR));
        for (i = 0; i < dir->filec; i++) {
          if (dir->files[i].type == 'e')
            wattron(win, COLOR_PAIR(EXECOLOR));
          else
            wattron(win, COLOR_PAIR(FILECOLOR));
          strncpy(buffer, dir->files[i].name,width-1-state.show_border);
          buffer[strlen(buffer)] = '\0';
          mvwaddstr(win, dir->folderc+i+state.show_border-shiftview, 
                    state.show_border, buffer);
          memset(buffer,0,MAXLINEBUFFER-1);
          /*highlight selected files*/
          if (state.cb.folderc + state.cb.filec > 0 && 
          is_selected(&state.cb, &dir->files[i], 1) == 0)
            mvwchgat(win, dir->folderc+i+state.show_border-shiftview, 
            state.show_border, width-1-state.show_border, A_STANDOUT | A_BOLD, VMCOLOR, NULL);
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
      if (dir->selected > dir->folderc-1 && dir->selected < dir->folderc + dir->filec) {
        wattron(win, COLOR_PAIR(TEXTCOLOR));
        if (dir->files[dir->selected - dir->folderc].preview == NULL) 
          dir->files[dir->selected - dir->folderc].preview = get_file_preview(&dir->files[dir->selected - dir->folderc]);
        if (dir->files[dir->selected - dir->folderc].preview != NULL) 
          mvwprintw(win, 0, 0, "%s", dir->files[dir->selected - dir->folderc].preview);
        else
          mvwprintw(win, 0, 0, "%s\nSize: %ld Bytes\nDate: %s\n", 
              dir->files[dir->selected - dir->folderc].name,
              dir->files[dir->selected - dir->folderc].bytesize,
              dir->files[dir->selected - dir->folderc].date);
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
    case RN_MODE: 
      werase(win);
      wattron(win, COLOR_PAIR(CMDCOLOR));
      mvwprintw(win, 0, 0, "Rename: %s_", line_buffer);
      wattroff(win, COLOR_PAIR(CMDCOLOR));
      wrefresh(win);
      break;
    case CONFIRM_MODE: 
      werase(win);
      wattron(win, COLOR_PAIR(CMDCOLOR));
      mvwprintw(win, 0, 0, "Are you sure? [y/N] %s_", line_buffer);
      wattroff(win, COLOR_PAIR(CMDCOLOR));
      wrefresh(win);
      break;
    default:
      break;
  }
}

void update_display(Display *dis, Directory **dirptr) {
  Directory *dir = *dirptr;
  Directory *top = dir->parent;
  WINDOW *leftw = dis->leftw;
  WINDOW *mainw = dis->mainw;
  WINDOW *rightw = dis->rightw;
  WINDOW *titlew = dis->titlew;
  WINDOW *previeww = dis->previeww;
  int key;
  /*print currently selected directory path*/
  if (dir->selected < dir->folderc)
    draw_window(titlew, dis->width, 1, dir, INFO_MODE, dir->folders[dir->selected].path);
  else if (dir->selected < (dir->folderc + dir->filec))
    draw_window(titlew, dis->width, 1, dir, INFO_MODE, dir->files[dir->selected - dir->folderc].path);
  /*refresh();*/
  /*draw windows*/
  draw_window(mainw, dis->mainw_width, dis->height, dir, DIR_MODE, NULL);
  draw_window(leftw, dis->leftw_width, dis->height, top, DIR_MODE, NULL);
  read_selected(dir);
  if (dir->selected < dir->folderc && dir->folders[dir->selected].subdir != NULL)
    draw_window(rightw, dis->rightw_width, dis->height, dir->folders[dir->selected].subdir, DIR_MODE, NULL);
  else {
    if (state.show_border) draw_window(rightw, dis->rightw_width, dis->height, dir, BOX_MODE, NULL);
    draw_window(previeww, dis->preview_width, dis->preview_height, dir, PREVIEW_MODE, NULL);
  }
  /*wait for user input*/
  key = wgetch(mainw);
  if (key != ERR)
    handle_input(key, dis, dir, dirptr);
  
  erase();
}

void kill_display(Display *dis) {
  delwin(dis->root);
  delwin(dis->titlew);
  delwin(dis->mainw);
  delwin(dis->leftw);
  delwin(dis->rightw);
  delwin(dis->previeww);

  free(dis);
	endwin();
}
