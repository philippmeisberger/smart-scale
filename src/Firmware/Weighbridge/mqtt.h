#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "logging.h"

#ifndef MQTT_H
#define MQTT_H

WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient = PubSubClient(wifiClient);

int publish(const char* channel, const char* message)
{
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  // Connect MQTT broker
  while (!mqttClient.connected())
  {
    DEBUG_PRINTF("publish(): Connecting to MQTT broker '%s:%i'...\n", MQTT_SERVER, MQTT_PORT);

    if (!mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD))
    {
      DEBUG_PRINTF("publish(): Connection failed with error code %i\n", mqttClient.state());
      return mqttClient.state();
    }
  }

  DEBUG_PRINTF("publish(): Publishing message on channel '%s': %s\n", channel, message);
  return mqttClient.publish(channel, message);
}

#endif //MQTT_H
