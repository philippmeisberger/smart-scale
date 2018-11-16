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
int weight = 0;

// Timestamp of last weighing process
int lastWeighingTime = 0;

// Last published weight
int lastWeightSent = 0;

// Number of attempts to connect to MQTT broker
int mqttConnectionAttempts = MQTT_CONNECTION_ATTEMPTS;

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
}

void setupDisplay()
{
    DEBUG_PRINTLN("setupDisplay(): Initializing...");

    // initialize with the I2C addr 0x3C (for the 128x32)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    
    display.clearDisplay();
    display.setTextColor(WHITE);
    updateDisplay(weight, "");
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
    DEBUG_PRINTF("setupWifi(): Connecting to Wi-Fi access point '%s'\n", WIFI_SSID);

    // Do not store Wi-Fi config in SDK flash area
    WiFi.persistent(false);

    // Disable auto Wi-Fi access point mode
    WiFi.mode(WIFI_STA);

    // Start Wi-Fi connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wi-Fi not yet connected?
    while (WiFi.status() != WL_CONNECTED)
    {
        updateDisplay(weight, "Connecting...");
        DEBUG_PRINT(".");
        delay(500);
    }

    // Wi-Fi connection established
    DEBUG_PRINT("setupWifi(): Connected to Wi-Fi access point. Obtained IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    updateDisplay(weight, "");
}

void updateDisplay(const int consumed, const char* statusText)
{
    display.clearDisplay();

    // Show consumption
    display.setTextSize(3);
    display.setCursor(0, 0);

    // More than 1000
    if (consumed >= 1000)
    {
        display.println(String((float)consumed / 1000) + "kg");
    }
    else
    {
        display.println(String(consumed) + "g");
    }

    // Show status
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.println(statusText);
    display.display();
}

void publishState(const int consumed)
{
    // Connect MQTT broker
    while (!mqttClient.connected())
    {
        DEBUG_PRINTF("publishState(): Connecting to MQTT broker '%s: %i'...\n", MQTT_SERVER, MQTT_PORT);
        
        if (mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD))
        {
            DEBUG_PRINTLN("publishState(): Connected to MQTT broker");
            updateDisplay(weight, "");
        }
        else
        {
            DEBUG_PRINTF("publishState(): Connection failed with error code %i. Try again...\n", mqttClient.state());
            updateDisplay(weight, "MQTT connection failed");

            // Give up?
            if (mqttConnectionAttempts == 0)
            {
                DEBUG_PRINTLN("publishState(): Maximum connection attempts exceeded... Giving up");
                mqttConnectionAttempts = MQTT_CONNECTION_ATTEMPTS;
                return;
            }
                
            mqttConnectionAttempts--;
            delay(2000);
        }
    }

    // Send weight
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& weight = jsonBuffer.createObject();

    // milli liters
    weight["total"] = 0;
    weight["actual"] = consumed;

    char message[weight.measureLength() + 1];
    weight.printTo(message, sizeof(message));

    DEBUG_PRINTF("publishState(): Publishing message on channel '%s': %s\n", MQTT_CHANNEL_STATE, message);
    mqttClient.publish(MQTT_CHANNEL_STATE, message);
}

float getWeight()
{
    hx711.update();
    return hx711.getData();
}

void loop()
{
    // Weight has changed
    if (weight != round(getWeight()))
    {
        // Show weight on display
        lastWeighingTime = millis();
        weight = round(getWeight());
        DEBUG_PRINTF("loop(): %i\n", weight);
        updateDisplay(weight, "");
    }
    else if (weight != 0)
    {
        // Stabilized?
        if ((millis() - lastWeighingTime > HX711_STABILIZING_INTERVAL) && (weight == round(getWeight())) && (lastWeightSent != weight))
        {
            // Publish weight
            lastWeighingTime = millis();
            lastWeightSent = weight;
            DEBUG_PRINTF("loop(): Publishing weight: %i\n", weight);
            publishState(weight);
        }
    }

    mqttClient.loop();
}
