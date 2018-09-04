// vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
//
#define WIFI                1

#include <Wemospin.h>
#define PIN_RELAY1          WEMOS_PIN_D5
#define PIN_RELAY2          WEMOS_PIN_D6
#define PIN_LED             WEMOS_PIN_D4

#include <Config.h>                          // password
#include <math.h>
#include <Thread.h>   
#include <ThreadController.h>   
 
#include <ESP8266WiFi.h>

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



Thread thrSetStatus         = Thread();
#if(WIFI == 1)
Thread thrRest              = Thread();
#endif
Thread thrTimer             = Thread();
ThreadController thrContrl  = ThreadController();


cRelay relay0 = cRelay(WEMOS_PIN_D0);
cRelay relay1 = cRelay(WEMOS_PIN_D1);

void Init ( ) {
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
    delay(1000);

#if(WIFI == 1)
    rest.variable("r0_ctrl",     &relay0.ctrl);
    rest.variable("r1_ctrl",     &relay1.ctrl);
    rest.variable("r0_timer",    &relay0.timer);
    rest.variable("r1_timer",    &relay1.timer);
    rest.function("relayctrl",   RelayCtrl);
#endif

    thrSetStatus.enabled    = true;
    thrSetStatus.setInterval(100);
    thrSetStatus.onRun(thrfSetStatus);

#if(WIFI == 1)
    thrRest.enabled         = true;
    thrRest.setInterval(100);
    thrRest.onRun(thrfRest);
#endif

    thrTimer.enabled         = true;
    thrTimer.setInterval(1000); // 1sec
    thrTimer.onRun(thrfTimer);

    thrContrl.add(&thrTimer);
    thrContrl.add(&thrSetStatus);
#if(WIFI == 1)
    thrContrl.add(&thrRest);
#endif
}
void loop ( ) {
    thrContrl.run();
}
void Machine ( ) {
}
void thrfSetStatus  () {
}
void thrfTimer ( ) {
    relay0.dectimer();
    relay1.dectimer();
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
    cRelay *cr;
    if(r.toInt() == 0) {
        cr = &relay0;
    } else {
        cr = &relay1;
    }
    cr->settimer(timer);
    return cr->timer;
}

