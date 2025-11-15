# Dual Bias Tester

A bias tester for tube amplifiers with octal power tubes (e.g. 6L6, 6V6, EL34, 5881). The tester can monitor two tubes at once, allowing you to adjust the bias and while seeing the bias difference between the two tubes to see how well-matched they are.

> [!CAUTION]
> Tube amps have high voltage current that can kill you, even when turned off. If you are uncomfortable working with high voltage circuits or taking the risks inherent to working on tube amplifiers, find a qualified tech to work on your amp. Wiring the tester or probes incorrectly is extremely dangerous, and can expose you to lethal current. Always think about what you are doing when working on amplifier circuits, always drain the filter capacitors after powering off the amp, and exercise caution at all times. Use this tester at your own risk.

## Hardware

The tester requires:

* 1 Arduino Nano
* 1 ADS1115 ADC board
* 1 128x64 SH1106 or SSD1306/1309 OLED display with I2C
* 3 momentary push buttons
* 6 banana sockets
* 2 octal bias probes that connect to the plate and cathode of the socket under test ([TubeDepot's Bias Scout Kit](https://www.tubedepot.com/products/tubedepot-bias-scout-kit) or equivalent, see section below). Note that the [Hoffman Amps Bias Checker](https://el34world.com/charts/BiasChecker3.htm) will not work, as that probe does not monitor the plate voltage, only the cathode current.

### DIY probes

The TubeDepot Bias Scout kit has everything you need, and is inexpensive at around $40 for two probes. If you want to make your own bias probe, for each probe you will need:

* 1 octal tube base (e.g. [P-SP8-47X from Amplified Parts](https://www.amplifiedparts.com/products/tube-base-8-pin-octal-125-diameter))
* 1 octal PCB socket (e.g. [P-ST8-810-PCL from Amplified Parts](https://www.amplifiedparts.com/products/socket-8-pin-octal-phenolic-pc-mount-long-lead))
* 1 1M 1/2 watt or greater 1% resistor
* 1 100 ohm 1/8 watt or greater 1% resistor
* 1 1 ohm 2 watt or greater 5% resistor
* 1 3 conductor wire
* 3 banana plugs

### Adapting for 9-pin tubes

The bias tester can, in principle, work to bias EL84 and other 9-pin power tubes. It just requires building a probe with the correct inputs for 9-pin tubes. 9-pin power tubes do not use the same pin arrangement as octal power tubes. That means creating a new nonal socket probe, or an adapter for an octal probe. Any nonal probe or adapter must connect to the correct voltage and current input jack and socket pins using the same component values and wiring as the octal probe.

### Wiring the hardware

The Nano, ADC, and OLED display all run on 5v DC, and the ADC and OLED communicate on the I2C bus. With I2C, multiple devices all communicate on the same bus, or wire, so there's only one connection to the Nano. You daisychain I2C devices in parallel on the data and clock wires. See the Adafruit

Nano
    A4 -> SDA (I2C data)
    A5 -> SCL (I2C clock)
    D5 -> button 1 (left)
    D6 -> button 2 (right)
    D7 -> button 3 (select)
    VIN -> VDD/VCC
    GND -> GND

ADS1115
    VDD -> VIN (5v DC)
    GND -> GND
    SDA -> SDA (I2C data)
    SCL -> SCL (I2C clock)
    A0 -> probe A voltage (red wire in Bias Scout)
    A1 -> probe A current (white wire in Bias Scout)
    A2 -> probe B voltage (red wire in Bias Scout)
    A3 -> probe B current (white wire in Bias Scout)

OLED display
    VCC -> VIN (5v DC)
    GND -> GND
    SDA -> SDA (I2C data)
    SCL -> SCL (I2C clock)

Probe A
    red plug -> probe A voltage socket
    white plug -> probe A current socket
    black plug -> probe A common socket

Probe B
    red plug -> probe B voltage socket
    white plug -> probe B current socket
    black plug -> probe B common socket

## Firmware

The firmware is in `src/main.cpp` and uses [PlatformIO](https://platformio.org/) to build and upload the compiled firmware to the Nano.

### Changing the display

By default, the firmware is configured for an SSD1306/1309 OLED display. If you use an SH1106 OLED display, find the `Display Initialization` section at the beginning of `main.cpp` and uncomment the configuration line for SH1106 and comment out the line for SSD1306/1309.

```cpp
// SH1106 OLED
U8X8_SH1106_128X64_NONAME_HW_I2C lcd(A5,A4,U8X8_PIN_NONE);
//
// SSD1306/1309 OLED
//U8X8_SSD1306_128X64_NONAME_HW_I2C lcd(A5, A4, U8X8_PIN_NONE);
```
