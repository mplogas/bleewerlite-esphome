#include "bleewer_light.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cmath>

namespace esphome {
namespace bleewer_light {

static const char *TAG = "bleewer_light";

// BLE UUIDs for Neewer lights
static const esp32_ble_tracker::ESPBTUUID SERVICE_UUID =
    esp32_ble_tracker::ESPBTUUID::from_raw("69400001-b5a3-f393-e0a9-e50e24dcca99");
static const esp32_ble_tracker::ESPBTUUID CHAR_UUID_WRITE =
    esp32_ble_tracker::ESPBTUUID::from_raw("69400002-b5a3-f393-e0a9-e50e24dcca99");
static const esp32_ble_tracker::ESPBTUUID CHAR_UUID_NOTIFY =
    esp32_ble_tracker::ESPBTUUID::from_raw("69400003-b5a3-f393-e0a9-e50e24dcca99");

void BLEewerLight::setup() {
  ESP_LOGD(TAG, "Setting up BLEewerLight...");
}

void BLEewerLight::loop() {
  this->flush_pending_commands_();
}

void BLEewerLight::dump_config() {
  ESP_LOGCONFIG(TAG, "BLEewer Light:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->model_name_.empty() ? "(auto-detect)" : this->model_name_.c_str());
  ESP_LOGCONFIG(TAG, "  CCT Only: %s", this->cct_only_ ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Infinity Mode: %d", this->protocol_.get_infinity_mode());
  if (this->cct_min_mireds_ > 0) {
    ESP_LOGCONFIG(TAG, "  CCT Range: %.1f - %.1f mireds (%dK - %dK)",
                  this->cct_min_mireds_, this->cct_max_mireds_,
                  mireds_to_kelvin_(this->cct_max_mireds_),
                  mireds_to_kelvin_(this->cct_min_mireds_));
  }
}

// --- BLE GATT event handler ---

void BLEewerLight::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                        esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK) {
        ESP_LOGI(TAG, "Connected to Neewer light");
      } else {
        ESP_LOGW(TAG, "Connection failed, status=%d", param->open.status);
      }
      break;
    }

    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGI(TAG, "Disconnected from Neewer light");
      this->connected_ = false;
      this->write_handle_ = 0;
      this->notify_handle_ = 0;
      this->pending_commands_.clear();
      break;
    }

    case ESP_GATTC_SEARCH_CMPL_EVT: {
      // Resolve specs from model name on first connection
      if (!this->specs_resolved_ && !this->model_name_.empty()) {
        this->resolve_specs_from_name_(this->model_name_);
      }

      // Extract MAC address for infinity protocol
      const uint8_t *mac = this->parent()->get_remote_bda();
      this->protocol_.set_mac_address(mac);
      ESP_LOGD(TAG, "Remote MAC: %02X:%02X:%02X:%02X:%02X:%02X",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

      // Find write characteristic
      auto *chr_write = this->parent()->get_characteristic(SERVICE_UUID, CHAR_UUID_WRITE);
      if (chr_write == nullptr) {
        ESP_LOGW(TAG, "Write characteristic not found");
        break;
      }
      this->write_handle_ = chr_write->handle;
      ESP_LOGD(TAG, "Write characteristic handle: 0x%04X", this->write_handle_);

      // Find notify characteristic and register for notifications
      auto *chr_notify = this->parent()->get_characteristic(SERVICE_UUID, CHAR_UUID_NOTIFY);
      if (chr_notify != nullptr) {
        this->notify_handle_ = chr_notify->handle;
        ESP_LOGD(TAG, "Notify characteristic handle: 0x%04X", this->notify_handle_);

        // Register for notifications
        auto err = esp_ble_gattc_register_for_notify(
            this->parent()->get_gattc_if(),
            this->parent()->get_remote_bda(),
            this->notify_handle_);
        if (err != ESP_OK) {
          ESP_LOGW(TAG, "Failed to register for notifications: %s", esp_err_to_name(err));
        }
      } else {
        ESP_LOGD(TAG, "Notify characteristic not found (optional)");
      }

      this->connected_ = true;
      this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
      ESP_LOGI(TAG, "GATT services resolved, ready to send commands");

      // Query initial power status
      this->status_queried_ = false;
      this->query_initial_status_();
      break;
    }

    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      if (param->reg_for_notify.status == ESP_GATT_OK) {
        ESP_LOGD(TAG, "Registered for notifications on handle 0x%04X", param->reg_for_notify.handle);
      } else {
        ESP_LOGW(TAG, "Failed to register for notify, status=%d", param->reg_for_notify.status);
      }
      break;
    }

    case ESP_GATTC_NOTIFY_EVT: {
      if (param->notify.handle == this->notify_handle_) {
        this->handle_notification_(param->notify.value, param->notify.value_len);
      }
      break;
    }

    case ESP_GATTC_WRITE_CHAR_EVT: {
      if (param->write.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Write characteristic failed, status=%d, handle=0x%04X",
                 param->write.status, param->write.handle);
      }
      break;
    }

    default:
      break;
  }
}

// --- Command queue and rate limiting ---

void BLEewerLight::queue_command_(const std::vector<uint8_t> &data) {
  if (!this->connected_ || this->write_handle_ == 0) {
    ESP_LOGW(TAG, "Not connected, dropping command");
    return;
  }
  if (this->pending_commands_.size() >= 10) {
    ESP_LOGW(TAG, "Command queue full, dropping oldest");
    this->pending_commands_.erase(this->pending_commands_.begin());
  }
  this->pending_commands_.push_back(data);
}

void BLEewerLight::flush_pending_commands_() {
  if (this->pending_commands_.empty()) {
    return;
  }

  uint32_t now = millis();
  if (now - this->last_write_time_ < MIN_WRITE_INTERVAL_MS) {
    return;
  }

  auto cmd = this->pending_commands_.front();
  this->pending_commands_.erase(this->pending_commands_.begin());
  this->send_command_(cmd);
  this->last_write_time_ = now;
}

void BLEewerLight::send_command_(const std::vector<uint8_t> &data) {
  if (this->write_handle_ == 0) {
    ESP_LOGW(TAG, "No write handle, cannot send command");
    return;
  }

  // Log hex dump of command
  char hex[64];
  int pos = 0;
  for (size_t i = 0; i < data.size() && pos < 60; i++) {
    pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", data[i]);
  }
  ESP_LOGV(TAG, "BLE TX [%d bytes]: %s", data.size(), hex);

  esp_err_t err = esp_ble_gattc_write_char(
      this->parent()->get_gattc_if(),
      this->parent()->get_conn_id(),
      this->write_handle_,
      data.size(),
      const_cast<uint8_t *>(data.data()),
      ESP_GATT_WRITE_TYPE_NO_RSP,
      ESP_GATT_AUTH_REQ_NONE);

  if (err != ESP_OK) {
    ESP_LOGW(TAG, "esp_ble_gattc_write_char failed: %s", esp_err_to_name(err));
  }
}

// --- LightOutput interface ---

light::LightTraits BLEewerLight::get_traits() {
  auto traits = light::LightTraits();

  if (this->cct_only_) {
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  } else {
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE, light::ColorMode::RGB});
  }

  // Set CCT range: use configured values or defaults (3200K-5600K)
  float min_mireds = this->cct_min_mireds_ > 0 ? this->cct_min_mireds_ : 178.6f;   // 5600K
  float max_mireds = this->cct_max_mireds_ > 0 ? this->cct_max_mireds_ : 312.5f;   // 3200K
  traits.set_min_mireds(min_mireds);
  traits.set_max_mireds(max_mireds);

  return traits;
}

void BLEewerLight::write_state(light::LightState *state) {
  float brightness;
  state->current_values_as_brightness(&brightness);

  // Power OFF when brightness is zero
  if (brightness == 0.0f) {
    if (this->power_state_) {
      this->queue_command_(this->protocol_.build_power_command(false));
      this->power_state_ = false;
    }
    return;
  }

  // Power ON if currently off
  if (!this->power_state_) {
    this->queue_command_(this->protocol_.build_power_command(true));
    this->power_state_ = true;
  }

  // Don't send color/brightness commands while a hardware effect is active —
  // the light runs the animation internally
  if (state->get_current_effect_index() != 0) {
    return;
  }

  uint8_t bri = static_cast<uint8_t>(std::round(brightness * 100.0f));
  bri = std::max(static_cast<uint8_t>(1), std::min(static_cast<uint8_t>(100), bri));

  auto mode = state->current_values.get_color_mode();

  if (mode == light::ColorMode::RGB) {
    float red, green, blue;
    state->current_values_as_rgb(&red, &green, &blue);

    // RGB to HSV conversion
    float max_c = std::max({red, green, blue});
    float min_c = std::min({red, green, blue});
    float delta = max_c - min_c;

    float hue = 0.0f;
    if (delta > 0.0f) {
      if (max_c == red) {
        hue = 60.0f * fmod((green - blue) / delta, 6.0f);
      } else if (max_c == green) {
        hue = 60.0f * ((blue - red) / delta + 2.0f);
      } else {
        hue = 60.0f * ((red - green) / delta + 4.0f);
      }
    }
    if (hue < 0.0f) hue += 360.0f;

    float saturation = (max_c > 0.0f) ? (delta / max_c) * 100.0f : 0.0f;

    uint16_t hue_int = static_cast<uint16_t>(std::round(hue)) % 360;
    uint8_t sat_int = static_cast<uint8_t>(std::round(saturation));
    sat_int = std::min(static_cast<uint8_t>(100), sat_int);

    ESP_LOGD(TAG, "RGB mode: hue=%d sat=%d bri=%d", hue_int, sat_int, bri);
    this->queue_command_(this->protocol_.build_hsi_command(hue_int, sat_int, bri));
  } else {
    // Color temperature mode — get_color_temperature() returns mireds directly
    float ct_mireds = state->current_values.get_color_temperature();
    if (ct_mireds <= 0) {
      auto traits = this->get_traits();
      ct_mireds = (traits.get_min_mireds() + traits.get_max_mireds()) / 2.0f;
    }

    uint16_t kelvin = mireds_to_kelvin_(ct_mireds);
    uint8_t temp = kelvin_to_neewer_temp_(kelvin);

    // Use state * brightness for actual output level
    float state_bri = state->current_values.get_brightness();
    float state_val = state->current_values.get_state();
    bri = static_cast<uint8_t>(std::round(state_val * state_bri * 100.0f));
    bri = std::max(static_cast<uint8_t>(1), std::min(static_cast<uint8_t>(100), bri));

    if (this->cct_only_) {
      // CCT-only lights use separate brightness (0x82) and temp (0x83) commands
      ESP_LOGD(TAG, "CCT separate: bri=%d temp=%d (%dK)", bri, temp, kelvin);
      this->queue_command_(this->protocol_.build_old_brightness_command(bri));
      this->queue_command_(this->protocol_.build_old_temp_command(temp));
    } else {
      ESP_LOGD(TAG, "CCT combined: bri=%d temp=%d (%dK)", bri, temp, kelvin);
      this->queue_command_(this->protocol_.build_cct_command(bri, temp));
    }
  }
}

// --- Status reading ---

void BLEewerLight::query_initial_status_() {
  if (this->status_queried_ || this->write_handle_ == 0) return;
  this->status_queried_ = true;

  // Query power status: [0x78, 0x85, 0x00, 0xFD]
  ESP_LOGD(TAG, "Querying initial power status...");
  std::vector<uint8_t> query_power = {0x78, 0x85, 0x00, 0xFD};
  this->queue_command_(query_power);

  // Query channel: [0x78, 0x84, 0x00, 0xFC]
  std::vector<uint8_t> query_channel = {0x78, 0x84, 0x00, 0xFC};
  this->queue_command_(query_channel);
}

void BLEewerLight::handle_notification_(uint8_t *data, uint16_t length) {
  if (length < 2) return;

  // Log raw notification
  char hex[64];
  int pos = 0;
  for (uint16_t i = 0; i < length && pos < 60; i++) {
    pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", data[i]);
  }
  ESP_LOGD(TAG, "Notify RX [%d bytes]: %s", length, hex);

  // Response format: [0x78, type, ...]
  if (data[0] != 0x78 || length < 4) return;

  uint8_t response_type = data[1];

  if (response_type == 0x02) {
    // Power status response: data[3] = 1 (ON) or 2 (STANDBY)
    bool is_on = (data[3] == 0x01);
    ESP_LOGI(TAG, "Power status: %s", is_on ? "ON" : "STANDBY");
    this->power_state_ = is_on;
  } else if (response_type == 0x01) {
    // Channel/mode response: data[3] = current channel
    ESP_LOGI(TAG, "Current channel: %d", data[3]);
  } else {
    ESP_LOGD(TAG, "Unknown notification type: 0x%02X", response_type);
  }
}

// --- Specs resolution ---

void BLEewerLight::resolve_specs_from_name_(const std::string &name) {
  if (this->specs_resolved_) return;

  std::string corrected = NeewerProtocol::get_corrected_name(name);
  LightSpecs specs = NeewerProtocol::get_light_specs(corrected);

  // Only apply auto-detected values if not overridden in YAML (0 means not set)
  if (this->cct_min_mireds_ == 0 && specs.cct_max_k > 0) {
    this->cct_min_mireds_ = 1000000.0f / static_cast<float>(specs.cct_max_k);
  }
  if (this->cct_max_mireds_ == 0 && specs.cct_min_k > 0) {
    this->cct_max_mireds_ = 1000000.0f / static_cast<float>(specs.cct_min_k);
  }

  // Auto-set CCT-only if not explicitly configured
  // (cct_only_ defaults to false, so only override if specs say CCT-only)
  if (specs.cct_only) {
    this->cct_only_ = true;
  }

  // Auto-set infinity mode if detected
  if (specs.infinity_mode != 0) {
    ESP_LOGD(TAG, "Auto-detected infinity_mode=%d from model '%s'", specs.infinity_mode, corrected.c_str());
    this->protocol_.set_infinity_mode(specs.infinity_mode);
  }

  this->specs_resolved_ = true;
  ESP_LOGI(TAG, "Specs resolved for '%s': CCT %.1f-%.1f mireds, %s, infinity=%d",
           corrected.c_str(), this->cct_min_mireds_, this->cct_max_mireds_,
           this->cct_only_ ? "CCT-only" : "RGB+CCT", this->protocol_.get_infinity_mode());
}

// --- Helper functions ---

uint8_t BLEewerLight::kelvin_to_neewer_temp_(uint16_t kelvin) {
  return static_cast<uint8_t>(kelvin / 100);
}

uint16_t BLEewerLight::mireds_to_kelvin_(float mireds) {
  return static_cast<uint16_t>(1000000.0f / mireds);
}

}  // namespace bleewer_light
}  // namespace esphome
