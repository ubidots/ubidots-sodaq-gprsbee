/*
    SODAQ library to using mDot bee. Some function are based on GPRSbee
    library of Kees Bakker
    Copyright (C) 2016  Mateo Velez - Metavix for Ubidots Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



#include <Stream.h>
#include <avr/pgmspace.h>
#ifdef ARDUINO_ARCH_AVR
#include <avr/wdt.h>
#else
#define wdt_reset()
#endif
#include <stdlib.h>

#include "SODAQGPRSbee.h"

Ubidots::Ubidots(char* token, char* server) {
    _vcc33Pin = -1;
    _onoffPin = -1;
    _statusPin = -1;
    _server = server;
    _token = token;
    _dsName = NULL;
    _dsTag = "GPRSbee";
    currentValue = 0;
    val = (Value *)malloc(MAX_VALUES*sizeof(Value));
}
void Ubidots::setOnBee(int vcc33Pin, int onoffPin, int statusPin) {
    init(vcc33Pin, onoffPin, statusPin);
    on();
}

/** // This function was taken from GPRSbee library of Kees Bakker
 * This function is to read the data from GPRS pins. This function is from Adafruit_FONA library
 * @arg timeout, time to delay until the data is transmited
 * @return replybuffer the data of the GPRS
 */
int Ubidots::readLine(uint32_t ts_max) {
    uint32_t ts_waitLF = 0;
    bool seenCR = false;
    int c;
    size_t bufcnt;
    bufcnt = 0;
    while (!isTimedOut(ts_max)) {
        wdt_reset();
        if (seenCR) {
            c = Serial1.peek();
            // ts_waitLF is guaranteed to be non-zero
            if ((c == -1 && isTimedOut(ts_waitLF)) || (c != -1 && c != '\n')) {
                // Line ended with just <CR>. That's OK too.
                goto ok;
            }
      // Only \n should fall through
        }
        c = Serial1.read();
        if (c < 0) {
            continue;
        }
        SerialUSB.print((char)c);  // echo the char
        seenCR = c == '\r';
        if (c == '\r') {
            ts_waitLF = millis() + 50;  // Wait another .05 sec for an optional LF
        } else if (c == '\n') {
            goto ok;
        } else {
            // Any other character is stored in the line buffer
            if (bufcnt < (DEFAULT_BUFFER_SIZE - 1)) {  // Leave room for the terminating NUL
                buffer[bufcnt++] = c;
            }
        }
    }
    SerialUSB.println(F("readLine timed out"));
    return -1;            // This indicates: timed out
    ok:
    buffer[bufcnt] = 0;     // Terminate with NUL byte
    return bufcnt;
}
////////////////////////////////////////////////////////////////////////////////
/////////////////////        GPRS Functions       /////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Ubidots::setDataSourceName(char* dsName) {
    _dsName = dsName;
}
void Ubidots::setDataSourceTag(char* dsTag) {
    _dsTag = dsTag;
}
float Ubidots::getValueWithDatasource(char* dsTag, char* idName) {
    float num;
    int i = 0;
    char allData[300];
    char message[4][50];
    sprintf(message[0], "AT+CIPSEND");
    sprintf(message[2], ">");
    sprintf(message[3], "OK|");
    String response;
    uint8_t bodyPosinit;
    sprintf(allData, "%s|LV|%s|%s:%s|end\n\x1A", USER_AGENT, _token, dsTag, idName);
    for (i = 0; i < 2; i++) {
        if (i != 1) {
            Serial1.println(message[i]);
            if (!waitForMessage(message[i+2], 6000)) {
                SerialUSB.print("Error at ");
                SerialUSB.println(message[i]);
                return false;
            }
        } else {
            Serial1.write(allData);
            if (!waitForMessage(message[i+2], 6000)) {
                SerialUSB.print("Error at ");
                SerialUSB.println(message[i]);
                return false;
            }
            response = String(buffer);
        }
    }
    bodyPosinit = 3 + response.indexOf("OK|");
    response = response.substring(bodyPosinit);
    num = response.toFloat();
    currentValue = 0;
    free(allData);
    return num;
}
bool Ubidots::setApn(char* apn, char* user, char* pwd) {
    char message[9][50];
    int i = 0;
    sprintf(message[0], "AT+CSQ");
    sprintf(message[1], "AT+CIPSHUT");
    sprintf(message[2], "AT+CGATT?");
    sprintf(message[3], "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    sprintf(message[4], "AT+SAPBR=3,1,\"APN\",\"%s\"", apn);
    sprintf(message[5], "AT+SAPBR=3,1,\"USER\",\"%s\"", user);
    sprintf(message[6], "AT+SAPBR=3,1,\"PWD\",\"%s\"", pwd);
    sprintf(message[7], "AT+SAPBR=1,1");
    sprintf(message[8], "AT+SAPBR=2,1");
    for (i = 0; i < 9; i++) {
        if (!sendMessageAndwaitForOK(message[i], 6000)) {
            Serial.print("Error with ");
            Serial.println(message[i]);
            return false;
        }
    }
    return true;
}
void Ubidots::add(char *variableName, float value, char *context) {
    (val+currentValue)->varName = variableName;
    (val+currentValue)->ctext = context;
    (val+currentValue)->varValue = value;
    currentValue++;
    if (currentValue > MAX_VALUES) {
        currentValue = MAX_VALUES;
    }
}
bool Ubidots::sendAll() {
    int i;
    char message[8][50];
    String all;
    String str;
    all = USER_AGENT;
    all += "|POST|";
    all += _token;
    all += "|";
    all += _dsTag;
    if (_dsName != NULL) {
        all += ":";
        all += _dsName;
    }
    all += "=>";
    for (i = 0; i < currentValue; ) {
        str = String(((val + i)->varValue), 2);
        all += String((val + i)->varName);
        all += ":";
        all += str;
        if ((val + i)->ctext != NULL) {
            all += "$";
            all += String((val + i)->ctext);
        }
        all += ",";
        i++;
    }
    all += "|end";
    Serial.println(all.c_str());
    sprintf(message[0], "AT+CIPMUX=0");
    sprintf(message[1], "AT+CIPSTART=\"TCP\",\"%s\",\"%s\"", _server, PORT);
    sprintf(message[2], "AT+CIPSEND");
    sprintf(message[4], "AT+CIPCLOSE");
    sprintf(message[5], ">");
    sprintf(message[6], "SEND OK");
    sprintf(message[7], "CLOSE OK");
    for (i = 0; i < 2; i++) {
        if (!sendMessageAndwaitForOK(message[i], 6000)) {
            Serial.print("Error with ");
            Serial.println(message[i]);
            currentValue = 0;
            return false;
        }
    }
    for (i = 2; i < 5; i++) {
        if (i != 3) {
            Serial1.println(message[i]);
        } else {
            Serial1.write(all.c_str());
            Serial1.write(0x1A);
        }
        if (!waitForMessage(message[i + 3], 6000)) {
            Serial.print("Error with ");
            Serial.println(message[i]);
            currentValue = 0;
            return false;
        }
    }
    currentValue = 0;
    return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////        bee init      /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Initializes the instance
// This function was taken from GPRSbee library of Kees Bakker
void Ubidots::init(int vcc33Pin, int onoffPin, int statusPin) {
    if (vcc33Pin >= 0) {
        _vcc33Pin = vcc33Pin;
        // First write the output value, and only then set the output mode.
        digitalWrite(_vcc33Pin, LOW);
        pinMode(_vcc33Pin, OUTPUT);
    }
    if (onoffPin >= 0) {
        _onoffPin = onoffPin;
        // First write the output value, and only then set the output mode.
        digitalWrite(_onoffPin, LOW);
        pinMode(_onoffPin, OUTPUT);
    }
    if (statusPin >= 0) {
        _statusPin = statusPin;
        pinMode(_statusPin, INPUT);
    }
}
// This function was taken from GPRSbee library of Kees Bakker
void Ubidots::on() {
    // First VCC 3.3 HIGH
    if (_vcc33Pin >= 0) {
        digitalWrite(_vcc33Pin, HIGH);
    }
    // Wait a little
    // TODO Figure out if this is really needed
    delay(200);
    if (_onoffPin >= 0) {
        digitalWrite(_onoffPin, HIGH);
    }
}
// This function was taken from GPRSbee library of Kees Bakker
void Ubidots::off() {
    if (_vcc33Pin >= 0) {
        digitalWrite(_vcc33Pin, LOW);
    }
    // The GPRSbee is switched off immediately
    if (_onoffPin >= 0) {
        digitalWrite(_onoffPin, LOW);
    }
    // Should be instant
    // Let's wait a little, but not too long
    delay(50);
}
// This function was taken from GPRSbee library of Kees Bakker
bool Ubidots::isOn() {
    if (_statusPin >= 0) {
        bool status = digitalRead(_statusPin);
        return status;
    }
    // No status pin. Let's assume it is on.
    return true;
}
// This function was taken from GPRSbee library of Kees Bakker
bool Ubidots::sendMessageAndwaitForOK(char *message, uint16_t timeout) {
    int len;
    uint32_t ts_max = millis() + timeout;
    while ((len = readLine(ts_max)) >= 0) {
        if (len == 0) {
            // Skip empty lines
            continue;
        }
        if (strcmp_P(buffer, PSTR("OK")) == 0) {
            return true;
        } else if (strcmp_P(buffer, PSTR("ERROR")) == 0) {
            return false;
        }
        // Other input is skipped.
    }
    return false;
}
// This function was taken from GPRSbee library of Kees Bakker
bool Ubidots::waitForMessage(const char *msg, uint32_t ts_max) {
    int len;
    while ((len = readLine(ts_max)) >= 0) {
        if (len == 0) {
            // Skip empty lines
            continue;
        }
        if (strncmp(buffer, msg, strlen(msg)) == 0) {
            return true;
        }
    }
    return false;         // This indicates: timed out
}
