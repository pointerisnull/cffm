#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "v.0.1.2 (indev)"
#define USECONFIGFILE 1

typedef struct State {
  short isRunning;
  short hasPerformedAction;
  short showHidden;
  short showBorder;
  short shiftPos;
} State;

extern State state;

#endif  
