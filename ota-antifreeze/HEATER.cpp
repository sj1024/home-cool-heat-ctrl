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

t_Climate_Def HEATER::getclimate () {
    //  https://playground.arduino.cc/Main/Dht
    //  DHT12 Library --> https://github.com/Bobadas/DHT12_library_Arduino
    //
    //  Absolute Humidity --> http://biomet.ucdavis.edu/conversions/HumCon.pdf
    //  Wet-bulb Temperature --> http://journals.ametsoc.org/doi/pdf/10.1175/JAMC-D-11-0143.1
    //  Dew-point Temperature --> http://wahiduddin.net/calc/density_algorithms.htm
    float temp = dht->readTemperature();
    float humi = dht->readHumidity();
    float ah = 2165 * humi * 0.01 * 0.6108 * exp(17.27 * temp / (temp + 237.3)) / (temp + 273.16);
    float wtemp = temp * atan(0.151977 * pow(humi + 8.313659, 0.5)) + atan(temp + humi) 
                  - atan(humi - 1.676331) + 0.003918 * pow(humi, 1.5) * atan(0.023101 * humi) - 4.686035;
    float dew_point = temp - ((14.55 + 0.114 * temp) * (1. - 0.01 * humi) 
                  + pow((2.5 + 0.007 * temp) * (1. - 0.01 * humi), 3) + (15.9 + 0.117 * temp) * pow((1. - 0.01 * humi), 14));
    float di = 0.72 * (temp + wtemp) + 40.6;
    climate.temp = temp;
    climate.humi = humi;
    climate.wet_temp = wtemp;
    climate.absolute_humi = ah;
    climate.dew_point = dew_point;
    climate.di = di;
    return climate;
}

