# DUPSExtender

**DUPSExtender** is an ESP32-based gateway project that bridges your **V-Guard DUPS (Digital UPS) Inverter** to your Home Automation system via **MQTT**. It connects to the inverter using Bluetooth Low Energy (BLE) and exposes its status and control features over WiFi.

## Features

-   **Real-time Monitoring**:
    -   Mains and Battery Voltage.
    -   Battery Charge/Discharge Percentage.
    -   Load Percentage and Current (Amps).
    -   Backup Time Remaining.
    -   System Temperature.
    -   Power Interruptions Log (Time since last backup).
-   **Remote Control**:
    -   Switch between **UPS** and **Normal** modes.
    -   Enable/Disable **High Power / Apprentice** mode.
    -   Enable/Disable **Turbo Charging**.
    -   Set **Regulator Level** (Stabilizer output).
    -   Force Cut-off timer (turn off output for X minutes).
-   **Integration**: Seamless integration with Home Assistant or any MQTT-compatible dashboard.

## System Architecture

```mermaid
graph LR
    Inverter[V-Guard DUPS Inverter] -- BLE --> ESP32[ESP32 Controller]
    ESP32 -- WiFi/MQTT --> MQTT_Broker[MQTT Broker]
    MQTT_Broker -- Subscribe --> HomeAssistant[Home Automation / Dashboard]
    HomeAssistant -- Publish --> MQTT_Broker
```

## Hardware Requirements

-   **ESP32 Development Board** (e.g., ESP32 DevKit V1).
-   **V-Guard Smart Inverter** (Supported models include those with "VG_SMART_BT" BLE name).

## Installation & Setup

1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/anup-raghavan/DUPSExtender.git
    cd DUPSExtender
    ```

2.  **Install PlatformIO**:
    This project is built using [PlatformIO](https://platformio.org/). Install the PlatformIO IDE extension for VS Code.

3.  **Configure Credentials**:
    Create a file named `secret.h` in the `include/` directory with your WiFi and MQTT credentials:

    ```cpp
    // include/secret.h
    #ifndef _SECRET_H
    #define _SECRET_H

    const char* ssid = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
    const char* mqtt_usr = "YOUR_MQTT_USERNAME";
    const char* mqtt_pwd = "YOUR_MQTT_PASSWORD";

    #endif
    ```

4.  **Battery Configuration**:
    Open `include/dups.h` and adjust the battery constants if your setup differs from a standard 150Ah Tubular battery:
    ```cpp
    #define BATTERY_CAPACITY_AH 150
    // ... other constants
    ```

5.  **Build and Upload**:
    Connect your ESP32 via USB and run:
    ```bash
    pio run -t upload
    ```

## MQTT API

The device communicates via the following topics.

### Telemetry (Read-Only)
Prefix: `dups/rd/`

| Topic | Description | Example Payload |
| :--- | :--- | :--- |
| `mv` | Mains Voltage | `230.50` |
| `bv` | Battery Voltage | `12.80` |
| `bp` | Battery Level | `100` (%) |
| `lp` | Load Percentage | `25` (%) |
| `bkm`| Backup Mode Active | `1` (Active) / `0` (Mains) |
| `bkt`| Est. Backup Time | Minutes |

### Control (Write)
Prefix: `dups/ctl/`

| Topic | Description | Payload |
| :--- | :--- | :--- |
| `ups` | Set UPS Mode | `1` (ON) / `0` (OFF) |
| `tch` | Turbo Charging | `1` (ON) / `0` (OFF) |
| `fcs` | Force Cut-off | `0`, `30`, `60`, `120` (Minutes) |

## Disclaimer
This is a personal project and is **not affiliated with or endorsed by V-Guard**. Use at your own risk. Improper use of hardware commands may damage your equipment.
