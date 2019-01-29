/*
    DHT12.h - Library for DHT12 sensor.
    v0.0.1 Beta
    Created by Bobadas, July 30,2016.
    Released into the public domain.
*/
#ifndef DHT12_h
#define DHT12_h
#include "Arduino.h"
#include "Wire.h"

#define CELSIUS        1
#define KELVIN        2
#define FAHRENHEIT    3

typedef struct {
    float temp;
    float humi;
    float absolute_humi;
    float wet_temp;
    float dew_point;
    float di;
} t_Climate_Def;

class DHT12
{
    public:
        DHT12(byte scale=0,byte id=0);
        float readTemperature(byte scale=0);
        float readHumidity();
        void readClimate(t_Climate_Def *climate);
    private:
        byte read();
        byte datos[5];
        byte _id;
        byte _scale;
};

#endif
