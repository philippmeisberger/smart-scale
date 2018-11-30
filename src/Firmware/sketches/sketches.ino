#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config.h"
#include "logging.h"
#include "mode.h"
#include "SnakeMode/snake_mode.h"
#include "Weighbridge/weighbridge.h"

#define FIRMWARE_VERSION "0.5"

// Connect SCL to D1 and SDA to D2
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

extern mode modes[];
extern int next_mode;
extern int selected_mode;


namespace MAIN_MODE{

  int _selected = 0;
  bool once = false;

  void update() {
    if (Serial.available() > 0) {
      char in = Serial.read();
      if (in == 'w') _selected--;
      if (in == 's') _selected++;
      if (in == 'a');
      if (in == 'd') {
        selected_mode = _selected;
        modes[selected_mode].setup();
      }
      _selected = min(max(0, _selected), next_mode-1);
      Serial.flush();
    }else if(once){
      return;
    }
    once = true;

    display.clearDisplay();
    display.setTextSize(2);
    for (int i = 0; i < 10; ++i) { //todo: set 0 to 1
      if (_selected == i) {
        display.setCursor(0, 16 * i);
        display.print(">");
      }

      display.setCursor(12, 16 * i);
      display.print(modes[i].name);
    }
    display.display();

    delay(100);
  }

  void start_mode() {
    once = false;
  }

  void setup(){
    modes[next_mode++] = mode(&start_mode, &update, (char *) "self");
  }

}


/// ////////////////////////////////////////////////
/// ARDUINO INIT:
/// ////////////////////////////////////////////////

void setupDisplay() {
  DEBUG_PRINTLN("setupDisplay(): Initializing...");

  // initialize with the I2C addr 0x3C (for the 128x32)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);

  //display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
}

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  delay(250);
  Serial.printf("ESP8266 Smart Weighbridge '%s'\n", FIRMWARE_VERSION);
#endif
  setupDisplay();
  // randomSeed(analogRead(0));

  // INCLUDE PACKAGES:
  selected_mode = 0;
  MAIN_MODE::setup();
  WEIGHBRIDGE::setup();
  SNAKE_MODE::setup();
}

void loop() {
  modes[selected_mode].update();
}
