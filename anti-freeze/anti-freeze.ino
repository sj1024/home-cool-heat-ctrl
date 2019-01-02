// vim: set rnu sw=4 ss=4 ts=4 extendtab smartindent autoindent fdm=indent :
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
 
#define WIFI                1
#define LOG_HEAT            1
#define LOG_COOL            0
#define LOG_CLIMATE         1

#include <Wemospin.h>
#define INIT_DI_CTRL        999
#define INIT_HEAT_CTRL      1 
#define PIN_RELAY_HEATER    WEMOS_PIN_D6
#define PIN_LED             WEMOS_PIN_D4
#define MACHINE_INTERVAL    (1000*60*5) // 5mins

#include "Config.h"                          // password
#include <math.h>
#include <DHT12.h>
#include <Wire.h>                               // DHT12 uses I2C (GPIO4 = SDA, GPIO5 = SCL)
#include <Thread.h>   
#include <ThreadController.h>   
DHT12 dht12;                                    // Default Unit = CELSIUS, ID = 0x5c
 
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <TelegramBot.h>

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
        if(0 >= timer) {
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

WiFiClientSecure net_ssl; 
TelegramBot bot(BOT_TOKEN, net_ssl); 
cRelay relay = cRelay(PIN_RELAY_HEATER);

#if(WIFI==1)
#include <aREST.h>
aREST rest = aREST();
#define LISTEN_PORT 80
WiFiServer server(LISTEN_PORT);
#endif

const char* ssid      = WIFI_SSID;              // your ssid
const char* password  = WIFI_PASSWORD;          // your password

typedef struct {
    float temp          =0;
    float humi          =0;
    float di            =0;
    float wet_temp      =0;
    float dew_point     =0;
    float absolute_humi =0;
} t_Climate_Def; 

typedef struct {
    int trig_score_cool =0;
    int trig_score_heat =0;
    int di              =INIT_DI_CTRL;
    int heat            =INIT_HEAT_CTRL;
    int timer           =0;
    String status_cool  ="RELAY_OFF";
    String status_heat  ="RELAY_OFF";
} t_System_Ctrl_Def;

Thread thrGetTemp           = Thread();
#if(WIFI == 1)
Thread thrRest              = Thread();
#endif
Thread thrTimer             = Thread();
ThreadController thrContrl  = ThreadController();

t_Climate_Def       t_Climate;
t_System_Ctrl_Def   t_SysCtrl;

void Init ( ) {
    t_Climate.temp              =0;
    t_Climate.humi              =0;
    t_Climate.di                =0;
    t_Climate.wet_temp          =0;
    t_Climate.dew_point         =0;
    t_Climate.absolute_humi     =0;
    t_SysCtrl.trig_score_cool   =0;
    t_SysCtrl.trig_score_heat   =0;
    t_SysCtrl.di                =INIT_DI_CTRL;
    t_SysCtrl.heat              =INIT_HEAT_CTRL;
    t_SysCtrl.timer             =0;
    t_SysCtrl.status_cool       ="RELAY_OFF";
    t_SysCtrl.status_heat       ="RELAY_OFF";
    pinMode(PIN_LED, OUTPUT);
    bot.begin();

}

void setup ( ) {
    Init();
    Serial.begin(9600);
#if(WIFI == 1)
    Serial.print("\n***** WIFI *****");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(PIN_LED, LOW); //ON
        delay(250);
        digitalWrite(PIN_LED, HIGH); //OFF
        delay(250);
        Serial.print(".");
    }
    server.begin();
    Serial.print("\nWifi Connected\nIP: ");
    Serial.print(WiFi.localIP());
#endif
    Serial.print("\n***** DHT12 *****");
    delay(1000);

    Wire.begin();
    delay(1000);

#if(WIFI == 1)
    rest.variable("Temp",       &t_Climate.temp);
    rest.variable("Humi",       &t_Climate.humi);
    rest.variable("Timer",      &relay.timer);
    rest.variable("Ctrl",       &relay.ctrl);
    rest.function("relayctrl",   RelayCtrl);

    thrRest.enabled         = true;
    thrRest.setInterval(100);
    thrRest.onRun(thrfRest);
    thrContrl.add(&thrRest);
#endif
    thrGetTemp.enabled = true;
    thrGetTemp.setInterval(10);
    thrGetTemp.onRun(thrfTemp);
    thrContrl.add(&thrGetTemp);

    thrTimer.enabled   = true;
    thrTimer.setInterval(60000); // 1min
    thrTimer.onRun(thrfTimer);
    thrContrl.add(&thrTimer);
}
void loop ( ) {
    thrContrl.run();
}

void MachineHeat ( ) {
    if(t_Climate.temp < (float)t_SysCtrl.heat) {
        relay.settimer(3); // 3mins on
#if(WIFI ==1)
        bot.sendMessage(ROOM_ID, "보일러실동파방지 동작!");  // Reply to the same chat with the same text
#endif
#if(LOG_HEAT == 1)
        Serial.print("\nRELAY_ON");
#endif
    }
}
void GetClimate ( t_Climate_Def *climate ) {
    float temp = dht12.readTemperature();
    float humi = dht12.readHumidity();
    //float temp = 100;
    //float humi = 100;

    float ah = 2165 * humi * 0.01 * 0.6108 * exp(17.27 * temp / (temp + 237.3)) / (temp + 273.16);
    float wtemp = temp * atan(0.151977 * pow(humi + 8.313659, 0.5)) + atan(temp + humi) 
                  - atan(humi - 1.676331) + 0.003918 * pow(humi, 1.5) * atan(0.023101 * humi) - 4.686035;
    float dew_point = temp - ((14.55 + 0.114 * temp) * (1. - 0.01 * humi) 
                  + pow((2.5 + 0.007 * temp) * (1. - 0.01 * humi), 3) + (15.9 + 0.117 * temp) * pow((1. - 0.01 * humi), 14));
    float di = 0.72 * (temp + wtemp) + 40.6;
    climate->temp = temp;
    climate->humi = humi;
    climate->wet_temp = wtemp;
    climate->absolute_humi = ah;
    climate->dew_point = dew_point;
    climate->di = di;
    return;
}
void thrfTemp ( ) {
    static int cnt;
	if(cnt == 0) {
        GetClimate(&t_Climate);
        MachineHeat();
#if(LOG_CLIMATE==1)      
        Serial.print("\n* Temperature (℃) = ");
        Serial.print(String(t_Climate.temp, 1));
        Serial.print("\n* Relative Humidity (%) = ");
        Serial.print(String(t_Climate.humi, 1));
        Serial.print("\n* Absolute Humidity (%) = ");
        Serial.print(String(t_Climate.absolute_humi, 1));
        Serial.print("\n* Wet Bulb Temperature(℃) = ");
        Serial.print(String(t_Climate.wet_temp, 1));
        Serial.print("\n* Due Point Temperature(℃) = ");
        Serial.print(String(t_Climate.dew_point, 1));
        Serial.print("\n* Discomport Index = ");
        Serial.print(String(t_Climate.di , 1));
        Serial.print("\n--");
#endif
        Serial.print("\nTick!!");
	}
	if(++cnt >= (MACHINE_INTERVAL/10)) { // as per 10 ms
	    cnt =0;
    }
}
void thrfTimer ( ) {
    relay.dectimer();
}
void thrfRest ( ) {
#if(WIFI == 1)
    WiFiClient client = server.available();
    if(!client) {
        return;
    }
    while(!client.available()) {
        delay(1);
    }
    rest.handle(client);
#endif
}
int RelayCtrl( String ctrl ) {
    String r = ctrl.substring(0, 1);
    String t = ctrl.substring(1, 4);
    int timer = t.toInt();
    relay.settimer(timer);
    return relay.timer;
}
