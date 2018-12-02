#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711_ADC.h>

#include "../config.h"
#include "../logging.h"
#include "../mode.h"

extern Adafruit_SSD1306 display;


namespace WEIGHBRIDGE
{
  // Connect black to E+, red to E-, green to A+, white to A-
  // Connect SCK to D8, DT to D3
  HX711_ADC hx711(D3, D8);

  // Currently displayed weight
  int currentWeight = 0;

  // Timestamp of last weighing process
  int lastWeighingTime = 0;

  // Display is currently in standby mode
  bool displayStandby  = false;

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

      WEIGHBRIDGE::loop();
    }
  }

  namespace VOLUME
  {
    WiFiClient wifiClient = WiFiClient();
    PubSubClient mqttClient = PubSubClient(wifiClient);
    const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

    // Last published weight
    int lastWeightSent = 0;

    // Overall consumption
    int consumption = 0;

    // Number of attempts to connect to MQTT broker
    int mqttConnectionAttempts = MQTT_CONNECTION_ATTEMPTS;

    void setupWifi()
    {
      // Stay offline
      if ((WIFI_SSID == "") || (WIFI_PASSWORD == ""))
        return;

      // Already connected
      if (WiFi.status() == WL_CONNECTED)
        return;

      DEBUG_PRINTF("setupWifi(): Connecting to Wi-Fi access point '%s'\n", WIFI_SSID);

      // Do not store Wi-Fi config in SDK flash area
      WiFi.persistent(false);

      // Disable auto Wi-Fi access point mode
      WiFi.mode(WIFI_STA);

      // Start Wi-Fi connection
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      // Wi-Fi not yet connected?
      // TODO: Maybe give up after several attempts
      while (WiFi.status() != WL_CONNECTED)
      {
        // TODO: Maybe show splash screen
        updateDisplay("", "Connecting...");
        DEBUG_PRINT(".");
        delay(500);
      }

      // Wi-Fi connection established
      DEBUG_PRINT("setupWifi(): Connected to Wi-Fi access point. Obtained IP address: ");
      DEBUG_PRINTLN(WiFi.localIP());
    }

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
      DEBUG_PRINTF("publishState(): Publishing state (consumed %i)\n", consumed);
      consumption += -consumed;

      // Offline
      if ((WIFI_SSID == "") || (WIFI_PASSWORD == ""))
      {
        DEBUG_PRINTLN("publishState(): Not publishing in offline mode");
        lastWeightSent = currentWeight;
        updateStatus("Offline");
        return;
      }

      updateStatus("");

      // Connect MQTT broker
      while (!mqttClient.connected())
      {
        DEBUG_PRINTF("publishState(): Connecting to MQTT broker '%s: %i'...\n", MQTT_SERVER, MQTT_PORT);

        if (mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD))
        {
          DEBUG_PRINTLN("publishState(): Connected to MQTT broker");
        }
        else
        {
          DEBUG_PRINTF("publishState(): Connection failed with error code %i\n", mqttClient.state());
          updateStatus("Publishing failed");

          // Give up?
          if (mqttConnectionAttempts == 0)
          {
            DEBUG_PRINTLN("publishState(): Maximum connection attempts exceeded... Giving up");
            mqttConnectionAttempts = MQTT_CONNECTION_ATTEMPTS;
            return;
          }

          mqttConnectionAttempts--;
          delay(MQTT_CONNECTION_ATTEMPT_DELAY);
        }
      }

      // Send JSON
      StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();

      json["consumed"] = consumed;
      json["consumption"] = consumption;

      char message[json.measureLength() + 1];
      json.printTo(message, sizeof(message));

      DEBUG_PRINTF("publishState(): Publishing message on channel '%s': %s\n", MQTT_CHANNEL_STATE, message);
      mqttClient.publish(MQTT_CHANNEL_STATE, message);

      lastWeightSent = currentWeight;
    }

    void setup()
    {
      DEBUG_PRINTLN("setup(): VOLUME");
      setupWifi();
      mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
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
      else
      {
        // Stabilized?
        if ((millis() - lastWeighingTime > HX711_STABILIZING_INTERVAL) && (currentWeight == weight) && (lastWeightSent != currentWeight))
        {
          // Publish state
          lastWeighingTime = millis();
          publishState((lastWeightSent == 0)? lastWeightSent : currentWeight - lastWeightSent);
          displayStandby = false;
        }
      }

      mqttClient.loop();
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
