#define DEBUG                         // Enable debug messages on serial

#define WIFI_SSID                     ""
#define WIFI_PASSWORD                 ""
#define WIFI_CONNECTING_INTERVAL      8000

#define HX711_STARTUP_DELAY           0
#define HX711_CALIBRATION_FACTOR      392.0  // 5kg sensor
#define HX711_STABILIZING_INTERVAL    2000
#define HX711_THRESHOLD               1      // Reduce weight variations (more than 1ml must be drunk)

#define OLED_WIDTH                    128    // OLED display width in pixels
#define OLED_HEIGHT                   64     // OLED display height in pixels
#define OLED_TIMEOUT                  30000  // OLED timeout in milliseconds

#define MQTT_CLIENTID                 "weighbridge"
#define MQTT_SERVER                   "hassio.local"
#define MQTT_PORT                     1883
#define MQTT_USERNAME                 MQTT_CLIENTID
#define MQTT_PASSWORD                 ""
#define MQTT_CHANNEL_STATE            "/weighbridge/api/1/state/"
