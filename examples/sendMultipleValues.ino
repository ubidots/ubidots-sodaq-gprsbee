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
    float value0 = analogRead(A0);
    float value1 = analogRead(A1);
    float value2 = analogRead(A2);
    float value3 = analogRead(A3);
    Client.add("Variable_Name_0",value0);
    Client.add("Variable_Name_1",value1);
    Client.add("Variable_Name_2",value2);
    Client.add("Variable_Name_3",value3);
    Client.sendAll();
    delay(1000);
}
