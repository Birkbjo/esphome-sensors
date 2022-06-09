#include "esphome.h"
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include "ir_Trotec.h"

const uint16_t kIrLed = 0; // LED PIN
IRTrotec3550 ac(kIrLed);

class WoodsAC : public Component, public Climate {
  public:
    sensor::Sensor *sensor_{nullptr};

    void set_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }

    void setup() override
    {
      if (this->sensor_) {
        this->sensor_->add_on_state_callback([this](float state) {
          this->current_temperature = state;
          this->publish_state();
        });
        this->current_temperature = this->sensor_->state;
      } else {
        this->current_temperature = NAN;
      }

      auto restore = this->restore_state_();
      if (restore.has_value()) {
        restore->apply(this);
      } else {
        this->mode = climate::CLIMATE_MODE_OFF;
        const float minz = 17;
        const float max = 30;
        this->target_temperature = roundf(clamp(this->current_temperature, minz, max));
        this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
        this->swing_mode = climate::CLIMATE_SWING_OFF;
      }

      if (isnan(this->target_temperature)) {
        this->target_temperature = 17;
      }

      ac.begin();
      ac.on();
      if (this->mode == CLIMATE_MODE_OFF) {
        ac.off();
      } else if (this->mode == CLIMATE_MODE_COOL) {
        ac.setMode(kTrotecCool);
      } else if (this->mode == CLIMATE_MODE_DRY) {
        ac.setMode(kTrotecDry);
      } else if (this->mode == CLIMATE_MODE_FAN_ONLY) {
        ac.setMode(kTrotecFan);
      }
      ac.setTemp(this->target_temperature);
      if (this->fan_mode == CLIMATE_FAN_LOW) {
        ac.setFan(kTrotecFanLow);
      } else if (this->fan_mode == CLIMATE_FAN_MEDIUM) {
        ac.setFan(kTrotecFanMed);
      } else if (this->fan_mode == CLIMATE_FAN_HIGH) {
        ac.setFan(kTrotecFanHigh);
      }
      if (this->swing_mode == CLIMATE_SWING_OFF) {
        ac.setSwingV(false);
      } else if (this->swing_mode == CLIMATE_SWING_VERTICAL) {
        ac.setSwingV(true);
      }
      if (this->mode == CLIMATE_MODE_OFF) {
        ac.off();
        ac.send();
      } else {
        ac.send();
      }

      ESP_LOGD("DEBUG", "WOODS A/C remote is in the following state:");
      ESP_LOGD("DEBUG", "  %s\n", ac.toString().c_str());
    }

    climate::ClimateTraits traits() {
      auto traits = climate::ClimateTraits();
      traits.set_supports_cool_mode(true);
      traits.set_supports_current_temperature(this->sensor_ != nullptr);
      traits.set_supports_dry_mode(true);
      traits.set_supports_fan_mode_auto(false);
      traits.set_supports_fan_mode_high(true);
      traits.set_supports_fan_mode_low(true);
      traits.set_supports_fan_mode_medium(true);
      traits.set_supports_fan_only_mode(true);
      traits.set_supports_swing_mode_off(true);
      traits.set_supports_swing_mode_vertical(true);
      traits.set_supports_two_point_target_temperature(false);
      traits.set_visual_max_temperature(30);
      traits.set_visual_min_temperature(17);
      traits.set_visual_temperature_step(1);

      return traits;
    }

  void control(const ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      ClimateMode mode = *call.get_mode();
      if (mode == CLIMATE_MODE_OFF) {
        ac.off();
      } else if (mode == CLIMATE_MODE_COOL) {
        ac.on();
        ac.setMode(kTrotecCool);
      } else if (mode == CLIMATE_MODE_DRY) {
        ac.on();
        ac.setMode(kTrotecDry);
      } else if (mode == CLIMATE_MODE_FAN_ONLY) {
        ac.on();
        ac.setMode(kTrotecFan);
      }
      this->mode = mode;
    }

    if (call.get_target_temperature().has_value()) {
      float temp = *call.get_target_temperature();
      ac.setTemp(temp);
      this->target_temperature = temp;
    }

    if (call.get_fan_mode().has_value()) {
      ClimateFanMode fan_mode = *call.get_fan_mode();
      if (fan_mode == CLIMATE_FAN_LOW) {
        ac.setFan(kTrotecFanLow);
      } else if (fan_mode == CLIMATE_FAN_MEDIUM) {
        ac.setFan(kTrotecFanMed);
      } else if (fan_mode == CLIMATE_FAN_HIGH) {
        ac.setFan(kTrotecFanHigh);
      }
      this->fan_mode = fan_mode;
    }

    if (call.get_swing_mode().has_value()) {
      ClimateSwingMode swing_mode = *call.get_swing_mode();
      if (swing_mode == CLIMATE_SWING_OFF) {
        ac.setSwingV(false);
      } else if (swing_mode == CLIMATE_SWING_VERTICAL) {
        ac.setSwingV(true);
      }
      this->swing_mode = swing_mode;
    }

    if (this->mode == CLIMATE_MODE_OFF) {
        ac.off();
        ac.send();
    } else {
      ac.send();
    }

    this->publish_state();

    ESP_LOGD("DEBUG", "WOODS A/C remote is in the following state:");
    ESP_LOGD("DEBUG", "  %s\n", ac.toString().c_str());
  }
};