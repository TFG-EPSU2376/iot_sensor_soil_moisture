# Soil Moisture Sensor with ESP32

This project implements a soil moisture sensor using the ESP32 platform. The device measures soil moisture levels and sends the data to an OpenThread Leader Gateway. It is designed to be part of an IoT network where multiple sensors communicate via the OpenThread protocol.

## Features

- Measures soil moisture using a capacitive sensor
- Connects to an OpenThread network as a child node
- Sends moisture data to the OpenThread Leader Gateway
- Low power consumption mode for extended battery life

## Hardware Requirements

- ESP32 Development Board
- Capacitive Soil Moisture Sensor
- OpenThread-capable devices (e.g., nRF52840)

## Software Requirements

- ESP-IDF Framework
- OpenThread Stack
- Configured OpenThread network settings

## Installation

1. Clone the repository:

   ```sh
   git clone https://github.com/yourusername/esp32_soil_moisture_sensor
   cd esp32_soil_moisture_sensor

   ```

2. Set up the ESP-IDF environment:
   Follow the instructions on the ESP-IDF documentation to set up your ESP32 development environment.

3. Initialize and update the OpenThread submodule:
   git submodule update --init --recursive

4. Configure the project, especially the sdkconfig file for OpenThread and sensor settings.

5. Build and flash the project:
   idf.py build
   idf.py flash
   idf.py monitor

## Configuration

Update the OpenThread and sensor configuration as needed in main.c:
#define OPENTHREAD_CHANNEL 11
#define OPENTHREAD_PANID 0x1234
#define MOISTURE_SENSOR_PIN GPIO_NUM_32

## Usage

Once flashed, the ESP32 will join the OpenThread network and begin measuring soil moisture. The data is periodically sent to the OpenThread Leader Gateway. You can monitor the ESP32's serial output for real-time data and network status.

## Troubleshooting

Ensure the soil moisture sensor is correctly connected to the ESP32.
Verify that the OpenThread settings match those of the Leader Gateway.
Use the idf.py monitor command to debug and view real-time logs.
Contributing
Contributions are welcome! Please submit a pull request or open an issue to discuss potential changes.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
