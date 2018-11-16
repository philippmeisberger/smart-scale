//#include <ESP8266WiFi.h>
//#include <PubSubClient.h>
//#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711_ADC.h>

#include "config.h"

#define FIRMWARE_VERSION "0.1"

/*WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient = PubSubClient(wifiClient);
const int BUFFER_SIZE = JSON_OBJECT_SIZE(20);*/

// Connect SCL to D1 and SDA to D2
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

// Connect black to E+, red to E-, green to A+, white to A-
// Connect SCK to D8, DT to D3
HX711_ADC hx711(D3, D8);

int weight, lastWeightSent = 0;
int t = 0;

void setup()
{
    Serial.begin(115200);
    delay(250);
    Serial.printf("ESP8266 Smart Weighbridge '%s'\n", FIRMWARE_VERSION);
    setupDisplay();
    setupScale();
    /*setupWifi();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);*/
}

void setupDisplay()
{
    Serial.println("setupDisplay(): Initializing ...");

    // initialize with the I2C addr 0x3C (for the 128x32)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    
    display.clearDisplay();
    display.setTextColor(WHITE);
    updateDisplay(0, "");
}

void setupScale()
{
  Serial.println("setupScale(): Initializing ...");
  hx711.begin();
  hx711.start(0);
  hx711.setCalFactor(392.0);
}

/*void setupWifi()
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
        updateStatus("Connecting ...");
        Serial.print(".");
        delay(500);
    }

    // Wi-Fi connection established
    Serial.print("setupWifi(): Connected to Wi-Fi access point. Obtained IP address: ");
    Serial.println(WiFi.localIP());
    updateStatus("");
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
            publishState(0);
            updateStatus("");
        }
        else
        {
            Serial.printf("connect(): Connection failed with error code %i. Try again...\n", mqttClient.state());
            updateStatus("MQTT connection failed");
            delay(2000);
        }
    }
}*/

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
    updateStatus(statusText);
}

void updateStatus(const char* statusText)
{
    display.setTextSize(1);
    display.setCursor(0, 24);
    display.println(statusText);
    display.display();
}

/*void publishState(const int consumed)
{
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject& weight = jsonBuffer.createObject();

    // milli liters
    weight["total"] = 2500;
    weight["actual"] = consumed;

    char message[weight.measureLength() + 1];
    weight.printTo(message, sizeof(message));

    Serial.printf("publishState(): Publish message on channel '%s': %s\n", MQTT_CHANNEL_STATE, message);
    mqttClient.publish(MQTT_CHANNEL_STATE, message);
}*/

float getWeight()
{
    hx711.update();
    return hx711.getData();
}

void loop()
{   
    if (weight != round(getWeight()))
    {
        t = millis();
        weight = round(getWeight());
        Serial.print("loop(): ");
        Serial.println(weight);
        updateDisplay(weight, "");
    }
    else if (weight != 0)
    {
        // Stabilized?
        if ((millis() - t > 2000) && (weight == round(getWeight())) && (lastWeightSent != weight))
        {
            t = millis();
            lastWeightSent = weight;
            Serial.print("loop(): Sending weight: ");
            Serial.println(weight);
            //connectMqtt();
        }

        //mqttClient.loop();
    }
}
