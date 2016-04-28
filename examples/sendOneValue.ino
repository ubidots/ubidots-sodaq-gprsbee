#include <SODAQGPRSbee.h>

// Put here your SIM APN
#define APN "Your_APN"
// Put here your USER of SIM APN
#define USER "Your_USER_APN"
// Put here your PASSWORD of SIM APN
#define PASSWORD "Your_PASSWORD_APN"

Ubidots Client;

void setup()
{
    // The code will not start unless the serial monitor is opened or 10 sec is passed
    // incase you want to operate Autonomo with external power source
    while ((!SerialUSB) && (millis() < 10000))
        ;
    
    SerialUSB.begin(115200);

    Serial1.begin(115200);
    loraClient.setOnBee(BEE_VCC, BEEDTR, BEECTS);
    while(!Client.setApn(APN, USER, PASSWORD));
}

void loop() {
    float value = analogRead(A0);
    Client.add("Variable_Name",value);
    Client.sendAll();
    delay(1000);
}
