# remote_base

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