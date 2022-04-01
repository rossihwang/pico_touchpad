// Copyright <2022> [Copyright rossihwang@gmail.com]

#include <gt911_i2c.hpp>
#include <dma_uart.hpp>
#include <sstream>

static Gt911Touch touch(i2c_default, 0x5D);
static DmaUart uart(uart0, 115200);

void gt_callback(uint gpio, uint32_t events) {
  std::array<TouchPoint, 5> tps;
  uint8_t status;
  int tps_amount = touch.read_touch_points(&tps, &status);
  uart.write("amount: " + std::to_string(tps_amount) + " status: " + std::to_string(static_cast<int>(status)) + "\r\n");
  if (0 < tps_amount) {
    for (int i = 0; i < tps_amount; ++i) {
      std::stringstream ss;
      ss << "x: " << tps[i].x << " y: " << tps[i].y << " size: " << tps[i].size << " id: " << static_cast<int>(tps[i].track_id);
      uart.write(ss.str()+"\r\n");
    }
  }
  uart.flush();
}

int main() {

  uart.write("gt911 interrupt example\r\n");
  
  GtInfo info;
  touch.read_info(&info);
  std::stringstream ss;
  ss << "fw_version: " << static_cast<int>(info.fw_version) 
      << " x_res: " << static_cast<int>(info.x_resolution)
      << " y_res: " << static_cast<int>(info.y_resolution);
  uart.write(ss.str()+"\r\n");
  uart.flush();

  gpio_set_irq_enabled_with_callback(GT911_INT_PIN, GPIO_IRQ_EDGE_FALL, true, gt_callback);

  for (;;);

  return 0;
}