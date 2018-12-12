# Weighbridge using ESP8266 and Home Assistant

Drinking enough water every day is important. This gadget helps to monitor your drinking behavior.

## Dependencies

* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
* [HX711_ADC](https://github.com/olkal/HX711_ADC)
* [PubSubClient](https://github.com/knolleary/pubsubclient)

## Connection

![connection](https://github.com/philippmeisberger/smart-scale/blob/master/doc/Scale.png)

## Configuration

The firmware must be configured before flashing to ESP8266. Rename `src/Firmware/Weighbridge/config-sample.h` to `src/Firmware/Weighbridge/config.h` and change the values like desired.

## Example configuration for Home Assistant

This example must be added to the `sensor` block of your configuration.

      - platform: mqtt
        state_topic: "/weighbridge/api/1/state/"
        name: "Actual water consumption"
        unit_of_measurement: 'ml'
        value_template: "{{ value_json.consumed }}"

      - platform: mqtt
        state_topic: "/weighbridge/api/1/state/"
        name: "Total water consumption"
        unit_of_measurement: 'ml'
        value_template: "{{ value_json.consumption }}"
