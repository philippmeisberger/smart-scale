#ifndef MODE_H
#define MODE_H

#include "buttons.h"

struct mode {
    void (*setup)();
    void (*loop)();
    char *name;

    mode() : setup(NULL), loop(NULL), name(NULL){}
    mode(void (*setup)(), void (*loop)(), char* name) : setup(setup), loop(loop), name(name){}
};

mode modes[3];
uint8_t next_mode = 0;
uint8_t selected_mode = 0;

inline void addMode(void (*setup)(), void (*loop)(), char* name) {
  modes[next_mode++] = mode(setup, loop, name);
}

void switchMode(uint8_t newMode) {
  selected_mode = newMode % next_mode;
  DEBUG_PRINTF("switchMode(): Changing mode to %i\n", selected_mode);
  modes[selected_mode].setup();
}

void selectMode()
{
  uint8_t selected = selected_mode;

  if (getButtonDownState() == HIGH)
  {
    DEBUG_PRINTLN("BUTTON_DOWN");
    switchMode(--selected);
    return;
  }

  if (getButtonUpState() == HIGH)
  {
    DEBUG_PRINTLN("BUTTON_UP");
    switchMode(++selected);
    return;
  }
}

#endif //MODE_H
