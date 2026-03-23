#pragma once
#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/light/light_output.h"
#include "neewer_protocol.h"

namespace esphome {
namespace bleewer_light {

class BLEewerLight : public Component, public ble_client::BLEClientNode, public light::LightOutput {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;

  // LightOutput interface
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;
  void setup_state(light::LightState *state) override { this->light_state_ = state; }

  // BLEClientNode interface
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  // Config setters (called from Python codegen)
  void set_cct_min_mireds(float mireds) { cct_min_mireds_ = mireds; }
  void set_cct_max_mireds(float mireds) { cct_max_mireds_ = mireds; }
  void set_cct_only(bool cct_only) { cct_only_ = cct_only; }
  void set_infinity_mode(uint8_t mode) { protocol_.set_infinity_mode(mode); }
  void set_model_name(const std::string &name) { model_name_ = name; }

  // Public access for effects
  void queue_command(const std::vector<uint8_t> &data) { this->queue_command_(data); }
  NeewerProtocol *get_protocol() { return &this->protocol_; }

 protected:
  uint16_t write_handle_{0};
  uint16_t notify_handle_{0};
  bool connected_{false};

  float cct_min_mireds_{0};  // 0 = auto-detect
  float cct_max_mireds_{0};
  bool cct_only_{false};
  bool specs_resolved_{false};
  std::string model_name_;

  // Command queue with rate limiting
  uint32_t last_write_time_{0};
  static const uint32_t MIN_WRITE_INTERVAL_MS = 50;
  std::vector<std::vector<uint8_t>> pending_commands_;

  NeewerProtocol protocol_;
  light::LightState *light_state_{nullptr};
  bool power_state_{false};

  // Status reading
  bool status_queried_{false};

  void send_command_(const std::vector<uint8_t> &data);
  void queue_command_(const std::vector<uint8_t> &data);
  void flush_pending_commands_();
  void resolve_specs_from_name_(const std::string &name);
  void query_initial_status_();
  void handle_notification_(uint8_t *data, uint16_t length);
  static uint8_t kelvin_to_neewer_temp_(uint16_t kelvin);
  static uint16_t mireds_to_kelvin_(float mireds);
};

}  // namespace bleewer_light
}  // namespace esphome
