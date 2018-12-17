#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config.h"
#include "logging.h"
#include "mode.h"
#include "snake.h"
#include "weighbridge.h"
#include "mqtt.h"

#define FIRMWARE_VERSION "0.6"

// Connect SCL to D1 and SDA to D2
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// Timestamp of last Wi-Fi connection attempt
int lastWifiConnectionAttempt = 0;

extern PubSubClient mqttClient;

// TODO: No menu should be provided
/*namespace MAIN_MODE{

  int _selected = 0;
  bool once = false;

  void loop() {
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

  void setup() {
    addMode(&start_mode, &loop, (char *) "self");
  }

}*/


/// ////////////////////////////////////////////////
/// ARDUINO INIT:
/// ////////////////////////////////////////////////

void setupDisplay() {
  DEBUG_PRINTLN("setupDisplay(): Initializing...");

  // initialize with the I2C addr 0x3C (for the 128x32)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);

  // TODO: Show splash screen
  //display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
}

void setupWifi()
{
  // Already connected
  if (WiFi.status() == WL_CONNECTED)
  {
    DEBUG_PRINT("setupWifi(): Connected to Wi-Fi access point. IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    return;
  }

  DEBUG_PRINTF("setupWifi(): Connecting to Wi-Fi access point '%s'\n", WIFI_SSID);

  // Do not store Wi-Fi config in SDK flash area
  WiFi.persistent(false);

  // Disable auto Wi-Fi access point mode
  WiFi.mode(WIFI_STA);

  // Start Wi-Fi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  delay(250);
  Serial.printf("ESP8266 Smart Weighbridge '%s'\n", FIRMWARE_VERSION);
#endif
  setupDisplay();
  setupWifi();
  // randomSeed(analogRead(0));

  // Include all modes
  //MAIN_MODE::setup();
  WEIGHBRIDGE::setup();
  SNAKE_MODE::setup();
}

void loop() {
  modes[selected_mode].loop();

  // Try to connect Wi-Fi
  if ((WiFi.status() != WL_CONNECTED) && (millis() - lastWifiConnectionAttempt > WIFI_CONNECTING_INTERVAL))
  {
    lastWifiConnectionAttempt = millis();
    setupWifi();
  }

  mqttClient.loop();
}
