# ESP8266 IoT Remote Buzzer Alarm
**By [Idlan Zafran Mohd Zaidie](https://github.com/IdlanZafran)**

[![Follow](https://img.shields.io/github/followers/IdlanZafran?label=Follow%20Me&style=social)](https://github.com/IdlanZafran)

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/IdlanZafran/esp8266-iot-remote-buzzer-alarm)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

_If you like this project, don’t forget to:_
- _⭐ Star this repository_
- _👤 Follow my GitHub for future updates_

## 📌 Overview
A smart, cloud-connected alarm system built for the ESP8266. This project monitors remote sensor values against customizable thresholds using the ThingsSentral IoT platform and triggers a local physical buzzer and LED sequence when conditions are met. It also features a built-in battery monitoring system and a robust Wi-Fi configuration portal.

## Features

- **Cloud-Synced Alarms:** Automatically triggers when remote sensor data exceeds predefined conditions, or can be triggered manually via the ThingsSentral dashboard.
- **Battery Monitoring System:** Reads external battery voltage via a voltage divider and visually indicates battery health using 3 LEDs. Automatically mutes the buzzer if the battery reaches a critical level to conserve power.
- **Smart Wi-Fi Provisioning:** Utilizes `RapidBootWiFi` to easily configure network credentials and IoT device IDs via a captive web portal, avoiding hardcoded Wi-Fi passwords.
- **Offline Fallback:** Features a visual LED "heartbeat" to let you know the device is functioning even if it loses connection to the server.
- **Non-Blocking Logic:** Uses `Ticker.h` for asynchronous background tasks like buzzer toggling and status animations.

## Hardware Requirements

- ESP8266 Development Board (e.g., NodeMCU or Wemos D1 Mini)
- 1x Active Buzzer
- 3x LEDs (Preferably different colors for battery levels)
- Resistors for LEDs (e.g., 220Ω or 330Ω)
- Resistors for Voltage Divider: 1x 10kΩ and 1x 4.7kΩ
- Power Source (e.g., 9V Battery or 3x Li-Po cells)

### Pin Mapping

| Component       | ESP8266 Pin        | Notes                                      |
| :-------------- | :----------------- | :----------------------------------------- |
| LED 1 (Low)     | `GPIO 14` (D5)     | Blinks fast when battery is critical       |
| LED 2 (Mid)     | `GPIO 12` (D6)     |                                            |
| LED 3 (High)    | `GPIO 13` (D7)     |                                            |
| Buzzer          | `GPIO 5` (D1)      | Active Low configuration                   |
| Battery Monitor | `A0`               | Connected via 10kΩ / 4.7kΩ voltage divider |
| Setup Button    | `GPIO 0` (D3/FLASH)| Hold during boot to open Wi-Fi portal      |

## Software Dependencies

To compile this project, you will need the Arduino IDE or PlatformIO with the ESP8266 core installed, along with the following libraries:

- `Ticker.h` (Built-in with ESP8266 core)
- `RapidBootWiFi`
- `ThingsSentral`

## Installation & Setup

1. **Clone the Repository**

```
   git clone https://github.com/YOUR_USERNAME/esp8266-iot-remote-buzzer-alarm.git
```
   2. **Security Note: Configure Credentials**
   By default, the code uses placeholder values for the ThingsSentral IDs. 
   **Do not commit your real IDs to GitHub.** When flashing to your device, replace the placeholders in the `Config` namespace with your actual credentials, or input them via the Wi-Fi Setup Portal.

   ```cpp
   char userID[16]      = "YOUR_USER_ID";
   char sensorID[32]    = "YOUR_SENSOR_ID";   
   char buzzerID[32]    = "YOUR_BUZZER_ID";
   char conditionID[32] = "YOUR_CONDITION_ID";
   ```

3. **Upload to ESP8266**
   Compile and upload the code to your board.

4. **Wi-Fi & Device Configuration**
   * Upon first boot, the device will fail to connect to Wi-Fi and broadcast an Access Point named `Remote_Buzzer_Alarm`.
   * Connect to this AP using your phone or computer.
   * A captive portal will appear. Enter your local Wi-Fi SSID, Password, and your ThingsSentral IDs.
   * Click Save. The device will reboot and connect to the cloud.
   * *Note:* You can force this portal to open at any time by holding down the `BOOT/FLASH` button (Pin 0) while powering on the device.

## Battery LED Behavior

| State | Voltage | LED Indicator |
| :--- | :--- | :--- |
| **Full** | ≥ 9.6V | 3 LEDs ON |
| **Medium** | 9.3V - 9.59V | 2 LEDs ON |
| **Low** | 9.1V - 9.29V | 1 LED ON |
| **Critical** | < 9.1V | 1 LED blinking fast (Buzzer is muted) |

## License
This project is licensed under the MIT License.
