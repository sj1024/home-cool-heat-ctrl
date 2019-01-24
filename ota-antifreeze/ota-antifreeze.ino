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
#include <ESP8266WebServer.h>
#include <TelegramBot.h>
#include "Config.h"                          // password
#include "HEATER.h"

#define PIN_RELAY_HEATER    WEMOS_PIN_D6
#define PIN_LED             WEMOS_PIN_D4

const char* ssid      = WIFI_SSID;              // your ssid
const char* password  = WIFI_PASSWORD;          // your password


ThreadController thrContrl  = ThreadController();
void config_rest_server_routing(void);

/* <custom> */
#define HTTP_REST_PORT 80
ESP8266WebServer http_rest_server(HTTP_REST_PORT);

#include <DHT12.h>
#define INIT_DI_CTRL        999 
#define INIT_HEAT_CTRL      1 
#define PIN_RELAY_HEATER    WEMOS_PIN_D6
#define PIN_LED             WEMOS_PIN_D4
#define MACHINE_INTERVAL    (1000*60*5) // 5mins

Thread thrTimer             = Thread();
Thread thrMain              = Thread();
volatile int timer=0;
volatile int cnt=0;

DHT12 dht12;
volatile int callback_flag = 0;
WiFiClientSecure net_ssl; 
TelegramBot bot(BOT_TOKEN, net_ssl); 
void callback_active(void);
HEATER heater = HEATER(PIN_RELAY_HEATER, &dht12, &callback_active);

void settimer(int t);
void machine(void);
/* </custom> */

void Init ( ) {
    pinMode(PIN_LED, OUTPUT);
}

void thrfTimer() {
    

    if(0 >= timer) {
        heater.ctrlpin(0);
    } else {
        timer = timer -1; 
    }
    if((cnt % 5)==0) { // every 5 min
        machine();
    }
    cnt++;

    if(callback_flag == 1) {
        t_Climate_Def climate;
        climate = heater.getclimate();
        Serial.println("\n\rcallback");
        bot.sendMessage(ROOM_ID, "열선 동작(ºC): " + String(climate.temp));
        callback_flag =0;
    }
}


void setup ( ) {
    Init();
    Serial.begin(9600);
    Serial.print("\n\r***** WIFI *****");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(PIN_LED, LOW); //ON
        delay(50);
        digitalWrite(PIN_LED, HIGH); //OFF
        delay(50);
        Serial.print(".");
    }
    Serial.print("\n\rWifi Connected\n\rIP: ");
    Serial.print(WiFi.localIP());

    ArduinoOTA.onStart([]() {
        Serial.println("\n\r\n\r>>> Programming Started");
    });
    
    ArduinoOTA.onEnd([]() {
      Serial.println("\n\r<<< End");
      ESP.reset();
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

    bot.begin();

    thrTimer.enabled = true;
    thrTimer.setInterval(60000); // 1min
    thrTimer.onRun(thrfTimer);
    thrContrl.add(&thrTimer);

    config_rest_server_routing();
    http_rest_server.begin();
    Serial.println("\n\rHTTP REST Server Started");
}
void loop ( ) {
    http_rest_server.handleClient();
    ArduinoOTA.handle();
    thrContrl.run();
}

/* ----------------------------------------------------------------------- */

void callback_active() {
    callback_flag =1;
}

void report(char* jsonmsgbuffer, int size) {
    StaticJsonBuffer<500> jsonBuffer;
    t_Climate_Def climate;
    climate = heater.getclimate();

    JsonObject& variables = jsonBuffer.createObject();
    variables.set("Ctrl",  heater.ctrl);
    variables.set("Timer", timer);
    variables.set("Humi",  climate.humi);
    variables.set("Temp",  climate.temp);

    JsonObject& jsonObj = jsonBuffer.createObject();
    JsonArray& variable = jsonObj.createNestedArray("variables");
    variable.add(variables);

    jsonObj.prettyPrintTo(jsonmsgbuffer, 200);
    return;
}

void get() {
    char JSONmessageBuffer[200];

    //if (led_resource.id == 0)
        //http_rest_server.send(204);
    //else {
        report(JSONmessageBuffer, 200);
        http_rest_server.send(200, "application/json", JSONmessageBuffer);
    //}
}

void json_to_resource(JsonObject& jsonBody) {
    String ctrl= jsonBody["ctrl"];
    Serial.print("\n\r* Ctrl Code: ");
    Serial.print(ctrl);
    String t = ctrl.substring(1, 4);
    int timer = t.toInt();
    settimer(timer);
}
void post_put() {
    StaticJsonBuffer<200> jsonBuffer;
    char JSONmessageBuffer[200];
    String post_body = http_rest_server.arg("plain");
    Serial.println(post_body);

    //decoding
    JsonObject& jsonBody = jsonBuffer.parseObject(http_rest_server.arg("plain"));


    Serial.print("\n\rHTTP Method: ");
    Serial.println(http_rest_server.method());
    
    if (!jsonBody.success()) {
        Serial.println("error in parsin json body");
        http_rest_server.send(400);
    }
    else {   
        if (http_rest_server.method() == HTTP_POST) {
            json_to_resource(jsonBody);
            report(JSONmessageBuffer, 200);
            http_rest_server.send(201, "application/json", JSONmessageBuffer);
        }
        else if (http_rest_server.method() == HTTP_PUT) {
            json_to_resource(jsonBody);
            report(JSONmessageBuffer, 200);
            http_rest_server.send(200, "application/json", JSONmessageBuffer);
        }
    }
}

void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, get);
    http_rest_server.on("/", HTTP_POST, post_put);
    http_rest_server.on("/", HTTP_PUT, post_put);
}


void machine() {
    t_Climate_Def climate;
    climate = heater.getclimate();
    if(climate.temp < (float)1) {
        settimer(3); // 3 mins on
    }
}

void settimer(int t) {
    if(0 >= t) {
        heater.ctrlpin(0);
    } else {
        heater.ctrlpin(1);
    }
    timer = t;
}

