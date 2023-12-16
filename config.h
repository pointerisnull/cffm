#ifndef CONFIG_H
#define CONFIG_H

typedef struct State {
  int isRunning;
  int hasChangedDir;
  int showHidden;
  int showBorder;
} State;

extern State state;

#endif  
