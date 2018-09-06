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
 
#define WIFI                1
#define LOG_HEAT            0
#define LOG_COOL            0
#define LOG_CLIMATE         0

#include <Wemospin.h>
#define INIT_DI_CTRL        999
#define INIT_HEAT_CTRL      -999
#define MIN_THRESHOLD       5                   // Min
#define PIN_RELAY_COOLER    WEMOS_PIN_D5
#define PIN_RELAY_HEATER    WEMOS_PIN_D6
#define PIN_LED             WEMOS_PIN_D4

#include <Config.h>                          // password
#include <math.h>
#include <DHT12.h>
#include <Wire.h>                               // DHT12 uses I2C (GPIO4 = SDA, GPIO5 = SCL)
#include <Thread.h>   
#include <ThreadController.h>   
DHT12 dht12;                                    // Default Unit = CELSIUS, ID = 0x5c
 
#include <ESP8266WiFi.h>

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
Thread thrSetStatus         = Thread();
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
    RelayCtrl(PIN_RELAY_COOLER, LOW); 
    RelayCtrl(PIN_RELAY_HEATER, LOW); 
    pinMode(PIN_LED, OUTPUT);
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
    rest.variable("di_ctrl",        &t_SysCtrl.di);
    rest.variable("heat_ctrl",      &t_SysCtrl.heat);
    rest.variable("timer_ctrl",          &t_SysCtrl.timer);
    rest.variable("DI",         &t_Climate.di);
    rest.variable("Temp",       &t_Climate.temp);
    //rest.variable("Wet Temp",   &t_Climate.wet_temp);
    rest.variable("Humi",       &t_Climate.humi);
    rest.variable("Status Cool",     &t_SysCtrl.status_cool);
    rest.variable("Status Heat",     &t_SysCtrl.status_heat);
    //rest.variable("Trig Cool",     &t_SysCtrl.trig_score_cool);
    //rest.variable("Trig Heat",     &t_SysCtrl.trig_score_heat);

    rest.function("di_ctrl",     DiCtrl);
    rest.function("timer_ctrl",  TimerCtrl);
    rest.function("heat_ctrl",   HeatCtrl);
    rest.function("cool_on", CoolOn);
    rest.function("heat_on", HeatOn);

#endif
    thrGetTemp.enabled = true;
    thrGetTemp.setInterval(1000);
    thrGetTemp.onRun(thrfTemp);

    thrSetStatus.enabled    = true;
    thrSetStatus.setInterval(100);
    thrSetStatus.onRun(thrfSetStatus);

#if(WIFI == 1)
    thrRest.enabled         = true;
    thrRest.setInterval(100);
    thrRest.onRun(thrfRest);
#endif

    thrTimer.enabled         = true;
    //thrTimer.setInterval(1000); // 1sec
    thrTimer.setInterval(1000*60); // 1min
    thrTimer.onRun(thrfTimer);

    thrContrl.add(&thrTimer);
    thrContrl.add(&thrGetTemp);
    thrContrl.add(&thrSetStatus);
#if(WIFI == 1)
    thrContrl.add(&thrRest);
#endif
}
void loop ( ) {
    thrContrl.run();
}
void RelayCtrl (int pin, int ctrl ) {
    if(ctrl == HIGH) {
        pinMode(pin,   OUTPUT);
        digitalWrite(pin, LOW);               // Relay ON 
    } else {
        digitalWrite(pin, HIGH);               // Relay OFF
        pinMode(pin, INPUT);
    }
}
void MachineCool ( ) {
#if(LOG_COOL == 1)
    Serial.print("\nt_SysCtrl.status_cool = ");
    Serial.print(t_SysCtrl.status_cool);
#endif

    if(t_SysCtrl.status_cool == "RELAY_OFF") {
        if(t_Climate.di > (float)t_SysCtrl.di) {
            t_SysCtrl.status_cool = "TRIGG_ON";
        }
    } else if(t_SysCtrl.status_cool == "RELAY_ON") {
        if(t_Climate.di < (float)t_SysCtrl.di) {
            t_SysCtrl.status_cool = "TRIGG_OFF";
        }
    } else if(t_SysCtrl.status_cool == "TRIGG_ON") {
        if(t_Climate.di > (float)t_SysCtrl.di) {
            t_SysCtrl.trig_score_cool++;
        } else {
            t_SysCtrl.trig_score_cool =0;
            t_SysCtrl.status_cool ="RELAY_OFF";
        }
        if(t_SysCtrl.trig_score_cool >= (MIN_THRESHOLD*60)) {
            // ON
            RelayCtrl(PIN_RELAY_COOLER, HIGH);
            t_SysCtrl.trig_score_cool =0;
            t_SysCtrl.status_cool ="RELAY_ON";
        }
    } else if(t_SysCtrl.status_cool == "TRIGG_OFF") {
        if(t_Climate.di < (float)t_SysCtrl.di) {
            t_SysCtrl.trig_score_cool++;
        } else {
            t_SysCtrl.trig_score_cool = 0;
            t_SysCtrl.status_cool = "RELAY_ON";
        }
        if(t_SysCtrl.trig_score_cool >= (MIN_THRESHOLD*60)) {
            // OFF
            RelayCtrl(PIN_RELAY_COOLER, LOW);
            t_SysCtrl.trig_score_cool =0;
            t_SysCtrl.status_cool ="RELAY_OFF";
        }
    }
#if(LOG_COOL == 1)
    Serial.print("\nTran_Score = ");
    Serial.print(String(t_SysCtrl.trig_score_cool));
    Serial.print("\nDI Tigger = ");
    Serial.print(String(t_SysCtrl.di));
#endif
}
void MachineHeat ( ) {
#if(LOG_HEAT == 1)
    Serial.print("\nt_SysCtrl.status_heat = ");
    Serial.print(t_SysCtrl.status_heat);
#endif

    if(t_SysCtrl.status_heat == "RELAY_OFF") {
        if(t_Climate.temp < (float)t_SysCtrl.heat) {
            t_SysCtrl.status_heat = "TRIGG_ON";
        }
    } else if(t_SysCtrl.status_heat == "RELAY_ON") {
        if(t_Climate.temp > (float)t_SysCtrl.heat) {
            t_SysCtrl.status_heat = "TRIGG_OFF";
        }
    } else if(t_SysCtrl.status_heat == "TRIGG_ON") {
        if(t_Climate.temp < (float)t_SysCtrl.heat) {
            t_SysCtrl.trig_score_heat++;
        } else {
            t_SysCtrl.trig_score_heat =0;
            t_SysCtrl.status_heat ="RELAY_OFF";
        }
        if(t_SysCtrl.trig_score_heat >= (MIN_THRESHOLD*60)) {
            // ON
            RelayCtrl(PIN_RELAY_HEATER, HIGH);
            t_SysCtrl.trig_score_heat =0;
            t_SysCtrl.status_heat ="RELAY_ON";
        }
    } else if(t_SysCtrl.status_heat == "TRIGG_OFF") {
        if(t_Climate.temp > (float)t_SysCtrl.heat) {
            t_SysCtrl.trig_score_heat++;
        } else {
            t_SysCtrl.trig_score_heat = 0;
            t_SysCtrl.status_heat = "RELAY_ON";
        }
        if(t_SysCtrl.trig_score_heat >= (MIN_THRESHOLD*60)) {
            // OFF
            RelayCtrl(PIN_RELAY_HEATER, LOW);
            t_SysCtrl.trig_score_heat =0;
            t_SysCtrl.status_heat ="RELAY_OFF";
        }
    }
#if(LOG_HEAT == 1)
    Serial.print("\nTran_Score = ");
    Serial.print(String(t_SysCtrl.trig_score_heat));
    Serial.print("\nHeat Trigger = ");
    Serial.print(String(t_SysCtrl.heat));
#endif
}
void thrfSetStatus  () {
    static int count;
    if(t_SysCtrl.status_cool == "RELAY_ON") {
        if(count > 63) {
            count = 0;
            digitalWrite(PIN_LED, LOW); //ON
        } else if(count > 62) {
            digitalWrite(PIN_LED, HIGH); //OFF
        } else if(count > 61) {
            digitalWrite(PIN_LED, LOW); //ON
        } else if(count > 60) {
            digitalWrite(PIN_LED, HIGH); //OFF
        }
    }else if(t_SysCtrl.status_cool == "RELAY_OFF") {
        if(count > 63) {
            count = 0;
            digitalWrite(PIN_LED, HIGH); //OFF
        } else if(count > 62) {
            digitalWrite(PIN_LED, LOW); //ON
        } else if(count > 61) {
            digitalWrite(PIN_LED, HIGH); //OFF
        } else if(count > 60) {
            digitalWrite(PIN_LED, LOW); //ON
        }
    }else if(t_SysCtrl.status_cool == "TRIGG_OFF") {
        if(count > 10) {
            if(count > 11) count = 0;
            digitalWrite(PIN_LED, HIGH); //Short OFF 
        }else {
            digitalWrite(PIN_LED, LOW); //Long ON
        }
    }else {
        if(count > 10) {
            if(count > 11) count = 0;
            digitalWrite(PIN_LED, LOW); //Short ON
        }else {
            digitalWrite(PIN_LED, HIGH); //Long OFF
        }
    }
    count++;
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
    GetClimate(&t_Climate);
    MachineCool();
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
}
void thrfTimer ( ) {
    t_SysCtrl.timer = t_SysCtrl.timer -1;
    if(0 > t_SysCtrl.timer) {
        Init();
    }
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
int TimerCtrl( String t ) {
    t_SysCtrl.timer= t.toInt();
    return t_SysCtrl.heat;
}
int HeatCtrl ( String h ) {
    t_SysCtrl.heat= h.toInt();
    return t_SysCtrl.heat;
}
int DiCtrl( String d ) {
    t_SysCtrl.di= d.toInt();
    return t_SysCtrl.di;
}
int CoolOn ( String ctrl ) {
    String on = ctrl.substring(0, 1);
    if(on.toInt() == 1) {
        String timer = ctrl.substring(1, 4);
        String di = ctrl.substring(4, 7);
        t_SysCtrl.timer = timer.toInt();
        t_SysCtrl.di = di.toInt();
        t_SysCtrl.status_cool = "RELAY_ON";
        RelayCtrl(PIN_RELAY_COOLER, HIGH); 
        t_SysCtrl.trig_score_cool = 0;
    } else {
        Init();
    }
    return t_SysCtrl.di;
}
int HeatOn ( String ctrl ) {
    String on = ctrl.substring(0, 1);
    if(on.toInt() == 1) {
        String timer = ctrl.substring(1, 4);
        String heat = ctrl.substring(4, 7);
        t_SysCtrl.timer = timer.toInt();
        t_SysCtrl.heat = heat.toInt();
        t_SysCtrl.status_heat= "RELAY_ON";
        RelayCtrl(PIN_RELAY_HEATER, HIGH); 
        t_SysCtrl.trig_score_heat= 0;
    } else {
        Init();
    }
    return t_SysCtrl.di;
}

