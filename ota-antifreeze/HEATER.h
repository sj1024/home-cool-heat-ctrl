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
        int timer;
        int ctrl;
        float temp;
        float humi;
        HEATER(int _pin, DHT12* dht12, event_cb_t cb);
        void interval(void);
        void settimer(int t);
    private:
        int pin;
        int cnt;
        String status;
        DHT12 *dht;                                  // Default Unit = CELSIUS, ID = 0x5c
        t_Climate_Def       climate;
        event_cb_t callback;
        void ctrlpin(int c);
        void machine();
		void getclimate(void);
};

