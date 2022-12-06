# pico_touchpad

## Modules
- 6.2 inch capacitive touch board with GT911 controller, support up to 5 touch points
- Raspberry Pi Pico with tinyusb



## Wiring

| Pico Pin  | Func      |
| ---- | --------- |
| GP4  | I2C0_SDA  |
| GP5  | I2C0_SCL  |
| GP0  | UART0_RX  |
| GP2  | GT911_INT |
| GP3  | GT911_RST |



## Support

- Single click to left button click
- Double click to drag
- One finger press with another click to right button click
- Swipe to move the wheel



## Known issues
- Some Interference on the interrupt pin, when the program maintains idle for a while, which leads to the controller read the status register with 0.


```
typedef struct TU_ATTR_PACKED
{
  uint8_t buttons; /**< buttons mask for currently pressed buttons in the mouse. */
  int8_t  x;       /**< Current delta x movement of the mouse. */
  int8_t  y;       /**< Current delta y movement on the mouse. */
  int8_t  wheel;   /**< Current delta wheel movement on the mouse. */
  int8_t  pan;     // using AC Pan
} hid_mouse_report_t;

/// Standard Mouse Buttons Bitmap
typedef enum
{
  MOUSE_BUTTON_LEFT     = TU_BIT(0), ///< Left button
  MOUSE_BUTTON_RIGHT    = TU_BIT(1), ///< Right button
  MOUSE_BUTTON_MIDDLE   = TU_BIT(2), ///< Middle button
  MOUSE_BUTTON_BACKWARD = TU_BIT(3), ///< Backward button,
  MOUSE_BUTTON_FORWARD  = TU_BIT(4), ///< Forward button,
}hid_mouse_button_bm_t;
```