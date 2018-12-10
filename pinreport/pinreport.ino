// vim: set rnu sw=4 ss=4 ts=4 et ai fdm=indent :
//
#define WIFI                1

#include <Wemospin.h>
#define PIN_BAT           WEMOS_PIN_D2
#define PIN_KIT           WEMOS_PIN_D1
#define PIN_LIV           WEMOS_PIN_D6
#define PIN_CAT           WEMOS_PIN_D7
#define PIN_BED           WEMOS_PIN_D5
#define PIN_LIB           WEMOS_PIN_D0



#include "Config.h"                          // password
#include <math.h>
#include <Thread.h>   
#include <ThreadController.h>   
 
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <TelegramBot.h>

WiFiClientSecure net_ssl; 
TelegramBot bot(BOT_TOKEN, net_ssl); 

#if(WIFI==1)
#include <aREST.h>
aREST rest = aREST();
#define LISTEN_PORT 80
WiFiServer server(LISTEN_PORT);
#endif

const char* ssid      = WIFI_SSID;              // your ssid
const char* password  = WIFI_PASSWORD;          // your password

class cRelay {
private:
    int pin;
    String status;
    void ctrlpin(int c) {
        ctrl = c;
        if(ctrl == 1) {
            pinMode(pin,   OUTPUT);
            digitalWrite(pin, LOW);               // Relay ON 
        } else {
            digitalWrite(pin, HIGH);               // Relay OFF
            pinMode(pin, INPUT);
        }
    }
public:
    int timer;
    int ctrl;
    cRelay(int _pin) {
        pin = _pin;
        timer = 0;
        ctrlpin(0);
    }
    void dectimer() {
        if(0 > timer) {
            ctrlpin(0);
        } else {
            timer = timer -1; 
        }
    }
    void settimer(int t) {
        if(t == 0) {
            ctrlpin(0);
        } else {
            ctrlpin(1);
        }
        timer = t;
    }
};



void Init ( ) {
    pinMode(PIN_BAT, INPUT);
    pinMode(PIN_KIT, INPUT);
    pinMode(PIN_LIV, INPUT);
    pinMode(PIN_CAT, INPUT);
    pinMode(PIN_BED, INPUT);
    pinMode(PIN_LIB, INPUT);
    Serial.begin(9600);
}


void setup ( ) {
    Init();
#if(WIFI == 1)
    Serial.print("\n***** WIFI *****");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    server.begin();
    Serial.print("\nWifi Connected\nIP: ");
    Serial.print(WiFi.localIP());
#endif
    delay(1000);

}


void loop ( ) {
    int pinBat;
    int pinKit;
    int pinLiv;
    int pinCat;
    int pinBed;
    int pinLib;
	int pins;

    pinBat = digitalRead(PIN_BAT);
    pinKit = digitalRead(PIN_KIT);
    pinLiv = digitalRead(PIN_LIV);
    pinCat = digitalRead(PIN_CAT);
    pinBed = digitalRead(PIN_BED);
    pinLib = digitalRead(PIN_LIB);


	pins = 0;
	pins  = pins | pinBat << 5;
	pins  = pins | pinKit << 4;
	pins  = pins | pinLiv << 3;
	pins  = pins | pinCat << 2;
	pins  = pins | pinBed << 1;
	pins  = pins | pinLib << 0;

    Serial.print("\n* pinBat= ");
    Serial.print(pinBat);
    Serial.print("\n* pinKit= ");
    Serial.print(pinKit);
    Serial.print("\n* pinLiv= ");
    Serial.print(pinLiv);
    Serial.print("\n* pinCat= ");
    Serial.print(pinCat);
    Serial.print("\n* pinBed= ");
    Serial.print(pinBed);
    Serial.print("\n* pinLib= ");
    Serial.print(pinLib);
    Serial.print("\n ----\n");
    Serial.print(String(pins, HEX));
    Serial.print("\n ----\n");
    delay(5000);

    bot.sendMessage(ROOM_ID, "0x" + String(pins, HEX));   
}

