// Copyright <2022> [Copyright rossihwang@gmail.com]

#include <bsp/board.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tusb.h>
#include <usb_descriptors.h>

#include <dma_uart.hpp>
#include <gt911_i2c.hpp>

enum {
  BLINK_NOT_MOUNTED = 250,  /// ms
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
static Gt911Touch touch(i2c_default, 0x14);
static DmaUart uart(uart0, 115200);
static bool is_touch_updated = false;
static int8_t delta_x, delta_y;
static uint16_t last_x, last_y;

void led_blinking_task(void);
void hid_task(void);
void touch_callback(uint gpio, uint32_t events);

int main(void) {
  board_init();
  tusb_init();
  gpio_set_irq_enabled_with_callback(GT911_INT_PIN, GPIO_IRQ_EDGE_FALL, true,
                                     touch_callback);

  while (1) {
    tud_task();
    led_blinking_task();

    hid_task();
  }

  return 0;
}

/// Device callbacks

void tud_mount_cb(void) { 
  uart.write("USB mounted\r\n");
  uart.flush();
  blink_interval_ms = BLINK_MOUNTED;
}

void tud_umount_cb(void) { 
  uart.write("USB unmounted\r\n");
  uart.flush();
  blink_interval_ms = BLINK_NOT_MOUNTED; 
}

void tud_suspend_cb(bool remote_wakup_en) {
  (void)remote_wakup_en;
  uart.write("USB suspended\r\n");
  uart.flush();
  blink_interval_ms = BLINK_SUSPENDED;
}

void tud_resume_cb(void) { 
  uart.write("USB resumed\r\n");
  uart.flush();
  blink_interval_ms = BLINK_MOUNTED;
}

/// USB HID
static void send_hid_report(uint8_t report_id) {
  if (!tud_hid_ready()) {
    return;
  }

  switch (report_id) {
    case REPORT_ID_KEYBOARD:
      break;
    case REPORT_ID_MOUSE: {
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta_x, delta_y, 0, 0);
    } break;
    case REPORT_ID_CONSUMER_CONTROL:
      break;
    case REPORT_ID_GAMEPAD:
      break;
    default:
      break;
  }
}

void hid_task(void) {
  /// Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms) {
    return;
  }
  start_ms += interval_ms;

  // Remote wakeup
  if (tud_suspended() && is_touch_updated) {
    tud_remote_wakeup();
  } else {
    send_hid_report(REPORT_ID_MOUSE);
  }
  is_touch_updated = false;
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report,
                                uint8_t len) {
  (void)instance;
  (void)len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize) {
  (void)instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT) {
    if (report_id == REPORT_ID_KEYBOARD) {
      if (bufsize < 1) {
        return;
      }
      /// TODO
    }
  }
}

void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;

  if (!blink_interval_ms) return;

  if (board_millis() - start_ms < blink_interval_ms) return;
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state;  /// toggle
}

void touch_callback(uint gpio, uint32_t events) {
  std::array<TouchPoint, 5> tps;
  uint8_t status;
  int tps_amount = touch.read_touch_points(&tps, &status);
  
  if (0 < tps_amount) {
    uart.write("amount: " + std::to_string(tps_amount) + "\r\n");
    uart.flush();
    delta_x = tps[0].x - last_x;
    delta_y = tps[0].y - last_y;
    last_x = tps[0].x;
    last_y = tps[0].y;
  } else {
    delta_x = 0;
    delta_y = 0;
    last_x = 0;
    last_y = 0;
  }
  
  if (last_x != 0 || last_y != 0) {
    is_touch_updated = true;
  }
}