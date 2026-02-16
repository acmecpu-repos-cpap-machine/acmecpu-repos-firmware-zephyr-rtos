# C201 ESP32 Firmware

Repository to contain source files for the ESP32 based firmware for Control Board Model C201

## CLI commands to build, flash and monitor

- First export the environment by
`. $HOME/esp/esp-idf/export.sh`

- to build run
`idf.py build`

- to flash and monitor run (check your USB number by `ls -l /dev/ttyUSB*`)
`idf.py -p /dev/ttyUSB0 flash monitor`

- to erase the flash memory run
`idf.py -p /dev/ttyUSB0 erase_flash`

## Use Eclipse IDE to build, flash, monitor and debug

[GitHub - espressif/idf-eclipse-plugin: Eclipse plugin for ESP-IDF CMake based projects (4.x and above)](https://github.com/espressif/idf-eclipse-plugin#installing-idf-plugin-using-update-site-url)


[Using Debugger - ESP32 - — ESP-IDF Programming Guide v4.2 documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/jtag-debugging/using-debugger.html#jtag-debugging-using-debugger-eclipse)

[JTAG Debugging - ESP32 - — ESP-IDF Programming Guide v4.2 documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/jtag-debugging/index.html)

[esp-iot-solution/ESP-Prog_guide_en.md at master · espressif/esp-iot-solution](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP-Prog_guide_en.md)
