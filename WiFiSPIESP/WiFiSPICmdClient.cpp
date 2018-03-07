/*
    SPI Command Processor for ESP8266 communicating as a slave.
    
  Copyright (c) 2017 Jiri Bilek. All rights reserved.

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

#include "WiFiSPICmd.h"
#include "SPICalls.h"
#include <ESP8266WiFi.h>

/*
 * 
 */
void WiFiSpiEspCommandProcessor::cmdStartClientTcp() {
    uint8_t cmd = data[2];
    
    // Get and test the parameters (4 input parameters)
    if (data[3] != 4) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }
    
    int32_t ipAddr;
    uint16_t port;
    uint8_t sock;
    uint8_t protocol;  // only TCP

    uint8_t dataPos = 4;  // Position in the input buffer

    // Read parameters
    if (getParameter(data, dataPos, reinterpret_cast<uint8_t*>(&ipAddr), sizeof(ipAddr)) < 0)
        return;  // Failure - received invalid parameter
    if (getParameter(data, dataPos, reinterpret_cast<uint8_t*>(&port), sizeof(port)) < 0)
        return;  // Failure - received invalid parameter
    if (getParameter(data, dataPos, &sock, sizeof(sock)) < 0)
        return;  // Failure - received invalid parameter
    if (getParameter(data, dataPos, &protocol, sizeof(protocol)) < 0)
        return;  // Failure - received invalid parameter
    if (sock >= MAX_SOCK_NUM)
        return;  // Invalid socket number
    
    if (data[dataPos] != END_CMD) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }

    setTxStatus(SPISLAVE_TX_PREPARING_DATA);
    
    #ifdef _DEBUG
        Serial.printf("WifiClient.connect, sock=%d, ip=%s, port=%d\n", sock, IPAddress(ipAddr).toString().c_str(), port);
    #endif

    uint8_t status = clients[sock].connect(IPAddress(ipAddr), port);

    replyStart(cmd, 1);
    replyParam(&status, 1);
    replyEnd();
}

/*
 * 
 */
void WiFiSpiEspCommandProcessor::cmdGetClientStateTcp() {
    uint8_t cmd = data[2];
    
    // Get and test the input parameter
    if (data[3] != 1 || data[4] != 1 || data[6] != END_CMD) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }

    uint8_t sock = data[5];
    if (sock >= MAX_SOCK_NUM)
        return;  // Invalid socket number
    
    uint8_t status = clients[sock].connected();  // 1 = connected

    // Is it a call of a closed client created in a server connection? 
    // Check if the server has connection
    if (! status && servers[sock] != NULL) {
        clients[sock] = servers[sock]->available(NULL);
        status = clients[sock].connected();  // 1 = connected
    }
    
    replyStart(cmd, 1);
    replyParam(&status, 1);
    replyEnd();
}    

/*
 * Works for TCP and UDP connection
 */
void WiFiSpiEspCommandProcessor::cmdAvailDataTcp() {
    uint8_t cmd = data[2];
    
    // Get and test the input parameter
    if (data[3] != 1 || data[4] != 1 || data[6] != END_CMD) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }

    uint8_t sock = data[5];
    if (sock >= MAX_SOCK_NUM)
        return;  // Invalid socket number

    int16_t avail;
    
    if (serversUDP[sock] == NULL)
        avail = clients[sock].available();  // TCP
    else
        avail = serversUDP[sock]->available();  // UDP 

    #ifdef _DEBUG
        Serial.printf("Avail[%d] = %d\n", sock, avail);
    #endif
    
    replyStart(cmd, 1);
    replyParam(reinterpret_cast<const uint8_t *>(&avail), sizeof(avail));
    replyEnd();
}    

/*
 * 
 */
void WiFiSpiEspCommandProcessor::cmdSendDataTcp() {
    uint8_t cmd = data[2];
    
    // Get and test the input parameter
    if (data[3] != 2 || data[4] != 1) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }

    uint8_t sock = data[5];
    if (sock >= MAX_SOCK_NUM)
        return;  // Invalid socket number

    uint16_t len = data[6] | (data[7] << 8);

    // Limit the length to 4000 characters
    // TODO: Remove the limit
    if (len > 4000) {
        #ifdef _DEBUG        
            Serial.println(F("Too much data (>4000 bytes)."));
        #endif        
        return;  // Failure
    }
    
    uint8_t dataPos = 8;  // Position in the input buffer

    // Allocate a buffer
    uint8_t* buffer = static_cast<uint8_t*>(malloc(len));
    if (buffer == NULL) {
        #ifdef _DEBUG
            Serial.printf("Malloc (%d) failed.\n", len);
        #endif
        return;  // Failure
    }

    // Read input data into the buffer
    uint8_t* buf = buffer;
    for (uint16_t i = 0;  i < len;  ++i)
    {
        // Get next character
        int16_t b = readByte(data, dataPos);
        if (b < 0) {
            #ifdef _DEBUG
                Serial.println(F("Not enough data."));
            #endif
            return;  // Failure
        }
    
        *buf++ = b;
    }

    len = clients[sock].write(static_cast<const uint8_t*>(buffer), len);
    free(buffer);

    replyStart(cmd, 1);
    replyParam(reinterpret_cast<const uint8_t *>(&len), sizeof(len));
    replyEnd();
}    

/*
 * 
 */
void WiFiSpiEspCommandProcessor::cmdGetDataTcp() {
    uint8_t cmd = data[2];
    
    // Get and test the input parameter
    if (data[3] != 2 || data[4] != 1 || data[6] != 1 || data[8] != END_CMD) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }

    uint8_t sock = data[5];
    if (sock >= MAX_SOCK_NUM)
        return;  // Invalid socket number
    
    uint8_t peek = data[7];
    
    int16_t reply = -1;
    int16_t avail;

    if (serversUDP[sock] == NULL)
        avail = clients[sock].available();  // TCP
    else
        avail = serversUDP[sock]->available();  // UDP 

    if (avail) {
        // Read/peek one character
         
        if (peek) {
            if (serversUDP[sock] == NULL)
                reply = clients[sock].peek();
            else
                reply = serversUDP[sock]->peek();
        } else {
            if (serversUDP[sock] == NULL)
                reply = clients[sock].read();
            else
                reply = serversUDP[sock]->read();
        }
    }
    
    replyStart(cmd, 1);
    replyParam(reinterpret_cast<const uint8_t *>(&reply), sizeof(reply));
    replyEnd();
}    

/*    
 *      
 */ 
void WiFiSpiEspCommandProcessor::cmdGetDatabufTcp() {
    uint8_t cmd = data[2];
    
    // Get and test the input parameter
    if (data[3] != 2 || data[4] != 1 || data[6] != 2 || data[9] != END_CMD) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }

    uint8_t sock = data[5];
    if (sock >= MAX_SOCK_NUM)
        return;  // Invalid socket number
    
    uint16_t len = data[7] | (data[8] << 8);

    // Allocate a buffer
    uint8_t* buffer = static_cast<uint8_t*>(malloc(len));
    if (buffer == NULL) {
        #ifdef _DEBUG
            Serial.printf("Malloc (%d) failed.\n", len);
        #endif
        return;  // Failure
    }

    // Read the data into the buffer
    if (serversUDP[sock] == NULL)
        len = clients[sock].read(buffer, len);
    else
        len = serversUDP[sock]->read(buffer, len);

    replyStart(cmd, 1);
    replyParam16(buffer, len);
    replyEnd();
    
    free(buffer);
}

/*
 * 
 */
void WiFiSpiEspCommandProcessor::cmdStopClientTcp() {
    uint8_t cmd = data[2];
    
    // Get and test the input parameter
    if (data[3] != 1 || data[4] != 1 || data[6] != END_CMD) {
        Serial.println(FPSTR(INVALID_MESSAGE_BODY));
        return;  // Failure - received invalid message
    }

    uint8_t sock = data[5];
    if (sock >= MAX_SOCK_NUM)
        return;  // Invalid socket number
    
    clients[sock].stop();

    uint8_t status = 0;
    replyStart(cmd, 1);
    replyParam(&status, 1);
    replyEnd();
}    

