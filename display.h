#ifndef DRAW_H
#define DRAW_H

#define MAXLINEBUFFER 256
#define MAXPREVIEWSIZE 4096

#include <ncurses.h>
#include "data.h"

typedef struct Display {
  WINDOW *root;
  WINDOW *titleWin;
  WINDOW *leftWin;
  WINDOW *mainWin;
  WINDOW *rightWin;
  WINDOW *previewWin;
  WINDOW *cmdWin;
 
  int mainWinWidth;
  int leftWinWidth;
  int rightWinWidth;
  int previewWidth;
  int previewHeight;
  int cmdWidth;
  int cmdHeight;
  int width, height;

} Display;

void get_updates(Display *dis);
Display *init_display(Directory *dir);
void update_display(Display *dis, Directory **dir);
void kill_display(Display *dis);

#endif
