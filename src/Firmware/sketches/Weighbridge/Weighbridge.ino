#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "config.h"

#define FIRMWARE_VERSION "0.1"

WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient = PubSubClient(wifiClient);
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);

void setup()
{
    Serial.begin(115200);
    delay(250);
    Serial.printf("ESP8266 Smart Weighbridge '%s'\n", FIRMWARE_VERSION);
  
  #ifdef PIN_STATUSLED
    pinMode(PIN_STATUSLED, OUTPUT);
  #endif

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
        // Blink 2 times when connecting
        blinkStatusLED(2);
        Serial.print(".");
        delay(500);
    }

    // Wi-Fi connection established
    Serial.print(F("setupWifi(): Connected to Wi-Fi access point. Obtained IP address: "));
    Serial.println(WiFi.localIP());
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
            publishState();
        }
        else
        {
            Serial.printf("connect(): Connection failed with error code %i. Try again...\n", mqttClient.state());
            blinkStatusLED(3);
            delay(2000);
        }
    }
}

void publishState()
{
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& weight = jsonBuffer.createObject();

    // milli liters
    weight["total"] = 2500;
    weight["actual"] = 250;

    char message[weight.measureLength() + 1];
    weight.printTo(message, sizeof(message));

    Serial.printf("publishState(): Publish message on channel '%s': %s\n", MQTT_CHANNEL_STATE, message);
    mqttClient.publish(MQTT_CHANNEL_STATE, message);
}

void blinkStatusLED(const int times)
{
  #ifdef PIN_STATUSLED
    for (int i = 0; i < times; i++)
    {
        // Enable LED (LOW = on)
        digitalWrite(PIN_STATUSLED, LOW);
        delay(100);

        // Disable LED (HIGH = off)
        digitalWrite(PIN_STATUSLED, HIGH);
        delay(100);
    }
  #endif
}

void loop()
{
    // TODO: Calculate weight
    connectMqtt();
    mqttClient.loop();
}
