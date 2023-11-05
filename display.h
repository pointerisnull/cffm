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

void checkUpdates(Display *dis);
void initDisplay(Display *dis, Directory *dir);//WINDOW *leftWin, WINDOW *mainWin, WINDOW *rightWin);
void display(Display *dis, Directory **dir); //WINDOW *leftWin, WINDOW *mainWin, WINDOW *rightWin);
void killDisplay();

#endif
