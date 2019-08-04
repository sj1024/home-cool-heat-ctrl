// vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
#include "RELAY.h"
using namespace std;

RELAY::RELAY(int _pin) {
    pin = _pin;
    ctrlpin(0);
}

void RELAY::ctrlpin(int c) {
    ctrl = c;
    if(ctrl == 1) {
        pinMode(pin,   OUTPUT);
        digitalWrite(pin, LOW);               // Relay ON 
    } else {
        digitalWrite(pin, HIGH);              // Relay OFF
        pinMode(pin, INPUT);
    } 
} 

