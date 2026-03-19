#include "neewer_effects.h"
#include "bleewer_light.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace bleewer_light {

static const char *TAG = "neewer_effects";

void NeewerEffect::start() {
  auto *output = static_cast<BLEewerLight *>(this->state_->get_output());

  // Use the current HA brightness for the effect, clamped to minimum of 16
  // (Neewer lights ignore effects below ~15% brightness)
  float ha_brightness;
  this->state_->current_values_as_brightness(&ha_brightness);
  if (ha_brightness > 0.0f) {
    uint8_t bri = static_cast<uint8_t>(std::round(ha_brightness * 100.0f));
    bri = std::max(bri, static_cast<uint8_t>(16));
    this->brightness_ = bri;
    this->bright_max_ = bri;
  }

  auto params = this->build_params_();
  auto cmd = output->get_protocol()->build_anm_command(this->effect_id_, params);
  output->queue_command(cmd);
  ESP_LOGI(TAG, "Started effect '%s' (id=%d, bri=%d)", this->get_name(), this->effect_id_, this->brightness_);
}

void NeewerEffect::stop() {
  ESP_LOGD(TAG, "Stopped effect '%s'", this->get_name());
  // Switching to CCT/HSI mode will be handled by the next write_state call
}

std::vector<uint8_t> NeewerEffect::build_params_() {
  // Each effect has a unique parameter layout matching NeewerLite-Python's calculateByteString
  std::vector<uint8_t> p;

  switch (this->effect_id_) {
    case 1:  // Lightning
      p = {brightness_, temp_, speed_};
      break;
    case 2:   // Paparazzi
    case 3:   // Defective Bulb
    case 6:   // CCT Flash
    case 8:   // CCT Pulse
      p = {brightness_, temp_, gm_, speed_};
      break;
    case 4:   // Explosion
      p = {brightness_, temp_, gm_, speed_, sparks_};
      break;
    case 5:   // Welding
      p = {bright_min_, bright_max_, temp_, gm_, speed_};
      break;
    case 7:   // Hue Flash
    case 9:   // Hue Pulse
      p = {brightness_, static_cast<uint8_t>(hue_ & 0xFF), static_cast<uint8_t>((hue_ >> 8) & 0xFF),
           saturation_, speed_};
      break;
    case 10:  // Cop Car
      p = {brightness_, special_options_, speed_};
      break;
    case 11:  // Candlelight
      p = {bright_min_, bright_max_, temp_, gm_, speed_, sparks_};
      break;
    case 12:  // Hue Loop
      p = {brightness_,
           static_cast<uint8_t>(hue_ & 0xFF), static_cast<uint8_t>((hue_ >> 8) & 0xFF),       // hue_min (reuse hue)
           static_cast<uint8_t>((hue_ + 180) & 0xFF), static_cast<uint8_t>(((hue_ + 180) >> 8) & 0xFF),  // hue_max
           speed_};
      break;
    case 13:  // CCT Loop
      p = {brightness_, temp_min_, temp_max_, speed_};
      break;
    case 14:  // INT Loop (CCT variant, subtype=0)
      p = {0, bright_min_, bright_max_, 0, 0, temp_, speed_};
      break;
    case 15:  // TV Screen (protocol effect 15)
      p = {bright_min_, bright_max_, temp_, gm_, speed_};
      break;
    case 16:  // Fireworks (protocol effect 16)
      p = {brightness_, special_options_, speed_, sparks_};
      break;
    case 17:  // Party (protocol effect 17)
      p = {brightness_, special_options_, speed_};
      break;
    default:
      ESP_LOGW(TAG, "Unknown effect id %d", this->effect_id_);
      p = {brightness_, speed_};
      break;
  }

  return p;
}

}  // namespace bleewer_light
}  // namespace esphome
