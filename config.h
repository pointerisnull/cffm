#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULTBACKGROUND -1
#define RED     1
#define GREEN   2
#define BLUE    3
#define CYAN    4
#define PURPLE  5
#define YELLOW  6
#define ORANGE  7
#define WHITE   8
#define BLACK   9

#define USECONFIGFILE 1
/*************************************************************************\
* Manual settings, used as defaults if using a config file.               *
*  If you would rather not use a config file, change USECONFIGFILE to 0   *
* and set your desired values below.                                      *
\*************************************************************************/
#define BORDERCOLOR   PURPLE
#define CURSORCOLOR   PURPLE
#define TITLECOLOR    GREEN
#define DIRCOLOR      PURPLE 
#define ROOTCOLOR     RED
#define FILECOLOR     WHITE
#define TEXTCOLOR     WHITE
#define EXECOLOR      RED
#define IMAGECOLOR    CYAN 
#define MEDIACOLOR    BLUE

#define SHOWHIDDENDEFAULT 0
#define SHOWBORDERDEFAULT 0
/*cursor's row threshold before the file list scrolls up*/
#define SHIFTSIZE     16

typedef struct State {
  short isRunning;
  short currentTab; //to be implemented
  short hasPerformedAction;
  /*settings*/
  short showHidden;
  short showBorder;
  short shiftPos;
  /*colors*/
  short bgColor;
  short mainBorderColor;
  short leftBorderColor;
  short rightBorderColor;
  short mainCursorColor;
  short leftCursorColor;
  short rightCursorColor;
  short titleColor;
  short dirColor;
  short rootDirColor;
  /*file colors*/
  short fileColor;
  short textColor;
  short exeColor;
  short txtColor;
  short imageColor;
  short mediaColor;
} State;

extern State state;

#endif  
