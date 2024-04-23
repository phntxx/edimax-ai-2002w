# Hacking the Edimax Edigreen Home AI-2002W to work with Home Assistant

The Edimax Edigreen Home is a decent air quality monitor.
However, by default, this thing pushes its air quality measurements directly to the cloud,
instead of only broadcasting them on the network.
While this has some convenience advantages, it also has the massive disadvantage of violating
the end-users privacy.

## Hardware

The AI-2002W consists of a Realtek RTL8195A SOC, which provides Wi-Fi connectivity and is the
brains of the operation.
It is then paired with a Plantower PMS5003 Air Quality Sensor, as well as a PCB containing
a MICS-VZ-89TE CO2 sensor and a SHT30-DIS temperature and humidity sensor.

Communication between the main board and the PMS5003 is done via UART,
while communication with the secondary PCB is done via I2C.

The front LEDs are controlled with the help of a TLC59116F 16-Channel I2C LED driver chip.

## Software

The Realtek RTL8195A can be flashed using the Arduino Firmware, as outlined
[here](https://www.amebaiot.com/en/ameba-arduino-getting-started/).

However, during my trials and tribulations, I could not get the I2C communication with
the secondary board to work. My conclusion at this point is that the I2C communication
is wrongly implemented, with no hope that it'll ever be updated to a suitable version, as
the board is already a couple years old at this point.

## Replacing the main board

In the end, I decided to replace the main board with a Wemos D1 mini. This has a number of
benefits, most notably:

- Working I2C implementation
- Decent community support
- Lower power draw (5v microUSB connection vs 12v barrel-jack PSU)

## Reverse-engineering the sensor board

With the help of a multimeter and data sheets for the onboard sensors, I was able to figure out
the connections on the secondary board. With the board laid flat and the connector on the left
side, the connections are as follows:

```
- +3.3V
- SDA
- SCL
- GND
- GND
```

## Wiring up the PMS5003

With the help of the data sheet of the PMS5003, I was able to determine the connections of its
connector (assuming the sensor is laid flat, with the four inlets at the bottom) to be:

```
- +5V
- GND
- SET
- RX
- TX
- RESET
- NC
- NC
```

## Wiring

With this information in hand, the two boards could be connected to the Wemos D1 mini as follows:

- Secondary board:
  - `+3.3V` => `+3.3V` on Wemos
  - `SDA` => `D2` on Wemos
  - `SCL` => `D1` on Wemos
  - `GND` => `GND` on Wemos
- PMS5003:
  - `+5V` => `+5V` on Wemos
  - `GND` => `GND` on Wemos
  - `SET` => `D3` on Wemos
  - `RX` => `D6` on Wemos
  - `TX` => `D7` on Wemos

## Software

I then wrote an Arduino sketch for the Wemos D1 mini that transmits all available
sensor data to a given broker via MQTT.

To install this Arduino sketch on your Wemos D1 mini, first add the ESP8266 series of boards to your Arduino IDE by following the
instructions listed [here](https://github.com/esp8266/Arduino?tab=readme-ov-file#installing-with-boards-manager).

Then, simply clone this repository, open `code.ino` in your Arduino IDE, change the variables from line `11` to line `23` to your
preference and click "Upload".

# References

- [FCC Report: Edimax Edigreen Home AI-2002 - Internal Photos](https://fcc.report/FCC-ID/ndd9520021801/3799812.pdf)
- [MICS-VZ-89TE Datasheet](https://www.sgxsensortech.com/content/uploads/2023/04/DS-0512-MiCS-VZ-89TE.pdf)
- [SHT30-DIS Datasheet](https://www.mouser.com/datasheet/2/682/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital-971521.pdf)
- [PMS5003 Datasheet](https://www.digikey.jp/htmldatasheets/production/2903006/0/0/1/PMS5003-Series-Manual.pdf)
