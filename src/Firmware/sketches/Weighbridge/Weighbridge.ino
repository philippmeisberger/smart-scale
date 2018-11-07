#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
//#include <HX711_ADC.h>

#include "config.h"

#define FIRMWARE_VERSION "0.1"

WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient = PubSubClient(wifiClient);
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

// Connect SCL to D1 and SDA to D2
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

void setup()
{
    Serial.begin(115200);
    delay(250);
    Serial.printf("ESP8266 Smart Weighbridge '%s'\n", FIRMWARE_VERSION);
  
    // initialize with the I2C addr 0x3C (for the 128x32)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    setupWifi();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void setupWifi()
{
    Serial.printf("setupWifi(): Connecting to Wi-Fi access point '%s'\n", WIFI_SSID);

    // Do not store Wi-Fi config in SDK flash area
    WiFi.persistent(false);

    // Disable auto Wi-Fi access point mode
    WiFi.mode(WIFI_STA);

    // Start Wi-Fi connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wi-Fi not yet connected?
    while (WiFi.status() != WL_CONNECTED)
    {
        updateDisplay(250, "Connecting ...");
        Serial.print(".");
        delay(500);
    }

    // Wi-Fi connection established
    Serial.print("setupWifi(): Connected to Wi-Fi access point. Obtained IP address: ");
    Serial.println(WiFi.localIP());
    updateDisplay(250, "");
}

void connectMqtt()
{
    while (!mqttClient.connected())
    {
        Serial.printf("connect(): Connecting to MQTT broker '%s:%i'...", MQTT_SERVER, MQTT_PORT);
        
        if (mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD))
        {
            Serial.println("connect(): Connected to MQTT broker");

            // Publish initial state
            publishState(250);
        }
        else
        {
            Serial.printf("connect(): Connection failed with error code %i. Try again...\n", mqttClient.state());
            updateDisplay(250, "MQTT connection failed");
            delay(2000);
        }
    }
}

void updateDisplay(const int consumed, const char* statusText)
{
    display.clearDisplay();
    display.setTextColor(WHITE);

    // Show consumption
    display.setTextSize(3);
    display.setCursor(0, 0);

    // Consumed more than 1000ml
    if (consumed >= 1000)
    {
        display.print(String(consumed / 1000) + "l");
    }
    else
    {
        display.print(String(consumed) + "ml");
    }
    
    // Show status
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.print(statusText);

    display.display();
}

void publishState(const int consumed)
{
    updateDisplay(consumed, "");
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& weight = jsonBuffer.createObject();

    // milli liters
    weight["total"] = 2500;
    weight["actual"] = consumed;

    char message[weight.measureLength() + 1];
    weight.printTo(message, sizeof(message));

    Serial.printf("publishState(): Publish message on channel '%s': %s\n", MQTT_CHANNEL_STATE, message);
    mqttClient.publish(MQTT_CHANNEL_STATE, message);
}

void loop()
{
    // TODO: Calculate weight
    connectMqtt();
    mqttClient.loop();
}
