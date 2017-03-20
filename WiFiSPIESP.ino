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

#include "SPISlave.h"
#include "SPICalls.h"
#include "WiFiSPICmd.h"

#include <ESP8266WiFi.h>

// Library version
const char* VERSION = "0.1.0";


/*
 * Setup
 */
void setup()
{
    WiFi.persistent(false);  // Solving trap in ESP8266WiFiSTA.cpp#144 (wifi_station_ap_number_set)
                             // Relevant for version 2.3.0 of the board SDK software
                             // Erasing of flash memory might help: https://github.com/kentaylor/EraseEsp8266Flash/blob/master/EraseFlash.ino                
    
    // Serial line for debugging
    Serial.begin(115200);
    Serial.setDebugOutput(true);

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
}

