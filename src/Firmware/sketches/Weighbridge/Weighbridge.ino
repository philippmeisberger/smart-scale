#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711_ADC.h>

#include "config.h"
#include "logging.h"

#define FIRMWARE_VERSION "0.2"

WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient = PubSubClient(wifiClient);

// Connect SCL to D1 and SDA to D2
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

// Connect black to E+, red to E-, green to A+, white to A-
// Connect SCK to D8, DT to D3
HX711_ADC hx711(D3, D8);

const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

// Current weight
int currentWeight = 0;

// Timestamp of last weighing process
int lastWeighingTime = 0;

// Display is currently in standby mode
bool displayStandby = false;

// Last published weight
int lastWeightSent = 0;

// Overall consumption
int consumption = 0;

// Number of attempts to connect to MQTT broker
int mqttConnectionAttempts = MQTT_CONNECTION_ATTEMPTS;

enum WeighingMode
{
    // Show mass in g/kg
    weight,

    // Show water consumption in ml/l
    volume
};

// Weight per default
WeighingMode weighingMode = weight;

void setup()
{
  #ifdef DEBUG
    Serial.begin(115200);
    delay(250);
    Serial.printf("ESP8266 Smart Weighbridge '%s'\n", FIRMWARE_VERSION);
  #endif
    setupDisplay();
    setupScale();
    setupWifi();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    updateDisplay(currentWeight, "");
    lastWeighingTime = millis();
}

void setupDisplay()
{
    DEBUG_PRINTLN("setupDisplay(): Initializing...");

    // initialize with the I2C addr 0x3C (for the 128x32)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.display();
}

void setupScale()
{
    DEBUG_PRINTLN("setupScale(): Initializing...");
    hx711.begin();
    hx711.start(HX711_STARTUP_DELAY);
    hx711.setCalFactor(HX711_CALIBRATION_FACTOR);
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

void updateDisplay(const int value, const char* statusText)
{
    String text;

    switch(weighingMode)
    {
        case weight:
            if (value >= 1000)
            {
                text = String((float)value / 1000) + "kg";
            }
            else
            {
                text = String(value) + "g";
            }

            break;

        case volume:
            if (value >= 1000)
            {
                text = String((float)value / 1000) + "l";
            }
            else
            {
                text = String(value) + "ml";
            }

            break;

        default:
            return;
    }

    updateDisplay(text.c_str(), statusText);
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

void updateStatus(const char* statusText)
{
    updateDisplay((weighingMode == weight)? currentWeight : consumption, statusText);
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

    // Send weight
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

float getWeight()
{
    hx711.update();
    return hx711.getData();
}

void loop()
{
    // Weight has changed
    if (currentWeight != round(getWeight()))
    {
        // Show weight on display
        lastWeighingTime = millis();
        currentWeight = round(getWeight());
        DEBUG_PRINTF("loop(): %i\n", currentWeight);
        updateDisplay((weighingMode == weight)? currentWeight : consumption, "");
        displayStandby = false;
    }
    else if (currentWeight != 0)
    {
        // Stabilized?
        if ((millis() - lastWeighingTime > HX711_STABILIZING_INTERVAL) && (currentWeight == round(getWeight())) && (lastWeightSent != currentWeight))
        {
            // Publish state
            lastWeighingTime = millis();

            // Only publish volume
            if (weighingMode == volume)
            {
                publishState((lastWeightSent == 0)? 0 : currentWeight - lastWeightSent);
            }
            else
            {
                lastWeightSent = currentWeight;
            }

            displayStandby = false;
        }
    }

  #if LCD_TIMEOUT > 0
    // Turn off display after some time
    if (!displayStandby && (millis() - lastWeighingTime > LCD_TIMEOUT))
    {
        DEBUG_PRINTLN("Turning off display");
        display.clearDisplay();
        display.display();
        displayStandby = true;
    }
  #endif

    mqttClient.loop();
}
