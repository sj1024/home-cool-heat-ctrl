/*
vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
Name:        inlineKeyboard.ino
Created:     29/05/2018
Author:      Stefano Ledda <shurillu@tiscalinet.it>
Description: a simple example that do:
             1) if a "show keyboard" text message is received, show the inline custom keyboard,
                otherwise reply the sender with "Try 'show keyboard'" message
             2) if "LIGHT ON" inline keyboard button is pressed turn on the onboard LED and show an alert message
             3) if "LIGHT OFF" inline keyboard button is pressed, turn off the onboard LED and show a popup message
             4) if "see docs" inline keyboard button is pressed,
                open a browser window with URL "https://github.com/shurillu/CTBot"
*/
#include <MyConfig.h>
#include <CTBot.h>
#include <Wire.h>                               // DHT12 uses I2C (GPIO4 = SDA, GPIO5 = SCL)
#include <DHT12.h>
#include <Wemospin.h>
#include <Thread.h>
#include <ThreadController.h>
#include <ArduinoOTA.h>
#include "RELAY.h"
#include <Regexp.h>


//#define DEVICE DEVICE_AC_LIBRARY
#define DEVICE DEVICE_AC_BEDROOM
//#define DEVICE DEVICE_HEATWIRE
//#define DEVICE DEVICE_WIREBULB


#define OFF_CALLBACK "RelayOff" // callback data sent when "LIGHT OFF" button is pressed
#define STATUS_CALLBACK "Status" // callback data sent when "LIGHT OFF" button is pressed

#define PIN_RELAY           WEMOS_PIN_D5
#define PIN_LED             WEMOS_PIN_D4
#define HEAT_TEMP           1

CTBot myBot;
CTBotInlineKeyboard CmdKbd;  // custom inline keyboard object helper

String ssid = WIFI_SSID;     // REPLACE mySSID WITH YOUR WIFI SSID
String pass = WIFI_PASSWORD; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
#if(DEVICE==DEVICE_AC_LIBRARY)
String token = AC_LIBRARY_BOT_TOKEN;
#elif(DEVICE==DEVICE_AC_BEDROOM)
String token = AC_BEDROOM_BOT_TOKEN;
#elif(DEVICE==DEVICE_WIREBULB)
String token = WIREBULB_BOT_TOKEN;
#else
String token = HEATWIRE_BOT_TOKEN;
#endif

long timer = 0;
float di = 77;
volatile int ctrl = 0;

DHT12 dht12;
RELAY relay = RELAY(PIN_RELAY);

Thread thrTelegram = Thread();
Thread thrTimer    = Thread();
ThreadController thrContrl  = ThreadController();

unsigned long oldmillis = 0;
unsigned long get_timeleft() {
    unsigned long m = millis();
    timer = timer - (m - oldmillis);
    oldmillis = m;
    if(timer <= 0)
        timer = 0;
    return timer;
}
void machine() {
#if(DEVICE==DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY || DEVICE==DEVICE_HEATWIRE)
    t_Climate_Def climate;
    dht12.readClimate(&climate);
#endif
#if(DEVICE==DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY)
    if(climate.di>di && get_timeleft() > 0) {
        relay.ctrlpin(1); // on
    } else {
        relay.ctrlpin(0); // off
    }
#elif(DEVICE==DEVICE_HEATWIRE)
    if(ctrl == 1) {
        relay.ctrlpin(0); // off
        ctrl = 0;
        Serial.print("\n\rRelay Off: Overheeat");
    } else if(climate.temp < HEAT_TEMP ) { // freezing ?
        relay.ctrlpin(1); // on
        ctrl = 1;
        Serial.print("\n\rRelay On: freezing ");
    } else {
        ctrl = 0;
        relay.ctrlpin(0); // off
        Serial.print("\n\rRelay Off: Warm");
    }
#elif(DEVICE==DEVICE_WIREBULB)
    if(get_timeleft() > 0) {
        relay.ctrlpin(1); // on
    } else {
        relay.ctrlpin(0); // off
    }
#endif
}
void call_status (TBMessage msg) {
    String status;
#if(DEVICE==DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY || DEVICE==DEVICE_HEATWIRE)
    t_Climate_Def climate;
    dht12.readClimate(&climate);
#endif
#if(DEVICE==DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY || DEVICE==DEVICE_WIREBULB)
    double inte, frac;
    frac = modf((float)get_timeleft()/3600000, &inte);
#endif
#if(DEVICE==DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY)
    status =  String("Temp: ") + String(climate.temp);
    status += String("\nTime left: ") + String(int(inte)) + String(":") + String(int(frac*60));
    status += String("\nDi: ") + String(climate.di) + String("   \xF0\x9F\x93\x8C ") + String(di);
    status += String("\nA/C: ");
    if(climate.di>di && get_timeleft() > 0) {
        status  = status  + String("ON");
    } else {
        status  = status  + String("OFF");
    }
#endif
#if(DEVICE==DEVICE_HEATWIRE)
    status =  String("Temp: ") + String(climate.temp) ;
    status += String("\nHeat Wire: ");
    if(climate.temp <= HEAT_TEMP) {
        status  += String("ON");
    } else {
        status  += String("OFF");
    }
#endif
#if(DEVICE==DEVICE_WIREBULB)
    status = String("Time left: ") + String(int(inte)) + String(":") + String(int(frac*60));
    status += String("\nLight: ");
    if(get_timeleft() > 0) {
        status +=  String("ON");
    } else {
        status +=  String("OFF");
    }
#endif
    status  += String("\nAddr: ") + WiFi.localIP().toString();
    status  += String("\nYour id: ") + String(msg.sender.id);
    myBot.sendMessage(msg.sender.id,  status, CmdKbd);
}

int auth(TBMessage msg) {
    int i = sizeof(allowed_user)/sizeof(allowed_user[0]);
    int passage = 0;

    for(i--; i >= 0; i--) {
        if(msg.sender.id == allowed_user[i]) {
            passage= 1;
        }
    }
    if(passage == 0) {
        String text = String("\xF0\x9F\x9A\xAA You're not allowed");
        text += String("\nYour id: ") + String(msg.sender.id);
        myBot.sendMessage(msg.sender.id, text, CmdKbd);
    }
    return passage;
}

void set_di(TBMessage msg, int _di) {
     di = (float)_di;
     machine();
     call_status(msg);
}

void set_timer(TBMessage msg, long _timer) {
     timer = _timer * 3600000; // hour to mm
     machine();
     call_status(msg);
}

void TelegramMessageHandler() {
    // a variable to store telegram message data
    TBMessage msg;
    MatchState ms;
    char buf[60];

    // if there is an incoming message...
    if (myBot.getNewMessage(msg)) {
        if(auth(msg) == 1) {
           // check what kind of message I received
            if (msg.messageType == CTBotMessageText) {
               // received a text message
                myBot.sendMessage(msg.sender.id, "Getting Start", CmdKbd);
            } else if (msg.messageType == CTBotMessageQuery) {
                // received a callback query message
                msg.callbackQueryData.toCharArray(buf,60);
                ms.Target(buf);
                Serial.println(msg.callbackQueryData);
#if(DEVICE==DEVICE_AC_LIBRARY || DEVICE==DEVICE_AC_BEDROOM)
                if(REGEXP_MATCHED == ms.Match("(DI_)(%d+)", 0)) {
                    Serial.println("DI cmd");
                    String di = ms.GetCapture(buf, 1);
                    set_di(msg, di.toInt());
                } else if(REGEXP_MATCHED == ms.Match("(TIMER_)(%d+)H", 0)) {
                    Serial.println("Timer cmd");
                    String timer = ms.GetCapture(buf, 1);
                    set_timer(msg, timer.toInt());
                } else if (msg.callbackQueryData.equals(STATUS_CALLBACK)) {
                    Serial.println("Status cmd");
                    call_status(msg);
                } else if (msg.callbackQueryData.equals(OFF_CALLBACK)) {
                    Serial.println("Off cmd");
                    timer = 0;
                    relay.ctrlpin(0);
                    call_status(msg);
                }
#elif(DEVICE==DEVICE_WIREBULB)
                if(REGEXP_MATCHED == ms.Match("(TIMER_)(%d+)H", 0)) {
                    String timer = ms.GetCapture(buf, 1);
                    set_timer(msg, timer.toInt());
                } else if (msg.callbackQueryData.equals(STATUS_CALLBACK)) {
                    call_status(msg);
                } else if (msg.callbackQueryData.equals(OFF_CALLBACK)) {
                    timer = 0;
                    relay.ctrlpin(0);
                    call_status(msg);
                }
#elif(DEVICE==DEVICE_HEATWIRE)
                call_status(msg);
#endif
           }
        }
    }
}


void thrfTimer () {
    static int cnt = 0;
    if(0 >= get_timeleft()) {
        relay.ctrlpin(0);
    }
    if((cnt++ % 5) == 0) { //every 5 min
        machine();
    }
    Serial.println(String(get_timeleft()));
}

void thrfTelegram () {
    ArduinoOTA.handle();
    TelegramMessageHandler();
    Serial.print("\r\nTime left:");
    Serial.print(String(get_timeleft()/3600000.0));
    Serial.print("\r\nMili:");
    Serial.print(String(millis()));
#if(DEVICE== DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY)
    Serial.print("\r\nDi:");
    Serial.println(di);
#endif
}

void loop () {
    thrContrl.run();
}

void setup() {
    // initialize the Serial
    Serial.begin(9600);
    Wire.begin();
    Serial.println("Starting TelegramBot...");
    Serial.print("Bot Token: ");
    Serial.println(token);

    // connect the ESP8266 to the desired access point
    myBot.wifiConnect(ssid, pass);

    // set the telegram bot token
    myBot.setTelegramToken(token);

    // check if all things are ok
    if (myBot.testConnection())
        Serial.println("\nTest Connection OK");
    else
        Serial.println("\nTest Connection NOK");

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
    CmdKbd.addButton("\xF0\x9F\x94\x8D", STATUS_CALLBACK, CTBotKeyboardButtonQuery);
#if(DEVICE==DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY || DEVICE==DEVICE_WIREBULB)
    CmdKbd.addButton("\xF0\x9F\x9A\xAB", OFF_CALLBACK, CTBotKeyboardButtonQuery);
    CmdKbd.addRow();
    CmdKbd.addButton("1 H", "TIMER_1H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("2 H", "TIMER_2H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("3 H", "TIMER_3H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("4 H", "TIMER_4H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("5 H", "TIMER_5H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("6 H", "TIMER_6H", CTBotKeyboardButtonQuery);
    CmdKbd.addRow();
    CmdKbd.addButton("7 H", "TIMER_7H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("8 H", "TIMER_8H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("9 H", "TIMER_9H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("10 H", "TIMER_10H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("11 H", "TIMER_11H", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("12 H", "TIMER_12H", CTBotKeyboardButtonQuery);
#endif
#if(DEVICE==DEVICE_AC_BEDROOM || DEVICE==DEVICE_AC_LIBRARY)
    CmdKbd.addRow();
    CmdKbd.addButton("70 DI", "DI_70", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("71 DI", "DI_71", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("72 DI", "DI_72", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("73 DI", "DI_73", CTBotKeyboardButtonQuery);
    CmdKbd.addRow();
    CmdKbd.addButton("74 DI", "DI_74", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("75 DI", "DI_75", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("76 DI", "DI_76", CTBotKeyboardButtonQuery);
    CmdKbd.addButton("77 DI", "DI_77", CTBotKeyboardButtonQuery);
#endif
    thrTimer.enabled = true;
    thrTimer.setInterval(60000);
    thrTimer.onRun(thrfTimer);
    thrContrl.add(&thrTimer);

    thrTelegram.enabled = true;
    thrTelegram.setInterval(1000);
    thrTelegram.onRun(thrfTelegram);
    thrContrl.add(&thrTelegram);
}
