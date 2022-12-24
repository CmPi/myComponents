# remote_base

I have several old Lacrosse technology weather sensors and I wanted to integrate them in my ESPHome world.

## Hardware

Easy: I have attached a 433MHz receiver (SRX882) to an ESP32.

## Software

This *remote_base* component add the support of the 433MHz protocol for those sensors.
Unfortunately, the original component remote_receiver has not been designed to enable easily the data received to be sent to sensors.
The workaround is therefore to return a string containing between 1 and 3 sensors values separated with semicolon. Each value has the form xxyyz with xx=TX or WS (protocol), yy=device address and z = sensor type (0: temperature, E: hygrometry, P: pression)

YAML configuration example

    external_components:
    - source: 
        type: local
        path: https://github.com/CmPi/myComponents.git

    remote_receiver:
    id: srx882
    pin:
        number: 4
        inverted: false
    mode: 
        input: true
        pullup: false
    tolerance: 35%
    filter: 150us
    dump: 
        - lacrosse

    