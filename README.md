# Weighbridge using ESP8266 and Home Assistant

## Connection

![connection](https://github.com/philippmeisberger/smart-scale/blob/master/doc/Scale.png)

## Configuration

The firmware must be configured before flashing to ESP8266. Rename `src/Firmware/sketches/Weighbridge/config-sample.h` to `src/Firmware/sketches/Weighbridge/config.h` and change the values like desired.

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
