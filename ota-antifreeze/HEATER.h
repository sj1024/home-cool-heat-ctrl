// vim: set rnu sw=4 ss=4 ts=4 et smartindent autoindent fdm=indent :
#include <DHT12.h>

typedef void (*event_cb_t)(void);

class HEATER {
    public:
        int ctrl;
        HEATER(int _pin, DHT12* dht12, event_cb_t cb);
        void ctrlpin(int c);
    private:
        int pin;
        String status;
        DHT12 *dht;                                  // Default Unit = CELSIUS, ID = 0x5c
        t_Climate_Def       climate;
        event_cb_t callback;
        void machine();
};

