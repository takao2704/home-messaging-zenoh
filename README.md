# Zenoh Pico Project

This project is a sample application using the Zenoh protocol with M5Stack devices.

## Requirements

- M5Stack device
- WiFi network
- PlatformIO IDE

## Setup

### 1. Clone the repository

```sh
git clone https://github.com/yourusername/zenoh_pico.git
cd zenoh_pico
```

### 2. Install PlatformIO IDE

If you haven't installed PlatformIO IDE, please install it from the following link:
[PlatformIO IDE](https://platformio.org/install/ide?install=vscode)

### 3. Update WiFi settings

Open the `src/config.h` file and set your WiFi SSID and password.

```cpp
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
```

### 4. Build the PlatformIO project

Open the project in PlatformIO IDE and click the build button to build the project.

### 5. Upload to the device

Once the build is successful, upload the firmware to the M5Stack device.

## Usage

1. Power on the M5Stack device.
2. Wait for the device to connect to WiFi and establish a Zenoh session.
3. Use the device buttons to select and send messages.

- Button A: Select next message
- Button B: Select previous message
- Button C: Send message

## License

This project is licensed under the EPL-2.0 or Apache-2.0 license. For more details, see the [LICENSE](https://github.com/eclipse-zenoh/zenoh-pico/tree/main/examples/arduino) file.
