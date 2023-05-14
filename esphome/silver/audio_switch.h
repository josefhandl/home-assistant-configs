
#include "esphome.h"

class AudioSwitch : public Component, public CustomAPIDevice {
private:

protected:
    static AudioSwitch* instance_;

    AudioSwitch() {}

public:
    int state = 42;

    static AudioSwitch *getInstance() {
        if (AudioSwitch::instance_ == nullptr) {
            AudioSwitch::instance_ = new AudioSwitch();
        }

        return AudioSwitch::instance_;
    }


    void setup() override {
        // This will be called once to set up the component
        // think of it as the setup() call in Arduino
        pinMode(5, INPUT);
        pinMode(4, OUTPUT);

        //subscribe_homeassistant_state(&AudioSwitch::on_state_changed, "sensor.input");
    }
    void loop() override {
        // This will be called very often after setup time.
        // think of it as the loop() call in Arduino
        if (digitalRead(5)) {
            digitalWrite(4, HIGH);

            // You can also log messages
            ESP_LOGD("custom", "The GPIO pin 5 is HIGH!");
        }
        delay(100);
    }
};

AudioSwitch* AudioSwitch::instance_ = nullptr;
