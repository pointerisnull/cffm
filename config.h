#ifndef CONFIG_H
#define CONFIG_H

typedef struct State {
  int showHidden;
  int hasChangedDir;
} State;

extern State state;

#endif  
