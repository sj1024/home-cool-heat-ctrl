/*
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

#define OFF_CALLBACK "RelayOff" // callback data sent when "LIGHT OFF" button is pressed
#define STATUS_CALLBACK "Status" // callback data sent when "LIGHT OFF" button is pressed

#define PIN_RELAY           WEMOS_PIN_D5
#define PIN_LED             WEMOS_PIN_D4

CTBot myBot;
CTBotInlineKeyboard CmdKbd;  // custom inline keyboard object helper

String ssid = WIFI_SSID;     // REPLACE mySSID WITH YOUR WIFI SSID
String pass = WIFI_PASSWORD; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
//String token = AC_BEDROOM_BOT_TOKEN;   // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
String token = AC_LIBRARY_BOT_TOKEN;   // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN

long timer = 0;
float di = 0;

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
    t_Climate_Def climate;
    dht12.readClimate(&climate);
    if(climate.di>di && get_timeleft() > 0) {
        relay.ctrlpin(1); // on
    } else {
        relay.ctrlpin(0); // off
    }

}
void call_status (TBMessage msg) {
  String status;
  t_Climate_Def climate;
  dht12.readClimate(&climate);
  status =  String("Temp: ") + String(climate.temp) + \
            String("\nDi: ") + String(climate.di) + \
            String("\nDiCtrl: ") + String(di) + \
            String("\nTimer: ") + String((float)get_timeleft()/3600000) + String(" H");
  myBot.sendMessage(msg.sender.id,  status, CmdKbd);
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
  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    // check what kind of message I received
    if (msg.messageType == CTBotMessageText) {
      // received a text message
      myBot.sendMessage(msg.sender.id, "Getting Start", CmdKbd);
    } else if (msg.messageType == CTBotMessageQuery) {
      // received a callback query message
      if (msg.callbackQueryData.equals("DI_70")) {
        set_di(msg, 70);
      } else if (msg.callbackQueryData.equals("DI_71")) {
        set_di(msg, 71);
      } else if (msg.callbackQueryData.equals("DI_72")) {
        set_di(msg, 72);
      } else if (msg.callbackQueryData.equals("DI_73")) {
        set_di(msg, 73);
      } else if (msg.callbackQueryData.equals("DI_74")) {
        set_di(msg, 74);
      } else if (msg.callbackQueryData.equals("DI_75")) {
        set_di(msg, 75);
      } else if (msg.callbackQueryData.equals("DI_76")) {
        set_di(msg, 76);
      } else if (msg.callbackQueryData.equals("DI_77")) {
        set_di(msg, 77);
      } else if (msg.callbackQueryData.equals("TIMER_1H")) {
        set_timer(msg, 1);
      } else if (msg.callbackQueryData.equals("TIMER_2H")) {
        set_timer(msg, 2);
      } else if (msg.callbackQueryData.equals("TIMER_3H")) {
        set_timer(msg, 3);
      } else if (msg.callbackQueryData.equals("TIMER_4H")) {
        set_timer(msg, 4);
      } else if (msg.callbackQueryData.equals("TIMER_5H")) {
        set_timer(msg, 5);
      } else if (msg.callbackQueryData.equals("TIMER_6H")) {
        set_timer(msg, 6);
      } else if (msg.callbackQueryData.equals("TIMER_7H")) {
        set_timer(msg, 7);
      } else if (msg.callbackQueryData.equals("TIMER_8H")) {
        set_timer(msg, 8);
      } else if (msg.callbackQueryData.equals("TIMER_9H")) {
        set_timer(msg, 9);
      } else if (msg.callbackQueryData.equals("TIMER_10H")) {
        set_timer(msg, 10);
      } else if (msg.callbackQueryData.equals("TIMER_11H")) {
        set_timer(msg, 11);
      } else if (msg.callbackQueryData.equals("TIMER_12H")) {
        set_timer(msg, 12);
      } else if (msg.callbackQueryData.equals(STATUS_CALLBACK)) {
        call_status(msg);
      } else if (msg.callbackQueryData.equals(OFF_CALLBACK)) {
        timer = 0;
        relay.ctrlpin(0);
        call_status(msg);
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

}

void loop () {
  thrContrl.run();
}

void setup() {
  // initialize the Serial
  Serial.begin(9600);
  Wire.begin();
  Serial.println("Starting TelegramBot...");

  // connect the ESP8266 to the desired access point
  myBot.wifiConnect(ssid, pass);

  // set the telegram bot token
  myBot.setTelegramToken(token);

  // check if all things are ok
  if (myBot.testConnection())
    Serial.println("\ntestConnection OK");
  else
    Serial.println("\ntestConnection NOK");


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

  // inline keyboard customization
  // add a query button to the first row of the inline keyboard
  // add another query button to the first row of the inline keyboard
  CmdKbd.addButton("OFF", OFF_CALLBACK, CTBotKeyboardButtonQuery);
  // add a new empty button row
  CmdKbd.addButton("STATUS", STATUS_CALLBACK, CTBotKeyboardButtonQuery);
  CmdKbd.addRow();
  //
  CmdKbd.addButton("DI 70", "DI_70", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("DI 71", "DI_71", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("DI 72", "DI_72", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("DI 73", "DI_73", CTBotKeyboardButtonQuery);
  CmdKbd.addRow();
  CmdKbd.addButton("DI 74", "DI_74", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("DI 75", "DI_75", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("DI 76", "DI_76", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("DI 77", "DI_77", CTBotKeyboardButtonQuery);
  CmdKbd.addRow();
  //
  CmdKbd.addButton("1 H", "TIMER_1H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("2 H", "TIMER_2H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("3 H", "TIMER_3H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("4 H", "TIMER_4H", CTBotKeyboardButtonQuery);
  CmdKbd.addRow();
  CmdKbd.addButton("5 H", "TIMER_5H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("6 H", "TIMER_6H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("7 H", "TIMER_7H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("8 H", "TIMER_8H", CTBotKeyboardButtonQuery);
  CmdKbd.addRow();
  CmdKbd.addButton("9 H", "TIMER_9H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("10 H", "TIMER_10h", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("11 H", "TIMER_11H", CTBotKeyboardButtonQuery);
  CmdKbd.addButton("12 H", "TIMER_12H", CTBotKeyboardButtonQuery);

  thrTimer.enabled = true;
  thrTimer.setInterval(60000);
  thrTimer.onRun(thrfTimer);
  thrContrl.add(&thrTimer);

  thrTelegram.enabled = true;
  thrTelegram.setInterval(1000);
  thrTelegram.onRun(thrfTelegram);
  thrContrl.add(&thrTelegram);
}
