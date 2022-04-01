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



## Known issues
- Some Interference on the interrupt pin, when the program maintains idle for a while, which leads to the controller read the status register with 0.
