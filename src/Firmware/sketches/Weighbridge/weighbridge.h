#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711_ADC.h>

#include "../config.h"
#include "../logging.h"
#include "../mode.h"

WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient = PubSubClient(wifiClient);

// Connect black to E+, red to E-, green to A+, white to A-
// Connect SCK to D8, DT to D3
HX711_ADC hx711(D3, D8);

extern Adafruit_SSD1306 display;
extern mode modes[];
extern int next_mode;
extern int selected_mode;


namespace WEIGHBRIDGE
{
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

  float getWeight();
  void updateStatus(const char* statusText);
  void updateDisplay(const char* text, const char* statusText);
  void publishState(const int value);

  enum WeighingMode
  {
    // Show mass in g/kg
    weight,

    // Show water consumption in ml/l
    volume
  };

  int currentWeight             = 0;                        // Current weight
  int lastWeighingTime          = 0;                        // Timestamp of last weighing process
  int lastWeightSent            = 0;                        // Last published weight
  int consumption               = 0;                        // Overall consumption
  int mqttConnectionAttempts    = MQTT_CONNECTION_ATTEMPTS; // Number of attempts to connect to MQTT broker
  bool displayStandby           = false;                    // Display is currently in standby mode
  WeighingMode weighingMode     = weight;                   // Weight per default

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

    // Send consumption
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

  String humanize(const int value)
  {
    const String unit_volumes[2] = {"ml", "l"};
    const String unit_weights[2] = {"g", "kg"};
    const String *units = weighingMode == weight ? &unit_weights[0] : &unit_volumes[0];

    if (value >= 1000)
    {
      return String(value / 1000.0) + units[1];
    }

    return String(value) + units[0];
  }

  float getWeight()
  {
    hx711.update();
    return hx711.getData();
  }

  // returns the weighingMode specific value
  int getValue()
  {
    return (weighingMode == weight)? currentWeight : consumption;
  }

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

  inline void updateDisplay(const char* text, const String str) { updateDisplay(text, str.c_str()); }
  inline void updateDisplay(const String text, const char* str) { updateDisplay(text.c_str(), str); }
  inline void updateDisplay(const String text, const String str) { updateDisplay(text.c_str(), str.c_str()); }

  void updateStatus(const char* statusText)
  {
    updateDisplay(humanize(getValue()), statusText);
  }

  // this method is called, when this mode is activated from the menu
  void start_mode()
  {
  }

  void update_weight()
  {
    auto weight = round(getWeight());

    if (currentWeight != weight)
    {
      currentWeight = weight;
      lastWeighingTime = millis();
      updateStatus("");
      displayStandby = false;
    }
  }

  // this method is called in the loop() if this mode is active
  void update()
  {
    if (weighingMode == weight)
    {
      update_weight();
    }
    else
    {
      // Stabilized?
      if ((millis() - lastWeighingTime > HX711_STABILIZING_INTERVAL) && (currentWeight == round(getWeight())) && (lastWeightSent != currentWeight))
      {
        // Publish state
        lastWeighingTime = millis();

        // publishState((lastWeightSent == 0) ? 0 : currentWeight - lastWeightSent);
        publishState(currentWeight - lastWeightSent);
        displayStandby = false;
      }
    }

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

    mqttClient.loop();

    // TODO: FIND A WAY TO GO TO THE MENU:
    // selected_mode = 0;
  }

  void setupWifi()
  {
    // Stay offline
    if ((WIFI_SSID == "") || (WIFI_PASSWORD == ""))
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

  void setupScale()
  {
    DEBUG_PRINTLN("setupScale(): Initializing...");
    hx711.begin();
    hx711.start(HX711_STARTUP_DELAY);
    hx711.setCalFactor(HX711_CALIBRATION_FACTOR);
  }

  // this method is called at the start of the ESP/Arduino
  void setup()
  {
    modes[next_mode++] = mode(&start_mode, &update, (char *) "weighbridge");
    setupScale();
    setupWifi();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    updateDisplay(humanize(currentWeight), "");
    lastWeighingTime = millis();
  }
}
