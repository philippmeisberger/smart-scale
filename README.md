# Weighbridge using ESP8266 and Home Assistant

[![Build Status](https://travis-ci.org/philippmeisberger/smart-scale.svg?branch=master)](https://travis-ci.org/philippmeisberger/smart-scale)

Drinking enough water every day is important. This gadget helps to monitor your drinking behavior.

## Dependencies

The project uses the following libraries:

* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
* [HX711_ADC](https://github.com/olkal/HX711_ADC)
* [PubSubClient](https://github.com/knolleary/pubsubclient)

## 3D model

The 3D model looks like this:

![3D model](https://github.com/philippmeisberger/smart-scale/blob/master/doc/model.png)

The file for printing the model can be found here: [3D model](https://github.com/philippmeisberger/smart-scale/blob/master/doc/model.zip)

## Connection

The scale is connected as follows:

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

      - platform: mqtt
        state_topic: "/snake/api/1/state/"
        name: "The Score you reached in a game of snake"
        unit_of_measurement: 'points'
        value_template: "{{ value_json.score }}"

      - platform: mqtt
        state_topic: "/snake/api/1/state/"
        name: "The Time you spend in the game"
        unit_of_measurement: 'seconds'
        value_template: "{{ value_json.time_played }}"
