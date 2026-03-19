#pragma once
#include "esphome/components/light/light_effect.h"

namespace esphome {
namespace bleewer_light {

class BLEewerLight;  // forward declaration

/// A Neewer hardware animation effect. The light runs the animation internally —
/// we just send the ANM command once on start.
class NeewerEffect : public light::LightEffect {
 public:
  explicit NeewerEffect(const char *name) : LightEffect(name) {}

  void set_effect_id(uint8_t id) { effect_id_ = id; }
  void set_brightness(uint8_t v) { brightness_ = v; }
  void set_speed(uint8_t v) { speed_ = v; }
  void set_temp(uint8_t v) { temp_ = v; }
  void set_gm(uint8_t v) { gm_ = v; }
  void set_sparks(uint8_t v) { sparks_ = v; }
  void set_bright_min(uint8_t v) { bright_min_ = v; }
  void set_bright_max(uint8_t v) { bright_max_ = v; }
  void set_hue(uint16_t v) { hue_ = v; }
  void set_saturation(uint8_t v) { saturation_ = v; }
  void set_special_options(uint8_t v) { special_options_ = v; }
  void set_temp_min(uint8_t v) { temp_min_ = v; }
  void set_temp_max(uint8_t v) { temp_max_ = v; }

  void start() override;
  void apply() override {}  // No-op: light runs animation internally
  void stop() override;

 protected:
  std::vector<uint8_t> build_params_();

  uint8_t effect_id_{1};
  uint8_t brightness_{100};
  uint8_t speed_{5};
  uint8_t temp_{56};
  uint8_t gm_{50};
  uint8_t sparks_{4};
  uint8_t bright_min_{20};
  uint8_t bright_max_{100};
  uint16_t hue_{0};
  uint8_t saturation_{100};
  uint8_t special_options_{2};
  uint8_t temp_min_{32};
  uint8_t temp_max_{56};
};

}  // namespace bleewer_light
}  // namespace esphome
