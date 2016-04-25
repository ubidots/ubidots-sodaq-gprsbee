# Ubidots SODAQ Autonomo with GPRSbee


This library is to connect easily an mDot with SODAQ Autonomo

## Requiremets

* [SODAQ Autonomo](http://shop.sodaq.com/nl/arduino-boards/)
* [GPRSbee](http://gprsbee.com/)
* [Arduino IDE 1.6.7 or higher](https://www.arduino.cc/en/Main/Software)

## Setup

1. Download Arduino IDE [here](https://www.arduino.cc/en/Main/Software)
2. Go to Arduino IDE **File -> Preferences -> Additional Board Manager URLs** and add next line there "http://downloads.sodaq.net/package_sodaq_index.json".
3. Open Board Manager via **Tools -> Board -> Board Manager** and search for SODAQ boards, select SODAQ SAMD boards for the Autonomo.
4. Download the GPRSbee library of Ubidots [here](https://github.com/ubidots/ubidots-sodaq-gprsbee)
5. Go to the Arduino IDE, click on **Sketch -> Include Library -> Add .ZIP Library**
6. Select the .ZIP file of GPRSbee and then "Accept" or "Choose"
5. Close the Arduino IDE and open it again.

    
## Send one value to Ubidots

To send one value from mDot to your gateway copy the next code:

```cpp
#include <GPRSbee.h>

// Put here your SSID of lora gateway
#define APN "Your_Lora_Gateway_SSID"
// Put here your PASS of lora gateway
#define USER_APN "Your_Lora_Gateway_PASS"
// Put here your frequenci sub-band of lora gateway
#define PASS_APN "1"

Ubidots loraClient;


void setup()
{
    // The code will not start unless the serial monitor is opened or 10 sec is passed
    // incase you want to operate Autonomo with external power source
    while ((!SerialUSB) && (millis() < 10000))
        ;
    
    SerialUSB.begin(115200);
    SerialUSB.println("Here we start !! ");

    Serial1.begin(115200);
    loraClient.setOnBee(BEE_VCC, BEEDTR, BEECTS);
    while(!loraClient.loraConnection(GATEWAY_SSID, GATEWAY_PASS, GATEWAY_SUB_BAND));
}

void loop() {
    float value = analogRead(A0);
    loraClient.loraSend(value);
    delay(1000);
}
```