/*
    ESP8266 SPI Slave for WiFi connection with Arduino
    Connect the SPI Master device to the following pins on the esp8266:

            ESP8266         |
    GPIO    NodeMCU   Name  |   Uno
  ===================================
     15       D8       SS   |   D10
     13       D7      MOSI  |   D11
     12       D6      MISO  |   D12
     14       D5      SCK   |   D13

    Note: If the ESP is booting at a moment when the SPI Master has the Select line HIGH (deselected)
    the ESP8266 WILL FAIL to boot!
    
    for OTA AVRISP support add connection from pin 4 (D2) to AVR reset pin

    Device to be compiled for: ESP8266

  Copyright (c) 2017 Jiri Bilek. All rights reserved.

  Based on SPISlave example application for ESP8266.
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
  Version history:
  0.1.0 15.03.17 JB  First version
  0.1.1 25.11.17 JB  Fixed UDP protocol
 */

//#define OTA_AVRISP

#include "SPISlave.h"
#include "SPICalls.h"
#include "WiFiSPICmd.h"

#include <ESP8266WiFi.h>

#ifdef OTA_AVRISP
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <ESP8266AVRISP.h>

const uint16_t port = 328;
const uint8_t reset_pin = 4;

ESP8266AVRISP avrprog(port, reset_pin);
#endif

// Library version
const char* VERSION = "0.1.1";


/*
 * Setup
 */
void setup()
{
#ifndef OTA_AVRISP
    WiFi.persistent(false);  // Solving trap in ESP8266WiFiSTA.cpp#144 (wifi_station_ap_number_set)
                             // Relevant for version 2.3.0 of the board SDK software
                             // Erasing of flash memory might help: https://github.com/kentaylor/EraseEsp8266Flash/blob/master/EraseFlash.ino                
#endif
    
    // Serial line for debugging
    Serial.begin(115200);
    Serial.setDebugOutput(true);

#ifdef OTA_AVRISP
    MDNS.begin("arduino");
    MDNS.enableArduino(port);

    WiFiManager wifiManager;
    wifiManager.autoConnect();

    // platform.txt:
    // tools.avrdude.upload.network_pattern="{cmd.path}" "-C{config.path}" -p{build.mcu} -c{upload.protocol}
    //          "-Pnet:{serial.port}:328" "-Uflash:w:{build.path}/{build.project_name}.with_bootloader.hex:i"
    avrprog.begin();
    avrprog.setReset(false); // let the AVR run
#endif

    Serial.printf("\n\nSPI SLAVE ver. %s\n", VERSION);


    // --- Setting callbacks for SPI protocol

    // --- onData
    // Data has been received from the master. Beware that len is always 32
    // and the buffer is autofilled with zeroes if data is less than 32 bytes long
    SPISlave.onData(SPIOnData);

    // --- onDataSent
    // The master has read out outgoing data buffer
    // that buffer can be set with SPISlave.setData
    SPISlave.onDataSent(SPIOnDataSent);

    // --- onStatus
    // Status has been received from the master.
    // The status register is a special register that both the slave and the master can write to and read from.
    // Can be used to exchange small data or status information
    SPISlave.onStatus(SPIOnStatus);

    // --- onStatusSent
    // The master has read the status register
    SPISlave.onStatusSent(SPIOnStatusSent);

    // Setup SPI Slave registers and pins
    SPISlave.begin();

    // Receiver and transmitter state
    setRxStatus(SPISLAVE_RX_READY);
    setTxStatus(SPISLAVE_TX_NODATA);

    // Initialize command processor
    WiFiSpiEspCommandProcessor::init();
}


/**
 * Loop
 */
void loop() {
    // Loop until received data packet
    if (dataReceived) {

        uint8_t dataBuf[32];  // copy of receiver buffer
        
//        uint32_t savedPS = noInterrupts();  // cli();
        memcpy(dataBuf, inputBuffer, 32);
//        xt_wsr_ps(savedPS);  // sei();

//delay(5);  // TODO: delete, only for debugging to slow down the processing

        WiFiSpiEspCommandProcessor::processCommand(dataBuf);

        // First enable the receiver and then enable the code in loop()
        setRxStatus(SPISLAVE_RX_READY);
        dataReceived = false;
    }

#ifdef OTA_AVRISP
    static AVRISPState_t last_state = AVRISP_STATE_IDLE;
    AVRISPState_t new_state = avrprog.update();
    if (last_state != new_state) {
        switch (new_state) {
            case AVRISP_STATE_IDLE: {
                Serial.printf("[AVRISP] now idle\r\n");
                ESP.reset();
                break;
            }
            case AVRISP_STATE_PENDING: {
                Serial.printf("[AVRISP] connection pending\r\n");
                SPISlave.end();
                break;
            }
            case AVRISP_STATE_ACTIVE: {
                Serial.printf("[AVRISP] programming mode\r\n");
                // Stand by for completion
                break;
            }
        }
        last_state = new_state;
    }
    // Serve the client
    if (last_state != AVRISP_STATE_IDLE) {
        avrprog.serve();
    }
#endif
}

