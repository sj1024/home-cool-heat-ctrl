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
#include <Wemospin.h>
#include <Thread.h>
#include <ThreadController.h>
#include "RELAY.h"

#define ON_CALLBACK  "RelayOn"  // callback data sent when "LIGHT ON" button is pressed
#define OFF_CALLBACK "RelayOff" // callback data sent when "LIGHT OFF" button is pressed
#define STATUS_CALLBACK "Status" // callback data sent when "LIGHT OFF" button is pressed

#define PIN_RELAY           WEMOS_PIN_D5
#define PIN_LED             WEMOS_PIN_D4

CTBot myBot;
CTBotInlineKeyboard CmdKbd;  // custom inline keyboard object helper
CTBotInlineKeyboard TimerKbd;  // custom inline keyboard object helper

String ssid = WIFI_SSID;     // REPLACE mySSID WITH YOUR WIFI SSID
String pass = WIFI_PASSWORD; // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
//String token = AC_BEDROOM_BOT_TOKEN;   // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
//String token = AC_LIBRARY_BOT_TOKEN;   // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
String token = CHAINBULB_BOT_TOKEN;   // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN


RELAY relay = RELAY(PIN_RELAY);

Thread thrTelegram = Thread();
Thread thrTimer    = Thread();
ThreadController thrContrl  = ThreadController();

long timer = 0;

void machine() {
    Serial.print("\n\r%%%%%%%%%%%%%%%%");
    Serial.print("\n\rTime left: ");
    Serial.print(String(timer));
    if(timer > 0) {
        relay.ctrlpin(1); // on
    } else {
        relay.ctrlpin(0); // off
    }
}
void call_status (TBMessage msg) {
  String status =  String("Timer: ") + String((float)timer/60) + String(" H");
  myBot.sendMessage(msg.sender.id,  status, CmdKbd);
}

void call_timer(TBMessage msg) {
    myBot.sendMessage(msg.sender.id, "Choose Timer in Hour", TimerKbd);
}

void call_run(TBMessage msg, long _timer) {
   timer = _timer*60; // hour to min 
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
      if (msg.callbackQueryData.equals(ON_CALLBACK)) {
	    call_timer(msg);
      } else if (msg.callbackQueryData.equals("TIMER_1H")) {
        call_run(msg, 1);
      } else if (msg.callbackQueryData.equals("TIMER_2H")) {
        call_run(msg, 2);
      } else if (msg.callbackQueryData.equals("TIMER_3H")) {
        call_run(msg, 3);
      } else if (msg.callbackQueryData.equals("TIMER_4H")) {
        call_run(msg, 4);
      } else if (msg.callbackQueryData.equals("TIMER_5H")) {
        call_run(msg, 5);
      } else if (msg.callbackQueryData.equals("TIMER_6H")) {
        call_run(msg, 6);
      } else if (msg.callbackQueryData.equals("TIMER_7H")) {
        call_run(msg, 7);
      } else if (msg.callbackQueryData.equals("TIMER_8H")) {
        call_run(msg, 8);
      } else if (msg.callbackQueryData.equals("TIMER_9H")) {
        call_run(msg, 9);
      } else if (msg.callbackQueryData.equals("TIMER_10H")) {
        call_run(msg, 10);
      } else if (msg.callbackQueryData.equals("TIMER_11H")) {
        call_run(msg, 11);
      } else if (msg.callbackQueryData.equals("TIMER_12H")) {
        call_run(msg, 12);
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


int Bot_mtbs = 1000*60;  // 1min
long Bot_lasttime; 

void thrfTimer () {
  static int cnt = 0;
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    Bot_lasttime = millis();
    if(0 >= timer) {
        relay.ctrlpin(0);
    } else {
        timer--;
    }
    if((cnt++ % 5) == 0) { //every 5 min 
        machine();
    }
    Serial.println(String(timer));
  }
}

void thrfTelegram () {
    TelegramMessageHandler();
}

void loop () {
  thrContrl.run();
}

void setup() {
  // initialize the Serial
  Serial.begin(9600);
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

  // inline keyboard customization
  // add a query button to the first row of the inline keyboard
  CmdKbd.addButton("ON", ON_CALLBACK, CTBotKeyboardButtonQuery);
  // add another query button to the first row of the inline keyboard
  CmdKbd.addButton("OFF", OFF_CALLBACK, CTBotKeyboardButtonQuery);
  // add a new empty button row
  CmdKbd.addRow();
  CmdKbd.addButton("STATUS", STATUS_CALLBACK, CTBotKeyboardButtonQuery);
  //
  TimerKbd.addButton("1H", "TIMER_1H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("2H", "TIMER_2H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("3H", "TIMER_3H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("4H", "TIMER_4H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("5H", "TIMER_5H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("6H", "TIMER_6H", CTBotKeyboardButtonQuery);
  TimerKbd.addRow();
  TimerKbd.addButton("7H", "TIMER_7H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("8H", "TIMER_8H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("9H", "TIMER_9H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("10H", "TIMER_10h", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("11H", "TIMER_11H", CTBotKeyboardButtonQuery);
  TimerKbd.addButton("12H", "TIMER_12H", CTBotKeyboardButtonQuery);

  thrTimer.enabled = true;
  thrTimer.setInterval(5000);
  thrTimer.onRun(thrfTimer);
  thrContrl.add(&thrTimer);

  thrTelegram.enabled = true;
  thrTelegram.setInterval(1000);
  thrTelegram.onRun(thrfTelegram);
  thrContrl.add(&thrTelegram);
}
