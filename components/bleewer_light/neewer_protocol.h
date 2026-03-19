#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace bleewer_light {

// Protocol constants
static const uint8_t PACKET_HEADER = 0x78;
static const uint8_t CMD_POWER     = 0x81;
static const uint8_t CMD_BRI_OLD   = 0x82;
static const uint8_t CMD_TEMP_OLD  = 0x83;
static const uint8_t CMD_HSI       = 0x86;
static const uint8_t CMD_CCT       = 0x87;
static const uint8_t CMD_ANM       = 0x88;
static const uint8_t CMD_INF_FX    = 0x8B;
static const uint8_t CMD_INF_PWR   = 0x8D;
static const uint8_t CMD_INF_HSI   = 0x8F;
static const uint8_t CMD_INF_CCT   = 0x90;
static const uint8_t CMD_INF_ANM   = 0x91;

// Light capabilities derived from model name
struct LightSpecs {
  std::string name;        // Matched model name from DB
  uint16_t cct_min_k;      // e.g. 3200
  uint16_t cct_max_k;      // e.g. 5600
  bool cct_only;           // true = no RGB/HSI support
  uint8_t infinity_mode;   // 0=standard, 1=infinity, 2=hybrid
};

// Default specs for unknown lights
static const LightSpecs DEFAULT_SPECS = {"", 3200, 5600, false, 0};

class NeewerProtocol {
 public:
  void set_infinity_mode(uint8_t mode) { infinity_mode_ = mode; }
  uint8_t get_infinity_mode() const { return infinity_mode_; }
  void set_mac_address(const uint8_t mac[6]);

  std::vector<uint8_t> build_power_command(bool on);
  std::vector<uint8_t> build_cct_command(uint8_t brightness, uint8_t temp, int8_t gm = 0);
  std::vector<uint8_t> build_hsi_command(uint16_t hue, uint8_t saturation, uint8_t brightness);
  std::vector<uint8_t> build_old_brightness_command(uint8_t brightness);
  std::vector<uint8_t> build_old_temp_command(uint8_t temp);
  std::vector<uint8_t> build_anm_command(uint8_t effect, const std::vector<uint8_t> &params);

  static LightSpecs get_light_specs(const std::string &ble_name);
  static std::string get_corrected_name(const std::string &ble_name);

 private:
  uint8_t infinity_mode_ = 0;
  uint8_t mac_[6] = {0};
  static void append_checksum(std::vector<uint8_t> &packet);
  std::vector<uint8_t> wrap_infinity(uint8_t inf_cmd, uint8_t inner_cmd,
                                     const std::vector<uint8_t> &params);
};

}  // namespace bleewer_light
}  // namespace esphome
