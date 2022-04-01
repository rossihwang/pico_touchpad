// Copyright <2022> [Copyright rossihwang@gmail.com]
#pragma once

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <array>

#define GT911_INT_PIN 6
#define GT911_RST_PIN 3

#define GT911_MAX_TPS 5

struct ConfigData {
  uint16_t reg;
  uint8_t data;
};

struct TouchPoint {
  uint16_t x;
  uint16_t y;
  uint16_t size;
  uint8_t track_id;
};

struct __attribute__ ((packed)) GtInfo {
  char prod_id[4];
  uint16_t fw_version;
  uint16_t x_resolution;
  uint16_t y_resolution;
  uint8_t vend_id;
};

class Gt911Touch {
 private:
  i2c_inst_t* i2c_;
  uint8_t dev_addr_;

 public:
  Gt911Touch(i2c_inst_t* i2c, uint8_t dev_addr);
  void read_info(GtInfo* info);
  int read_touch_points(std::array<TouchPoint, 5>* points, uint8_t* status);

 private:
  void init_interface();
  /// Two address options: 0x14, 0x5D 
  void reset_with_addr(uint8_t addr);
  void init_gt911();
};