#include <MyConfig.h>
#include <Wemospin.h>
#include <Wemospin.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Wire.h>                               // DHT12 uses I2C (GPIO4 = SDA, GPIO5 = SCL)
#include <DHT12.h>  
#include "RELAY.h"

#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

#define HOSTNAME "boil.heat"

#define PIN_RELAY           WEMOS_PIN_D5
#define PIN_LED             WEMOS_PIN_D4

#define TIMER_INTERVAL      1000

struct Led {
    byte id;
    byte gpio;
    byte status;
} led_resource;

const char* wifi_ssid = WIFI_SSID;
const char* wifi_passwd = WIFI_PASSWORD;

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

DHT12 dht12;
volatile int timer=0;
volatile int di=0;
volatile int callback_flag = 0;
volatile int ctrl = 0;
void callback_active(void);
RELAY relay = RELAY(PIN_RELAY, &callback_active);

void callback_active() {
    callback_flag =1;
}


int init_wifi() {
    int retries = 0;

    Serial.println("Connecting to WiFi AP..........");

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    return WiFi.status(); // return the WiFi connection status
}


void json_to_resource(JsonObject& jsonBody) {
    String ctrl= jsonBody["ctrl"];
    Serial.print("\n\r%% Ctrl Code: ");
    Serial.print(ctrl);
}

void report(char* jsonmsgbuffer, int size) {

    StaticJsonBuffer<500> jsonBuffer;
    t_Climate_Def climate;
    dht12.readClimate(&climate);

    JsonObject& variables = jsonBuffer.createObject();
    variables.set("Hostname",  HOSTNAME);
    variables.set("Ctrl",  ctrl);
    variables.set("Timer", timer);
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

void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, get);
    http_rest_server.on("/", HTTP_POST, post_put);
    http_rest_server.on("/", HTTP_PUT, post_put);
}

void post_put() {
    StaticJsonBuffer<200> jsonBuffer;
    char JSONmessageBuffer[200];
    String post_body = http_rest_server.arg("plain");
    Serial.print("\n\rPost Body: ");
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


void setup(void) {

    Serial.begin(9600);
    Wire.begin();

    if (init_wifi() == WL_CONNECTED) {
        Serial.print("Connetted to ");
        Serial.print(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
    }

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

    config_rest_server_routing();

    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");

}

void machine() {

    t_Climate_Def climate;
    dht12.readClimate(&climate);
    Serial.print("\n\r%%%%%%%%%%%%%%%%");
    Serial.print("\n\rTemp: ");
    Serial.print(String(climate.temp));

    if(ctrl == 1) {
        relay.ctrlpin(0); // off
		ctrl = 0;
        Serial.print("\n\rRelay Off: Overheeat");
	} else if(climate.temp < 1) { // freezing ?
        relay.ctrlpin(1); // on
		ctrl = 1;
        Serial.print("\n\rRelay On: freezing ");
    } else {
		ctrl = 0;
        relay.ctrlpin(0); // off
        Serial.print("\n\rRelay Off: Warm");
    }

}

void loop(void) {

    static int cnt = 0;
    ArduinoOTA.handle();
    http_rest_server.handleClient();
    if(0 >= timer) {
        relay.ctrlpin(0); // off
    } else {
        timer--;
    }
    if((cnt++ % (5*60)) == 0) { //every 5 time
        machine();
    }

    if(callback_flag == 1) {
        callback_flag =0;
    }
    delay(TIMER_INTERVAL);

}
