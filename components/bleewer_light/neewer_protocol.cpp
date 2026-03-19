#include "neewer_protocol.h"
#include <cstring>
#include "esphome/core/log.h"

namespace esphome {
namespace bleewer_light {

static const char *TAG = "neewer_protocol";

// --- Light specs database (order matters: more specific names before less specific) ---

struct LightSpecEntry {
  const char *prefix;
  uint16_t cct_min_k;
  uint16_t cct_max_k;
  bool cct_only;
  uint8_t infinity_mode;
};

static const LightSpecEntry LIGHT_SPECS_DB[] = {
    {"Apollo",     5600, 5600,  true,  0},
    {"BH-30S RGB", 2500, 10000, false, 1},
    {"CB60 RGB",   2500, 6500,  false, 1},
    {"CL124",      2500, 10000, false, 2},
    {"GL1C",       2900, 7000,  false, 1},
    {"GL1",        2900, 7000,  true,  0},
    {"HB80C",      2500, 7500,  false, 1},
    {"MS60B",      2700, 6500,  true,  1},
    {"NL140",      3200, 5600,  true,  0},
    {"RGB C80",    2500, 10000, false, 1},
    {"RGB CB60",   2500, 10000, false, 1},
    {"RGB1200",    2500, 10000, false, 1},
    {"RGB1000",    2500, 10000, false, 1},
    {"RGB176 A1",  2500, 10000, false, 0},
    {"RGB176",     3200, 5600,  false, 0},
    {"RGB168",     2500, 8500,  false, 2},
    {"RGB140",     2500, 10000, false, 1},
    {"RGB18",      3200, 5600,  false, 0},
    {"RGB190",     3200, 5600,  false, 0},
    {"RGB1",       3200, 5600,  false, 1},
    {"RGB450",     3200, 5600,  false, 0},
    {"RGB480",     3200, 5600,  false, 0},
    {"RGB512",     2500, 10000, false, 1},
    {"RGB530PRO",  3200, 5600,  false, 0},
    {"RGB530",     3200, 5600,  false, 0},
    {"RGB650",     3200, 5600,  false, 0},
    {"RGB660PRO",  3200, 5600,  false, 0},
    {"RGB660",     3200, 5600,  false, 0},
    {"RGB800",     2500, 10000, false, 1},
    {"RGB960",     3200, 5600,  false, 0},
    {"RGB-P200",   3200, 5600,  false, 0},
    {"RGB-P280",   3200, 5600,  false, 0},
    {"SL70",       3200, 8500,  false, 0},
    {"SL80",       3200, 8500,  false, 0},
    {"SL90 Pro",   2500, 10000, false, 1},
    {"SL90",       2500, 10000, false, 1},
    {"SNL1320",    3200, 5600,  true,  0},
    {"SNL1920",    3200, 5600,  true,  0},
    {"SNL480",     3200, 5600,  true,  0},
    {"SNL530",     3200, 5600,  true,  0},
    {"SNL660",     3200, 5600,  true,  0},
    {"SNL960",     3200, 5600,  true,  0},
    {"SRP16",      3200, 5600,  true,  0},
    {"SRP18",      3200, 5600,  true,  0},
    {"TL60",       2500, 10000, false, 1},
    {"WRP18",      3200, 5600,  true,  0},
    {"ZK-RY",      5600, 5600,  false, 0},
    {"ZRP16",      3200, 5600,  true,  0},
};

static const size_t LIGHT_SPECS_DB_SIZE = sizeof(LIGHT_SPECS_DB) / sizeof(LIGHT_SPECS_DB[0]);

// --- Name correction map (device_id -> model name) ---

struct NameCorrectionEntry {
  const char *device_id;
  const char *model_name;
};

static const NameCorrectionEntry NAME_CORRECTIONS[] = {
    {"20200015", "RGB1"},
    {"20200037", "SL90"},
    {"20200049", "RGB1200"},
    {"20210006", "Apollo 150D"},
    {"20210007", "RGB C80"},
    {"20210012", "CB60 RGB"},
    {"20210018", "BH-30S RGB"},
    {"20210034", "MS60B"},
    {"20210035", "MS60C"},
    {"20210036", "TL60 RGB"},
    {"20210037", "CB200B"},
    {"20220014", "CB60B"},
    {"20220016", "PL60C"},
    {"20220035", "MS150B"},
    {"20220041", "AS600B"},
    {"20220043", "FS150B"},
    {"20220046", "RP19C"},
    {"20220051", "CB100C"},
    {"20220055", "CB300B"},
    {"20220057", "SL90 Pro"},
    {"20230021", "BH-30S RGB"},
    {"20230022", "HS60B"},
    {"20230025", "RGB1200"},
    {"20230031", "TL120C"},
    {"20230050", "FS230 5600K"},
    {"20230051", "FS230B"},
    {"20230052", "FS150 5600K"},
    {"20230064", "TL60 RGB"},
    {"20230080", "MS60C"},
    {"20230092", "RGB1200"},
    {"20230108", "HB80C"},
    {"20230042", "HS60C"},
};

static const size_t NAME_CORRECTIONS_SIZE = sizeof(NAME_CORRECTIONS) / sizeof(NAME_CORRECTIONS[0]);

// --- Checksum ---

void NeewerProtocol::append_checksum(std::vector<uint8_t> &packet) {
  uint8_t sum = 0;
  for (auto b : packet) {
    sum += b;
  }
  packet.push_back(sum & 0xFF);
}

// --- MAC address ---

void NeewerProtocol::set_mac_address(const uint8_t mac[6]) {
  memcpy(mac_, mac, 6);
}

// --- Infinity wrapper ---

std::vector<uint8_t> NeewerProtocol::wrap_infinity(uint8_t inf_cmd, uint8_t inner_cmd,
                                                    const std::vector<uint8_t> &params) {
  // [0x78, inf_cmd, length, MAC0..5, inner_cmd, ...params, checksum]
  // length = 6 (MAC) + 1 (inner_cmd) + params.size()
  uint8_t length = 6 + 1 + static_cast<uint8_t>(params.size());
  std::vector<uint8_t> packet;
  packet.reserve(3 + length + 1);  // header + cmd + len + payload + checksum
  packet.push_back(PACKET_HEADER);
  packet.push_back(inf_cmd);
  packet.push_back(length);
  for (int i = 0; i < 6; i++) {
    packet.push_back(mac_[i]);
  }
  packet.push_back(inner_cmd);
  for (auto b : params) {
    packet.push_back(b);
  }
  append_checksum(packet);

  ESP_LOGV(TAG, "Infinity packet (cmd=0x%02X, inner=0x%02X): %d bytes", inf_cmd, inner_cmd,
           packet.size());
  return packet;
}

// --- Command builders ---

std::vector<uint8_t> NeewerProtocol::build_power_command(bool on) {
  uint8_t on_off = on ? 0x01 : 0x02;

  if (infinity_mode_ == 1) {
    return wrap_infinity(CMD_INF_PWR, CMD_POWER, {on_off});
  }

  // Standard and hybrid use the same power command
  std::vector<uint8_t> packet = {PACKET_HEADER, CMD_POWER, 0x01, on_off};
  append_checksum(packet);

  ESP_LOGD(TAG, "Power %s: [%02X %02X %02X %02X %02X]", on ? "ON" : "OFF",
           packet[0], packet[1], packet[2], packet[3], packet[4]);
  return packet;
}

std::vector<uint8_t> NeewerProtocol::build_cct_command(uint8_t brightness, uint8_t temp,
                                                        int8_t gm) {
  if (infinity_mode_ == 1) {
    // Infinity: wrap with MAC, add GM+50 and trailing 0x04
    uint8_t gm_byte = static_cast<uint8_t>(gm + 50);
    return wrap_infinity(CMD_INF_CCT, CMD_CCT, {brightness, temp, gm_byte, 0x04});
  }

  if (infinity_mode_ == 2) {
    // Hybrid: 3 params (brightness, temp, GM+50)
    uint8_t gm_byte = static_cast<uint8_t>(gm + 50);
    std::vector<uint8_t> packet = {PACKET_HEADER, CMD_CCT, 0x03, brightness, temp, gm_byte};
    append_checksum(packet);

    ESP_LOGD(TAG, "CCT hybrid: bri=%d temp=%d gm=%d", brightness, temp, gm);
    return packet;
  }

  // Standard (infinity_mode 0): only brightness + temp, no GM byte
  // NeewerLite-Python truncates to currentSendValue[0:5] for mode 0
  std::vector<uint8_t> packet = {PACKET_HEADER, CMD_CCT, 0x02, brightness, temp};
  append_checksum(packet);

  ESP_LOGD(TAG, "CCT standard: bri=%d temp=%d", brightness, temp);
  return packet;
}

std::vector<uint8_t> NeewerProtocol::build_hsi_command(uint16_t hue, uint8_t saturation,
                                                        uint8_t brightness) {
  uint8_t hue_lo = hue & 0xFF;
  uint8_t hue_hi = (hue >> 8) & 0xFF;

  if (infinity_mode_ == 1) {
    return wrap_infinity(CMD_INF_HSI, CMD_HSI, {hue_lo, hue_hi, saturation, brightness});
  }

  // Standard and hybrid use the same HSI command
  std::vector<uint8_t> packet = {PACKET_HEADER, CMD_HSI, 0x04, hue_lo, hue_hi, saturation,
                                  brightness};
  append_checksum(packet);

  ESP_LOGD(TAG, "HSI: hue=%d sat=%d bri=%d", hue, saturation, brightness);
  return packet;
}

std::vector<uint8_t> NeewerProtocol::build_old_brightness_command(uint8_t brightness) {
  std::vector<uint8_t> packet = {PACKET_HEADER, CMD_BRI_OLD, 0x01, brightness};
  append_checksum(packet);

  ESP_LOGD(TAG, "Old brightness: %d", brightness);
  return packet;
}

std::vector<uint8_t> NeewerProtocol::build_old_temp_command(uint8_t temp) {
  std::vector<uint8_t> packet = {PACKET_HEADER, CMD_TEMP_OLD, 0x01, temp};
  append_checksum(packet);

  ESP_LOGD(TAG, "Old temp: %d", temp);
  return packet;
}

std::vector<uint8_t> NeewerProtocol::build_anm_command(uint8_t effect, const std::vector<uint8_t> &params) {
  if (infinity_mode_ == 1) {
    // Infinity: wrap with MAC
    std::vector<uint8_t> inner_params;
    inner_params.push_back(effect);
    inner_params.insert(inner_params.end(), params.begin(), params.end());
    return wrap_infinity(CMD_INF_ANM, CMD_INF_FX, inner_params);
  }

  // Standard (mode 0) and hybrid (mode 2): use 0x8B with full params.
  // Tested working on HS60C. Legacy 0x88 only works on very old lights.
  std::vector<uint8_t> packet = {PACKET_HEADER, CMD_INF_FX};
  packet.push_back(static_cast<uint8_t>(1 + params.size()));
  packet.push_back(effect);
  packet.insert(packet.end(), params.begin(), params.end());
  append_checksum(packet);

  ESP_LOGD(TAG, "ANM: effect=%d, %d params", effect, params.size());
  return packet;
}

// --- Light specs lookup ---

LightSpecs NeewerProtocol::get_light_specs(const std::string &ble_name) {
  // First apply name correction, then search specs DB
  std::string name = get_corrected_name(ble_name);

  for (size_t i = 0; i < LIGHT_SPECS_DB_SIZE; i++) {
    if (name.find(LIGHT_SPECS_DB[i].prefix) != std::string::npos) {
      LightSpecs specs;
      specs.name = LIGHT_SPECS_DB[i].prefix;
      specs.cct_min_k = LIGHT_SPECS_DB[i].cct_min_k;
      specs.cct_max_k = LIGHT_SPECS_DB[i].cct_max_k;
      specs.cct_only = LIGHT_SPECS_DB[i].cct_only;
      specs.infinity_mode = LIGHT_SPECS_DB[i].infinity_mode;

      ESP_LOGI(TAG, "Matched '%s' -> %s (CCT %d-%dK, %s, infinity=%d)",
               ble_name.c_str(), specs.name.c_str(), specs.cct_min_k, specs.cct_max_k,
               specs.cct_only ? "CCT-only" : "RGB+CCT", specs.infinity_mode);
      return specs;
    }
  }

  ESP_LOGW(TAG, "No specs match for '%s', using defaults (3200-5600K, RGB+CCT, standard)",
           ble_name.c_str());
  return DEFAULT_SPECS;
}

// --- Name correction ---

std::string NeewerProtocol::get_corrected_name(const std::string &ble_name) {
  // BLE names often contain a device_id substring that maps to a known model.
  // Check if any known device_id appears in the BLE name.
  for (size_t i = 0; i < NAME_CORRECTIONS_SIZE; i++) {
    if (ble_name.find(NAME_CORRECTIONS[i].device_id) != std::string::npos) {
      ESP_LOGD(TAG, "Name correction: '%s' -> '%s' (via device_id %s)",
               ble_name.c_str(), NAME_CORRECTIONS[i].model_name,
               NAME_CORRECTIONS[i].device_id);
      return NAME_CORRECTIONS[i].model_name;
    }
  }

  // No correction needed, return original name
  return ble_name;
}

}  // namespace bleewer_light
}  // namespace esphome
