#define DEBUG

#define WIFI_SSID                     ""
#define WIFI_PASSWORD                 ""

#define HX711_STARTUP_DELAY           0
#define HX711_CALIBRATION_FACTOR      392.0   // 5kg sensor
#define HX711_STABILIZING_INTERVAL    2000

#define LCD_TIMEOUT                   30000

#define MQTT_CLIENTID                 "weighbridge"
#define MQTT_SERVER                   "hassio.local"
#define MQTT_PORT                     1883
#define MQTT_USERNAME                 MQTT_CLIENTID
#define MQTT_PASSWORD                 ""
#define MQTT_CHANNEL_STATE            "/weighbridge/api/1/state/"
#define MQTT_CONNECTION_ATTEMPTS      3
#define MQTT_CONNECTION_ATTEMPT_DELAY 2000
