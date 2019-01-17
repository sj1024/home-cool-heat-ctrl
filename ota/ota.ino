// vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
//
//  ESP8266_DHT12_WiFi_WeMos.ino (2017.8.24) --> WeMos D1 Mini version
//
//  https://playground.arduino.cc/Main/Dht
//  DHT12 Library --> https://github.com/Bobadas/DHT12_library_Arduino
//
//  Absolute Humidity --> http://biomet.ucdavis.edu/conversions/HumCon.pdf
//  Wet-bulb Temperature --> http://journals.ametsoc.org/doi/pdf/10.1175/JAMC-D-11-0143.1
//  Dew-point Temperature --> http://wahiduddin.net/calc/density_algorithms.htm
//
 
#include <Wemospin.h>
#include "Config.h"                          // password
#include <math.h>
#include <Wire.h>                               // DHT12 uses I2C (GPIO4 = SDA, GPIO5 = SCL)
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <aREST.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Thread.h>   
#include <ThreadController.h>   

#define PIN_RELAY_HEATER    WEMOS_PIN_D6
#define PIN_LED             WEMOS_PIN_D4

const char* ssid      = WIFI_SSID;              // your ssid
const char* password  = WIFI_PASSWORD;          // your password

ThreadController thrContrl  = ThreadController();
Thread thrOTA              = Thread();

void Init ( ) {
    pinMode(PIN_LED, OUTPUT);

}



void setup ( ) {
    Init();
    Serial.begin(9600);
    Serial.print("\n***** WIFI *****");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(PIN_LED, LOW); //ON
        delay(50);
        digitalWrite(PIN_LED, HIGH); //OFF
        delay(50);
        Serial.print(".");
    }
    Serial.print("\nWifi Connected\nIP: ");
    Serial.print(WiFi.localIP());

    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    thrOTA.enabled = true;
    thrOTA.setInterval(400);
    thrOTA.onRun(thrfOTA);
    thrContrl.add(&thrOTA);
}
void loop ( ) {
    thrContrl.run();
}

void thrfOTA() {
    ArduinoOTA.handle();
}
