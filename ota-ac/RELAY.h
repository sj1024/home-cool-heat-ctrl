// vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
#include "Arduino.h"

typedef void (*event_cb_t)(void);
class RELAY {
    public:
        int ctrl;
        RELAY(int _pin, event_cb_t cb);
        void ctrlpin(int c);
    private:
        int pin;
        event_cb_t callback;
};

