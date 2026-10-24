# Smart Student ID Card

An STM32F401-based campus terminal that uses one RFID card for attendance,
cashless mess payment, heart-rate monitoring and emergency SOS alerts,
with a Flask + SQLite server and live web dashboard.

Course project – 23CSE304 Embedded Systems, Amrita Vishwa Vidyapeetham, Coimbatore.

![System design](docs/system_design.png)

## Features

- **Attendance** – tap an RFID card; logged with an RTC timestamp
- **Mess payment** – Rs 50 per meal, balance checked and deducted on the server
- **Online recharge** – from the web dashboard (simulated, no payment gateway)
- **Heart rate** – MAX30102, shown on the LCD and logged every 10 s
- **SOS** – one button: 1 press switches mode, 3 presses send an SOS alert
- **Library noise and restricted-area alerts** – KY-037 and HC-SR04

## Hardware

| Component                     | Interface      | Pins         |
| ----------------------------- | -------------- | ------------ |
| STM32F401 Black Pill          | —              | —            |
| RC522 RFID                    | SPI1           | PA4–PA7, PB0 |
| 16×2 LCD (I2C)                | I2C1, 0x27     | PB6, PB7     |
| DS3231 RTC                    | I2C1, 0x68     | PB6, PB7     |
| MAX30102                      | I2C1, 0x57     | PB6, PB7     |
| KY-037 sound sensor           | ADC1           | PA2          |
| HC-SR04 ultrasonic            | GPIO + TIM2    | PB12, PB13   |
| Push button                   | EXTI9          | PB9          |
| Piezo buzzer                  | TIM3 PWM       | PB4          |
| ESP8266 NodeMCU (AT firmware) | USART1, 115200 | PA9, PA10    |

## Firmware

- Register-level C (no HAL), Keil µVision 5, Arm Compiler 6
- 84 MHz clock from 25 MHz crystal via PLL
- Super-loop scheduler with three interrupts: USART1 RX (ring buffer), EXTI9 (button), SysTick (1 ms)
- Built-in component tests: set `TEST_COMPONENT` 1–12 to test one module at a time

### Build and flash

1. Open `firmware/es_project.uvprojx` in Keil µVision 5
2. Set `WIFI_SSID`, `WIFI_PASS` and `SERVER_IP` at the top of `es_project_final.c`
3. Build (F7) and flash with an ST-Link (F8)

## Server

```bash
cd server
pip install -r requirements.txt
python server.py
```

Open http://localhost:5000 for the dashboard. The laptop and the ESP8266
must be on the same 2.4 GHz network, and TCP port 5000 must be allowed
through the firewall.

| Endpoint                               | Purpose                                     |
| -------------------------------------- | ------------------------------------------- |
| POST /api/attendance                   | Log attendance (404 if card not registered) |
| POST /api/mess                         | Deduct meal cost (402 if balance too low)   |
| POST /api/health, /api/noise, /api/sos | Log sensor events                           |
| POST /api/students, /api/recharge      | Dashboard: register, recharge               |

## Limitations

- Recharge is simulated – no payment gateway
- No login or HTTPS on the server
- Taps are not stored if Wi-Fi is down
- Heart rate is basic monitoring, not a medical measurement

## Wiring

All connections are to an STM32F401 "Black Pill". Pin labels match the board's silkscreen (A4 = PA4, B6 = PB6, etc.).

### Power rails

| Breadboard rail | Connect to         | Powers                                                      |
| --------------- | ------------------ | ----------------------------------------------------------- |
| 5V              | Black Pill **5V**  | LCD, HC-SR04                                                |
| 3V3             | Black Pill **3V3** | RC522, DS3231, MAX30102, KY-037, push button, buzzer module |
| GND             | Black Pill **G**   | All modules, **including the NodeMCU GND**                  |

> The 5V pin is only powered when the Black Pill's USB-C cable is connected.
> On long breadboards, bridge the split power rails in the middle.

### Programming (ST-Link V2)

| ST-Link V2 | Black Pill (4-pin SWD header)            |
| ---------- | ---------------------------------------- |
| SWDIO      | DIO                                      |
| SWCLK      | SCK                                      |
| GND        | GND                                      |
| 3.3V       | not connected (board powered from USB-C) |

### RC522 RFID reader (SPI1) – 3.3 V only

| RC522    | Black Pill    |
| -------- | ------------- |
| SDA (CS) | A4            |
| SCK      | A5            |
| MOSI     | A7            |
| MISO     | A6            |
| IRQ      | not connected |
| GND      | G             |
| RST      | B0            |
| 3.3V     | 3V3           |

### 16×2 LCD with I2C backpack (I2C1, address 0x27)

| LCD backpack | Black Pill |
| ------------ | ---------- |
| GND          | G          |
| VCC          | 5V         |
| SDA          | B7         |
| SCL          | B6         |

### DS3231 RTC – ZS-042 module (I2C1, address 0x68)

| DS3231   | Black Pill    |
| -------- | ------------- |
| VCC      | 3V3           |
| GND      | G             |
| SDA      | B7            |
| SCL      | B6            |
| SQW, 32K | not connected |

> Bridge the **A0** solder pads on the ZS-042 board. This moves its on-board
> EEPROM from 0x57 to 0x56 so it does not clash with the MAX30102.

### MAX30102 heart-rate sensor (I2C1, address 0x57)

| MAX30102 | Black Pill    |
| -------- | ------------- |
| VIN      | 3V3           |
| GND      | G             |
| SDA      | B7            |
| SCL      | B6            |
| INT      | not connected |

### KY-037 sound sensor (ADC1)

| KY-037 | Black Pill    |
| ------ | ------------- |
| +      | 3V3           |
| G      | G             |
| AO     | A2            |
| DO     | not connected |

### HC-SR04 ultrasonic sensor (GPIO + TIM2)

| HC-SR04 | Black Pill |
| ------- | ---------- |
| VCC     | 5V         |
| Trig    | B12        |
| Echo    | B13        |
| GND     | G          |

### ESP8266 NodeMCU – AT firmware (USART1, 115200 baud)

| NodeMCU   | Black Pill                 |
| --------- | -------------------------- |
| TX        | A10 (USART1 RX)            |
| RX        | A9 (USART1 TX)             |
| GND       | G (common ground required) |
| micro-USB | laptop (power only)        |

> TX and RX are crossed. Close any serial monitor open on the NodeMCU's COM port.

### Push button – Mode / SOS (EXTI9)

| Button module (G-V-S) | Black Pill |
| --------------------- | ---------- |
| G                     | G          |
| V                     | 3V3        |
| S                     | B9         |

One press switches between Attendance and Mess mode; three quick presses send an SOS.
The firmware detects whether the module is active-high or active-low at start-up
(do not hold the button while the board boots).

### Piezo buzzer (TIM3 PWM, 2.7 kHz)

| Buzzer                  | Black Pill |
| ----------------------- | ---------- |
| + (or module I/O)       | B4         |
| − (or GND)              | G          |
| VCC (3-pin module only) | 3V3        |

### External reset button (optional)

| Button module | Black Pill    |
| ------------- | ------------- |
| S             | RST           |
| V             | G             |
| G             | not connected |

### Pin summary

| Pin | Function                 | Pin | Function                   |
| --- | ------------------------ | --- | -------------------------- |
| A2  | KY-037 AO (ADC1)         | B0  | RC522 RST                  |
| A4  | RC522 SDA/CS             | B4  | Buzzer (TIM3 PWM)          |
| A5  | RC522 SCK (SPI1)         | B6  | I2C1 SCL                   |
| A6  | RC522 MISO (SPI1)        | B7  | I2C1 SDA                   |
| A7  | RC522 MOSI (SPI1)        | B9  | Button (EXTI9)             |
| A9  | ESP8266 RX (USART1 TX)   | B12 | HC-SR04 Trig               |
| A10 | ESP8266 TX (USART1 RX)   | B13 | HC-SR04 Echo (via divider) |
| C13 | On-board LED (heartbeat) |     |                            |

### Expected I2C scan

With LCD, RTC and MAX30102 connected, component test 1 (`TEST_COMPONENT 1`)
should report: **27 56 57 68**.
