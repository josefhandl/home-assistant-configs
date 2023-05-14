
#include "esphome.h"

class AudioInput : public PollingComponent, public Sensor {
private:
    //int previous_state = 42;

protected:
    static AudioInput* instance_;

    AudioInput() : PollingComponent(15000) {}

public:

    static AudioInput *getInstance() {
        if (AudioInput::instance_ == nullptr) {
            AudioInput::instance_ = new AudioInput();
        }

        return AudioInput::instance_;
    }

    void setup() override {
        // This will be called by App.setup()
    }
    void update() override {
        // This will be called every "update_interval" milliseconds.
        ESP_LOGD("custom", "published state 42.0");

        //if (this->previous_state != 42) {
        //    this->state = 42;
        //}

        //this->previous_state = this->state;
        publish_state(AudioSwitch::getInstance()->state);
    }
    void turn_off() {
        AudioSwitch::getInstance()->state = 0;
    }

    void turn_on() {
        AudioSwitch::getInstance()->state = 42;
    }
};

AudioInput* AudioInput::instance_ = nullptr;

