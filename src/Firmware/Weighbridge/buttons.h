#include "logging.h"
#include "config.h"

#ifndef BUTTONS_H
#define BUTTONS_H

// Current button states
int buttonDownState;
int buttonUpState;
int buttonLeftState;
int buttonRightState;

// Previous button states
int lastButtonDownState = LOW;
int lastButtonUpState = LOW;
int lastButtonLeftState = LOW;
int lastButtonRightState = LOW;

// Previous debounce times
unsigned long lastButtonDownDebounceTime = 0;
unsigned long lastButtonUpDebounceTime = 0;
unsigned long lastButtonLeftDebounceTime = 0;
unsigned long lastButtonRightDebounceTime = 0;

int getButtonState(int pin, int* buttonState, int* lastButtonState, unsigned long* lastButtonDebounceTime, bool toggle)
{
  int stateChanged = LOW;
  int state = digitalRead(pin);

  if (state != *lastButtonState) 
  {
    *lastButtonDebounceTime = millis();
  }
  
  if ((millis() - *lastButtonDebounceTime) > BUTTON_DEBOUNCE_DELAY) 
  {
    // Button state has changed
    if (state != *buttonState) 
    {
      *buttonState = state;
      DEBUG_PRINTLN("getButtonState(): State changed");

      // Button pressed
      if (toggle && (state == HIGH))
      {
        stateChanged = HIGH;
        DEBUG_PRINTLN("getButtonState(): Button pressed");
      }
    }
  }

  *lastButtonState = state;
  return toggle? stateChanged : *buttonState;
}

inline int getButtonDownState(bool toggle = true)
{
  return getButtonState(BUTTON_DOWN, &buttonDownState, &lastButtonDownState, &lastButtonDownDebounceTime, toggle);
}

inline int getButtonUpState(bool toggle = true)
{
  return getButtonState(BUTTON_UP, &buttonUpState, &lastButtonUpState, &lastButtonUpDebounceTime, toggle);
}

inline int getButtonLeftState(bool toggle = true)
{
  return getButtonState(BUTTON_LEFT, &buttonLeftState, &lastButtonLeftState, &lastButtonLeftDebounceTime, toggle);
}

inline int getButtonRightState(bool toggle = true)
{
  return getButtonState(BUTTON_RIGHT, &buttonRightState, &lastButtonRightState, &lastButtonRightDebounceTime, toggle);
}

#endif //BUTTONS_H
