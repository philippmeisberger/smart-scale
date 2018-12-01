#ifndef MODE_H
#define MODE_H

struct mode {
    char *name;
    void (*setup)();
    void (*update)();

    mode() : setup(NULL), update(NULL), name(NULL){}
    mode(void (*setup)(), void (*update)(), char* name) : setup(setup), update(update), name(name){}
};

mode modes[10];
int next_mode = 0;
int selected_mode = 0;

inline void addMode(void (*start_mode)(), void (*update)(), char* name) {
  modes[next_mode++] = mode(start_mode, update, name);
}

void switchMode(int newMode) {
  selected_mode = min(max(0, newMode), next_mode - 1);
  DEBUG_PRINTF("switchMode(): Changing mode to %i\n", selected_mode);
  modes[selected_mode].setup();
}

void selectMode() {
  // TODO: Replace by button push
  if (Serial.available() > 0) {
    DEBUG_PRINTF("switchMode(): %i modes available\n", next_mode);
    int selected = selected_mode;
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
