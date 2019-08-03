// vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
#include "Arduino.h"

class RELAY {
    public:
        int ctrl;
        RELAY(int _pin);
        void ctrlpin(int c);
    private:
        int pin;
};

