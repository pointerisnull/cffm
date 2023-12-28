#ifndef CONFIG_H
#define CONFIG_H
/*you shouldn't modify these*/
#define TERMCOLOR -1
#define RED     1
#define GREEN   2
#define BLUE    3
#define CYAN    4
#define PURPLE  5
#define YELLOW  6
#define WHITE   7
#define BLACK   8
/*window modes*/
#define DIR_MODE      0
#define PREVIEW_MODE  1
#define BOX_MODE      2
#define CMD_MODE      3

/*********************************\
* set your desired values below.  *
\*********************************/

#define BACKGROUND    TERMCOLOR
#define BORDERCOLOR   PURPLE
#define CURSORCOLOR   PURPLE
#define TITLECOLOR    WHITE
#define DIRCOLOR      PURPLE 
#define ROOTCOLOR     RED
#define FILECOLOR     WHITE
#define TEXTCOLOR     WHITE
#define EXECOLOR      RED
#define IMAGECOLOR    CYAN 
#define MEDIACOLOR    BLUE

#define SHOWHIDDENDEFAULT 0
#define SHOWBORDERDEFAULT 1
/*cursor's row threshold before the file list scrolls up*/
#define SHIFTSIZE 16
/*key bindings*/
#define key_up          'k'
#define key_down        'j'
#define key_left        'h' /*up dir*/
#define key_right       'l' /*down dir*/
#define key_update      'u' /*update directory*/
#define key_quit        'q'
#define key_cmd         ':'
#define key_select      's'
#define key_rename      't'
#define key_cut         'x'
#define key_copy        'y'
#define key_paste       'p'
#define key_delete      'd'
#define key_visual_mode 'v'
#define key_show_hidden 'r'
#define key_show_border 'b'

#define DATE_FORMAT "%a %Y-%m-%d %H:%M:%S %Z"

#include "hash.h"

typedef struct State {
  short isRunning;
  short currentTab; /* to be implemented */
  short hasPerformedAction;
  /*settings*/
  short showHidden;
  short showBorder;
  short shiftPos;
  /*directory hashtable*/
  Table ht;
} State;

extern State state;

#endif  
