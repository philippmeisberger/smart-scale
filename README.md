# Weighbridge using ESP8266 and Home Assistant

## Example configuration for Home Assistant

This example must be added to the `sensor` block of your configuration.

      - platform: mqtt
        state_topic: "/weighbridge/api/1/state/"
        name: "Total water"
        unit_of_measurement: 'ml'
        value_template: "{{ value_json.total }}"

      - platform: mqtt
        state_topic: "/weighbridge/api/1/state/"
        name: "Actual water"
        unit_of_measurement: 'ml'
        value_template: "{{ value_json.actual }}"
