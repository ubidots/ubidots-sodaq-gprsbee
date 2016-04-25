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

Ubidots::Ubidots(char* token) {
    _vcc33Pin = -1;
    _onoffPin = -1;
    _statusPin = -1;
    _token = token;

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
    SerialUSB.print((char)c);                 // echo the char
    seenCR = c == '\r';
    if (c == '\r') {
      ts_waitLF = millis() + 50;        // Wait another .05 sec for an optional LF
    } else if (c == '\n') {
      goto ok;
    } else {
      // Any other character is stored in the line buffer
      if (bufcnt < (DEFAULT_BUFFER_SIZE - 1)) {    // Leave room for the terminating NUL
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
    String response;
    uint8_t bodyPosinit;
    sprintf(allData, "%s|LV|%s|%s:%s|end", USER_AGENT, _token, dsTag, idName);
    Serial1.println("AT+CIPSEND");
    if (!waitForMessage(">", 6000)) {
      SerialUSB.println("Error at CIPSEND");
      return false;
    }
    Serial1.write(allData);
    Serial1.write(0x1A);
    if (!waitForMessage("SEND OK", 6000)) {
      SerialUSB.println("Error sending the message");
      return false;
    }
    bodyPosinit = 3 + response.indexOf("OK|");
    response = response.substring(bodyPosinit);
    num = response.toFloat();
    currentValue = 0;
    free(allData);
    return num;
}
bool Ubidots::setApn(char* apn, char* user, char* pwd) {
    Serial1.println("AT+CSQ");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error at CSQ");
      return false;
    }
    Serial1.println("AT+CIPSHUT");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error at CSQ");
      return false;
    }
    Serial1.println("AT+CGATT?");
    if (!waitForOK(6000)) {
      SerialUSB.println("GPRS is not attached");
      return false;
    }
    Serial1.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error at setting up CONTYPE");
      return false;
    }
    Serial1.print("AT+SAPBR=3,1,\"APN\",\"");
    Serial1.print(apn);
    Serial1.println("\"");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error at setting up APN");
      return false;
    }
    Serial1.print("AT+SAPBR=3,1,\"USER\",\"");
    Serial1.print(user);
    Serial1.println("\"");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error at setting up apn user");
      return false;
    }
    Serial1.print("AT+SAPBR=3,1,\"PWD\",\"");
    Serial1.print(pwd);
    Serial1.println("\"");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error at setting up apn pass");
      return false;
    }
    Serial1.println("AT+SAPBR=1,1");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error with AT+SAPBR=1,1 Connection ip");
      return false;
    }
    Serial1.println("AT+SAPBR=2,1");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error with AT+SAPBR=2,1 no IP to show");
      return false;
    }
    return true;
}
void Ubidots::add(char *variableName, float value, char *context) {
    (val+currentValue)->varName = variableName;
    (val+currentValue)->ctext = context;
    (val+currentValue)->varValue = value;
    currentValue++;
    if (currentValue > MAX_VALUE) {
        currentValue = MAX_VALUE;
    }
}
bool Ubidots::sendAll() {

    int i;
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
        str = String(((val + i)->varValue), 5);
        all += String((val + i)->varName);
        all += ":";
        all += str;
        if ((val + i)->ctext != NULL) {
            all += "$";
            all += String((val + i)->ctext);
        }
        i++;
        if(i < currentValue){
            all += ","; 
        }
    }
    all += "|end";
    Serial.println(all.c_str());
    Serial1.println("AT+CIPMUX=0");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error CIPMUX=0");
      return false;
    }
    Serial1.print("AT+CIPSTART=\"TCP\",\"");
    Serial1.print(SERVER);
    Serial1.print("\",\"");
    Serial1.print(PORT);
    Serial1.println("\"");
    if (!waitForOK(6000)) {
      SerialUSB.println("Error at CIPSTART");
      return false;
    }
    Serial1.println("AT+CIPSEND");
    if (!waitForMessage(">", 6000)) {
      SerialUSB.println("Error at CIPSEND");
      return false;
    }
    Serial1.write(all.c_str());
    Serial1.write(0x1A);
    if (!waitForMessage("SEND OK", 6000)) {
      SerialUSB.println("Error sending the message");
      return false;
    }
    Serial1.println("AT+CIPCLOSE");
    if (!waitForMessage("CLOSE OK", 6000)) {
      SerialUSB.println("Error closing TCP connection");
      return false;
    }
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
bool Ubidots::waitForOK(uint16_t timeout)
{
  int len;
  uint32_t ts_max = millis() + timeout;
  while ((len = readLine(ts_max)) >= 0) {
    if (len == 0) {
      // Skip empty lines
      continue;
    }
    if (strcmp_P(buffer, PSTR("OK")) == 0) {
      return true;
    }
    else if (strcmp_P(buffer, PSTR("ERROR")) == 0) {
      return false;
    }
    // Other input is skipped.
  }
  return false;
}
// This function was taken from GPRSbee library of Kees Bakker
bool Ubidots::waitForMessage(const char *msg, uint32_t ts_max)
{
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
// This function was taken from GPRSbee library of Kees Bakker
bool Ubidots::waitForMessage_P(const char *msg, uint32_t ts_max)
{
  int len;
  while ((len = readLine(ts_max)) >= 0) {
    if (len == 0) {
      // Skip empty lines
      continue;
    }
    if (strncmp_P(buffer, msg, strlen_P(msg)) == 0) {
      return true;
    }
  }
  return false;         // This indicates: timed out
}
// This function was taken from GPRSbee library of Kees Bakker
int Ubidots::waitForMessages(PGM_P msgs[], size_t nrMsgs, uint32_t ts_max)
{
  int len;
  while ((len = readLine(ts_max)) >= 0) {
    if (len == 0) {
      // Skip empty lines
      continue;
    }
    for (size_t i = 0; i < nrMsgs; ++i) {
      if (strcmp_P(buffer, msgs[i]) == 0) {
        return i;
      }
    }
  }
  return -1;         // This indicates: timed out
}

/* This function was taken from GPRSbee library of Kees Bakker
 * \brief Wait for a prompt, or timeout
 *
 * \return true if succeeded (the reply received), false if otherwise (timed out)
 */
bool Ubidots::waitForPrompt(const char *prompt, uint32_t ts_max)
{
  const char * ptr = prompt;

  while (*ptr != '\0') {
    wdt_reset();
    if (isTimedOut(ts_max)) {
      break;
    }

    int c = Serial1.read();
    if (c < 0) {
      continue;
    }

    SerialUSB.print((char)c);
    switch (c) {
    case '\r':
      // Ignore
      break;
    case '\n':
      // Start all over
      ptr = prompt;
      break;
    default:
      if (*ptr == c) {
        ptr++;
      } else {
        // Start all over
        ptr = prompt;
      }
      break;
    }
  }

  return true;
}
