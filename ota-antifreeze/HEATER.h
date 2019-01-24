#include <DHT12.h>

typedef void (*event_cb_t)(void);

typedef struct {
    float temp          =0;
    float humi          =0;
    float di            =0;
    float wet_temp      =0;
    float dew_point     =0;
    float absolute_humi =0;
} t_Climate_Def; 

class HEATER {
    public:
        int ctrl;
        HEATER(int _pin, DHT12* dht12, event_cb_t cb);
		t_Climate_Def getclimate(void);
        void ctrlpin(int c);
    private:
        int pin;
        String status;
        DHT12 *dht;                                  // Default Unit = CELSIUS, ID = 0x5c
        t_Climate_Def       climate;
        event_cb_t callback;
        void machine();
};

