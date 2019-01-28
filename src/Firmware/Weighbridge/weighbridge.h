#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <HX711_ADC.h>

#include "config.h"
#include "logging.h"
#include "mode.h"
#include "mqtt.h"
#include "buttons.h"

extern Adafruit_SSD1306 display;

namespace WEIGHBRIDGE
{
  // Connect black to E+, red to E-, green to A+, white to A-
  HX711_ADC hx711(HX711_DT, HX711_SCK);

  // Currently displayed weight
  int currentWeight = 0;

  // Timestamp of last weighing process
  unsigned long lastWeighingTime = 0;

  // Display is currently in standby mode
  bool displayStandby = false;

  void updateDisplay(const char* text, const char* statusText)
  {
    display.clearDisplay();

    // Show text in medium font size
    display.setTextSize(3);
    display.setCursor(0, 0);
    display.println(text);

    // Show status in small font size
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.println(statusText);
    display.display();
  }

  inline void updateDisplay(const char* text, const String status) { updateDisplay(text, status.c_str()); }
  inline void updateDisplay(const String text, const char* status) { updateDisplay(text.c_str(), status); }
  inline void updateDisplay(const String text, const String status) { updateDisplay(text.c_str(), status.c_str()); }

  float getWeight()
  {
    hx711.update();
    return hx711.getData();
  }

  void setupScale()
  {
    DEBUG_PRINTLN("setupScale(): Initializing...");
    hx711.begin();
    hx711.start(HX711_STARTUP_DELAY);
    hx711.setCalFactor(HX711_CALIBRATION_FACTOR);
  }

  void loop()
  {
  #if OLED_TIMEOUT > 0
    // Turn off display after some time
    if (!displayStandby && (millis() - lastWeighingTime > OLED_TIMEOUT))
    {
      DEBUG_PRINTLN("Turning off display");
      display.clearDisplay();
      display.display();
      displayStandby = true;
    }
  #endif

    selectMode();
  }

  namespace WEIGHT
  {
    String humanize(const int value)
    {
      if (value >= 1000)
      {
        return String(value / 1000.0) + "kg";
      }

      return String(value) + "g";
    }

    inline void updateStatus(const char* statusText)
    {
      updateDisplay(humanize(currentWeight), statusText);
    }

    void setup()
    {
      DEBUG_PRINTLN("setup(): WEIGHT");
      updateDisplay(humanize(currentWeight), "");
      lastWeighingTime = millis();
    }

    void loop()
    {
      int weight = round(getWeight());

      // Weight has changed
      if (currentWeight != weight)
      {
        DEBUG_PRINTLN(weight);
        currentWeight = weight;
        lastWeighingTime = millis();
        updateStatus("");
        displayStandby = false;
      }

      if (getButtonLeftState() == HIGH)
      {
        hx711.tare();
        DEBUG_PRINTLN("loop(): Tare");
      }

      WEIGHBRIDGE::loop();
    }
  }

  namespace VOLUME
  {
    const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

    // Last published weight
    int lastWeightSent = 0;

    // Overall consumption
    int consumption = 0;

    String humanize(const int value)
    {
      if (value >= 1000)
      {
        return String(value / 1000.0) + "l";
      }

      return String(value) + "ml";
    }

    inline void updateStatus(const char* statusText)
    {
      updateDisplay(humanize(consumption), statusText);
    }

    void publishState(const int consumed)
    {
      // Reduce weight variations
      if ((lastWeightSent != 0) && (abs(consumed) <= HX711_THRESHOLD))
      {
        DEBUG_PRINTLN("publishState(): Not publishing as it was not drunk enough");
        return;
      }

      DEBUG_PRINTF("publishState(): Publishing state (consumed %i)\n", consumed);
      consumption += -consumed;

      // Offline
      if (WiFi.status() != WL_CONNECTED)
      {
        DEBUG_PRINTLN("publishState(): No WiFi connection");
        lastWeightSent = currentWeight;
        updateStatus("No WiFi connection");
        return;
      }

      updateStatus("");

      // Initialize JSON
      StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();

      json["consumed"] = consumed;
      json["consumption"] = consumption;

      char message[json.measureLength() + 1];
      json.printTo(message, sizeof(message));

      // Publish JSON
      if (!publish(MQTT_CHANNEL_STATE, message))
      {
        updateStatus("Publishing failed");
      }

      lastWeightSent = currentWeight;
    }

    void setup()
    {
      DEBUG_PRINTLN("setup(): VOLUME");
      updateDisplay(humanize(consumption), "");
      lastWeighingTime = millis();
    }

    void loop()
    {
      int weight = round(getWeight());

      // Weight has changed
      if (currentWeight != weight)
      {
        DEBUG_PRINTLN(weight);
        currentWeight = weight;
        lastWeighingTime = millis();
        updateStatus("");
        displayStandby = false;
      }
      else if ((abs(currentWeight) > HX711_THRESHOLD))
      {
        // Stabilized?
        if ((millis() - lastWeighingTime > HX711_STABILIZING_INTERVAL) && (currentWeight == weight) && (currentWeight != lastWeightSent))
        {
          // Publish state
          lastWeighingTime = millis();
          publishState((lastWeightSent == 0)? lastWeightSent : currentWeight - lastWeightSent);
          displayStandby = false;
        }
      }

      WEIGHBRIDGE::loop();
    }
  }

  void setup()
  {
    DEBUG_PRINTLN("setup(): Initializing weighbridge...");
    setupScale();
    addMode(&WEIGHT::setup, &WEIGHT::loop, (char*) "weight");
    addMode(&VOLUME::setup, &VOLUME::loop, (char*) "volume");
    WEIGHT::setup();
  }
}
