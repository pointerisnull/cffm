#ifndef DRAW_H
#define DRAW_H

#include "data.h"

typedef struct Display {
  WINDOW *root;
  WINDOW *leftWin;
  WINDOW *mainWin;
  WINDOW *rightWin;
 
  int mainWinWidth;
  int leftWinWidth;
  int rightWinWidth;
  int width, height;

} Display;

void get_updates(Display *dis);
Display *init_display(Directory *dir);
void update_display(Display *dis, Directory **dir);
void kill_display(Display *dis);

#endif
