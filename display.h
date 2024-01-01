#ifndef DRAW_H
#define DRAW_H

#define MAXLINEBUFFER 256
#define MAXPREVIEWSIZE 4096

#include <ncurses.h>
#include "data.h"

typedef struct Display {
  WINDOW *root;
  WINDOW *titlew;
  WINDOW *leftw;
  WINDOW *mainw;
  WINDOW *rightw;
  WINDOW *previeww;
 
  int mainw_width;
  int leftw_width;
  int rightw_width;
  int preview_width;
  int preview_height;
  int width, height;
 
} Display;

void get_updates(Display *dis);
Display *init_display(Directory *dir);
void update_display(Display *dis, Directory **dir);
void kill_display(Display *dis);

#endif
