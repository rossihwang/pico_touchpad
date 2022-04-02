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
static uint8_t buttons;
static uint16_t last_x, last_y;

void led_blinking_task(void);
void hid_task(void);
void mouse_task();

int main(void) {
  board_init();
  tusb_init();

  while (1) {
    tud_task();
    led_blinking_task();

    hid_task();
    mouse_task();
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
      tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, delta_x, delta_y, 0, 0);
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


void mouse_task() {
  static uint8_t state;
  const uint8_t kStInit = 0;
  const uint8_t kStIdle = 1;
  const uint8_t kStSingleClick = 2;
  const uint8_t kStSingleClickRelease = 3;
  const uint8_t kStDoubleClick = 4;
  const uint8_t kStRelease = 5;
  static uint8_t press_counter;
  static uint8_t release_counter;

  std::array<TouchPoint, 5> tps;
  uint8_t status;

  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;
  static uint32_t last_press_stamp;

  if (board_millis() - start_ms < interval_ms) {
    return;
  }
  start_ms += interval_ms;
  int tps_amount = touch.read_touch_points(&tps, &status);

  last_press_stamp += interval_ms;
  if (5000 < last_press_stamp) {
    last_press_stamp = 5000;
  }

  switch (state) {
    case kStInit:
    {
      state = kStRelease;
    }
    break;
    case kStIdle:
    {
      press_counter = 0;
      release_counter = 0;
      state = kStRelease;
    }
    break;
    case kStSingleClick:
    {
      if (0 == tps_amount) {
        release_counter++;
        delta_x = 0;
        delta_y = 0;
        if (1 < release_counter) {
          if (20 < last_press_stamp && last_press_stamp < 200) {
            buttons |= MOUSE_BUTTON_LEFT;
            is_touch_updated = true;
            state = kStSingleClickRelease;
            uart.write("left single click(" + std::to_string(last_press_stamp) + "\r\n");
            uart.flush();
          } else {
            uart.write("moving release(" + std::to_string(last_press_stamp) + ")\r\n");
            uart.flush();
            state = kStIdle;
          }
        }
      } else {
        release_counter = 0;
        delta_x = tps[0].x - last_x;
        delta_y = tps[0].y - last_y;
        last_x = tps[0].x;
        last_y = tps[0].y;
        is_touch_updated = true;
      }
    }
    break;
    case kStSingleClickRelease:
    {
      buttons &= ~MOUSE_BUTTON_LEFT;
      is_touch_updated = true;
      state = kStIdle;
      uart.write("left single click release\r\n");
      uart.flush();
    }
    break;
    case kStDoubleClick:
    {
      if (0 == tps_amount) {
        release_counter++;
        if (1 < release_counter) {
          state = kStIdle;
          delta_x = 0;  // Avoid sliding after release
          delta_y = 0;
          buttons &= ~MOUSE_BUTTON_LEFT;
          is_touch_updated = true;
          uart.write("left double click release\r\n");
          uart.flush();
        }
      } else {
        release_counter = 0;
        delta_x = tps[0].x - last_x;
        delta_y = tps[0].y - last_y;
        last_x = tps[0].x;
        last_y = tps[0].y;
        is_touch_updated = true;
      }
    }
    break;
    case kStRelease:
    {
      if (0 < tps_amount) {
        press_counter++;
        if (1 < press_counter) {
          if (100 < last_press_stamp && last_press_stamp < 500) {
            buttons |= MOUSE_BUTTON_LEFT;
            is_touch_updated = true;
            uart.write("left double click(" + std::to_string(last_press_stamp) + ")\r\n");
            uart.flush();
            last_press_stamp = 0;
            state = kStDoubleClick;
          } else {
            last_x = tps[0].x;
            last_y = tps[0].y;
            last_press_stamp = 0;
            state = kStSingleClick;
            uart.write("moving\r\n");
            uart.flush();
          }
        }
      } else {
        press_counter = 0;
      }
    }
    break;
    default:
    break;
  }
}