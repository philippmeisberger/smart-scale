#ifndef MODE_H
#define MODE_H

struct mode {
    char *name;
    void (*setup)();
    void (*loop)();

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

void selectMode() {
  // TODO: Replace by button push
  if (Serial.available() > 0) {
    DEBUG_PRINTF("selectMode(): %i modes available\n", next_mode);
    uint8_t selected = selected_mode;
    char in = Serial.read();

    if (in == 'w') {
      selected--;
    }
    else if (in == 's') {
      selected++;
    }

    Serial.flush();
    switchMode(selected);
    delay(100);
  }
}

#endif //MODE_H
