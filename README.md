# AirQ IoT Sensor Node

## Description
This directory contains the firmware code for the physical IoT hardware node used in the AirQ system. Designed primarily for microcontrollers like the ESP32 or ESP8266, this code reads physical environmental data from connected sensors and transmits it via Wi-Fi.

## Key Features
- **Hardware Integration**: Interfaces directly with physical sensors (e.g., MQ-135 for CO2, DHT11/22 for Temperature and Humidity, PM2.5 dust sensors).
- **Wi-Fi Connectivity**: Connects to the local network to transmit data to the Edge Layer or directly to the Cloud.
- **Low-Level Processing**: Handles analog-to-digital conversions and base calibrations for raw electrical signals.
- **JSON Serialization**: Packages hardware readings into structured JSON objects for standard transmission.

## Tech Stack
- C / C++
- Arduino IDE / PlatformIO framework
- Libraries for specific hardware sensors (e.g., DHT sensor library, WiFi library)

## Deployment to Hardware
1. Open the `.ino` or `.cpp` project file in Arduino IDE or VSCode with PlatformIO.
2. Select the correct board model (e.g., NodeMCU, ESP32 Dev Module).
3. Update the code with the target Wi-Fi credentials (`SSID` and `PASSWORD`).
4. Connect the microcontroller via USB and click **Upload**.
5. Open the Serial Monitor at the correct baud rate (typically 9600 or 115200) to verify sensor readings and network connection.
