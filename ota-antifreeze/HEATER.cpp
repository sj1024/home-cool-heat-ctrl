// vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
#include "HEATER.h"
using namespace std;

HEATER::HEATER(int _pin, DHT12* dht12, event_cb_t cb) {
    pin = _pin;
    dht = dht12;
    callback = cb;
    ctrlpin(0);
}

void HEATER::ctrlpin(int c) {
    ctrl = c;
    if(ctrl == 1) {
        pinMode(pin,   OUTPUT);
        digitalWrite(pin, LOW);               // Relay ON 
        callback();
    } else {
        digitalWrite(pin, HIGH);              // Relay OFF
        pinMode(pin, INPUT);
    } 
} 

