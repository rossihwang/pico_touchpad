// Copyright <2022> [Copyright rossihwang@gmail.com]

#include <pico/binary_info.h>

#include <gt911_i2c.hpp>
#include <cstring>

static ConfigData default_configs[] = {};

constexpr uint16_t kConfigStartReg = 0x8047;
static const uint8_t configs[] = {
  0x81,
  0x00,   // Resolution X axis: 1024
  0x04, 
  0x58,   // Resolution Y axis: 600
  0x02,
  0x05,   // 5 touch points 
  0x3D,   // interrupt on falling edge
  0x20, 
  0x22,   // Shake Count
  0x08, 
  0x28, 
  0x08,
  0x5F, 
  0x41, 
  0x03, 
  0x0F,   // Refresh Rate: (5 + N) ms, default 0x05
  0x00,   // x_threshold, default 0x00
  0x00,   // y_threshold, default 0x00
  0x00, 
  0x00, 
  0x00, 
  0x00, 
  0x00, 
  0x17,
  0x1A, 0x1D, 0x14, 0x88, 0x29, 0x08, 0x99, 0x9B, 0xB2, 0x04, 0x00, 0x00,
  0x00, 0x21, 0x01, 0x1D, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x82, 0xB4, 0x9E, 0xD5, 0xF4, 0x07, 0x00, 0x00, 0x04,
  0x89, 0x86, 0x00, 0x84, 0x8F, 0x00, 0x7F, 0x99, 0x00, 0x7B, 0xA3, 0x00,
  0x77, 0xAE, 0x00, 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x10, 0x12,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x04, 0x06, 0x08, 0x0F, 0x10, 0x12, 0x16, 0x18, 0x1C, 0x1D, 0x1E, 0x1F,
  0x20, 0x21, 0x22, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x53, 0x01
};

const uint16_t kProdIdStartReg = 0x8140;
const uint16_t kStatusReg = 0x814E;
const uint16_t kTouchPointsStartReg = 0x814F;

Gt911Touch::Gt911Touch(i2c_inst_t * i2c, uint8_t dev_addr)
    : i2c_(i2c),
               dev_addr_(dev_addr) {
  init_interface();
  reset_with_addr(dev_addr_);
  init_gt911();
}

void Gt911Touch::read_info(GtInfo* info) {
  uint8_t reg[2] = {static_cast<uint8_t>(kProdIdStartReg >> 8), static_cast<uint8_t>(kProdIdStartReg)};
  uint8_t data[11];
  i2c_write_blocking(i2c_, dev_addr_, reg, 2, true);
  i2c_read_blocking(i2c_, dev_addr_, data, 11, false);
  *info = *(GtInfo*)data; 
}

int Gt911Touch::read_touch_points(std::array<TouchPoint, 5>* points, uint8_t* status) {
  uint8_t status_data[3] = {static_cast<uint8_t>(kStatusReg >> 8), static_cast<uint8_t>(kStatusReg), 0x00};
  uint8_t data[40];
  i2c_write_blocking(i2c_, dev_addr_, status_data, 2, true);
  i2c_read_blocking(i2c_, dev_addr_, data, 1, false);
  bool buffer_status = data[0] & 0x80;
  int amount = 0;
  *status = data[0];

  if (buffer_status) {
    amount = data[0] & 0x0F;
    if (0 < amount && amount <= GT911_MAX_TPS) {
      uint8_t points_reg[2] = {static_cast<uint8_t>(kTouchPointsStartReg >> 8), static_cast<uint8_t>(kTouchPointsStartReg)};
      i2c_write_blocking(i2c_, dev_addr_, points_reg, 2, true);
      i2c_read_blocking(i2c_, dev_addr_, data, GT911_MAX_TPS * 8, false);  // FIXME: Good to read all points?
      for (int i = 0; i < amount; ++i) {
        (*points)[i].x = data[i * 8 + 2] << 8 | data[i * 8 + 1];
        (*points)[i].y = data[i * 8 + 4] << 8 | data[i * 8 + 3];
        (*points)[i].size = data[i * 8 + 6] << 8 | data[i * 8 + 5];
        (*points)[i].track_id = data[i * 8];
      }
    }
    i2c_write_blocking(i2c_, dev_addr_, status_data, 3, false);  // clear status register
  }

  return amount;
}

void Gt911Touch::init_interface() {
  i2c_init(i2c_, 325 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

  gpio_init(GT911_INT_PIN);
  gpio_set_dir(GT911_INT_PIN, GPIO_OUT);
  gpio_put(GT911_INT_PIN, 0);

  gpio_init(GT911_RST_PIN);
  gpio_set_dir(GT911_RST_PIN, GPIO_OUT);
  gpio_put(GT911_RST_PIN, 0);

}

void Gt911Touch::reset_with_addr(uint8_t addr) {
  assert(addr == 0x14 || addr == 0x5D);
  if (addr == 0x14) {
    gpio_put(GT911_INT_PIN, 1);
    sleep_us(200);  /// greater than 100us
    gpio_put(GT911_RST_PIN, 1);
    sleep_us(6000);  /// greater than 5ms
    gpio_set_dir(GT911_INT_PIN, GPIO_IN);
  } else if (addr == 0x5D) {
    sleep_us(200);  /// greater than 100us
    gpio_put(GT911_RST_PIN, 1);
    sleep_us(6000);  /// greater than 5ms
    gpio_set_dir(GT911_INT_PIN, GPIO_IN);
  }
  // gpio_pull_down(GT911_INT_PIN);
}

void Gt911Touch::init_gt911() {
  uint8_t reg[2] = {static_cast<uint8_t>(kConfigStartReg >> 8),
                     static_cast<uint8_t>(kConfigStartReg)};
  i2c_write_blocking(i2c_, dev_addr_, reg, 2, true);
  for (int i = 0; i < sizeof(configs) / sizeof(uint8_t); ++i) {
    i2c_write_blocking(i2c_, dev_addr_, &configs[i], 1, true);
  }
}
