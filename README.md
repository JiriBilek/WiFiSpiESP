WiFiESPSpi
==========

This application is intended to run on ESP8266 module with accessible HSPI interface (e.g. ESP-12).

The application implements custom SPI protocol on HSPI interface of ESP8266 module and enables to use it as a dedicated WiFi slave device - see below. The SPI interface is used because of its strictly master/slave nature.

## Requirements

### ESP8266 device

The ESP8266 has two SPI interfaces. The first is connected to the flash memory, the second (named HSPI) can be used by user. The HSPI signals are multiplexed with GPIO12-15. Suitable device for the slave is ESP-12.

### Client library for Arduino AVR family

While this project enables the SPI communication as a slave device, on master devices the corresponding library must be loaded.
For Arduino AVR devices the library is [WifiSpi](https://github.com/JiriBilek/WiFiSpi).

#### Wiring

The library uses hardware SPI on Arduino so most of the signals are fixed to certain pins.

The wiring is as follows:

     Name  |   Uno   |      ESP8266
           |         |  GPIO    NodeMCU
    -------+---------+-----------------
      SS   |   D10   |   15       D8
     MOSI  |   D11   |   13       D7
     MISO  |   D12   |   12       D6
     SCK   |   D13   |   14       D5

Please be careful, the ESP8266 chip ports are **NOT** 5V tolerant, if you're connecting to a 5V device you have to use a level converter.

## Installing the application

The simplest way to do it is to use the Arduino IDE with support for ESP8266.

### Installing the Arduino IDE for ESP8266
 
The Arduino IDE environment is capable to compile and flash programs for ESP8266 modules.
The installation process is described on [esp8266/arduino](https://github.com/esp8266/Arduino) github pages.
 
### Compiling and flashing

Don't forget to change the board values in the Arduino environment (menu Tools - Board and next).
For flashing you will need to connect the USB to Serial converter to Rx and Tx pins and put CH_PD high (3.3V) and GPIO15 low (GND).
Put GPIO0 low (GND) and reset the chip just before flashing begins. This is a standard procedure and you can find details anywhere on the Internet.
If you are using modules with USB connection (NodeMCU, e.g.), all you have to do is to connect the module to your USB port. 

## Optional OTA AVR ISP

If enabled, the WiFi SPI firmware can upload a sketch over WiFi to attached AVR microcontroller. [ESP8266AVRISP library](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266AVRISP) is used. 

OTA AVR ISP requires a connection from esp8266 pin to AVR reset pin. Default in the firmware source code is pin 4 (labeled D2 on NodeMCU and Wemos boards).

For OTA AVR ISP the esp8266 must be connected to network from start. [WiFiManager library](https://github.com/tzapu/WiFiManager) is used to handle the connection. With WiFi Manager active, do not use WiFiSpi.begin() in the AVR sketch. Only wait while `WiFiSpi.status() != WL_CONNECTED`.

For OTA upload from the IDE, change the key `tools.avrdude.upload.network_pattern` in `hardware/arduino/avr/platform.txt` in Arduino IDE installation folder to
```
tools.avrdude.upload.network_pattern="{cmd.path}" "-C{config.path}" -p{build.mcu} -c{upload.protocol} "-Pnet:{serial.port}:328" "-Uflash:w:{build.path}/{build.project_name}.with_bootloader.hex:i"
```
Note the with_bootloader.hex. ISP programming doesn't need a bootloader and it erases an existing one. Using hex with bootloader ensures the bootloader for serial upload over USB.

To enabled OTA AVR ISP, uncomment the `#define OTA_AVRISP` in WiFiSPIESP.ino before uploading the firmware.
 
 
## ToDo and Wish Lists

- TLS connection
- SPI protocol optimization

## Credits

The protocol running on SPI interface was inspired by protocol used in Arduino WiFi library. The code structure and some code is taken from this library, too.

The SPI master code was inspired and parts were taken from the [ESPSlave example](https://github.com/esp8266/Arduino/tree/master/libraries/SPISlave) of the ESP8266 Arduino project.



