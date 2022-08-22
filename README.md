This repo will be archived due the new RPI Pico W model

## Legacy (HTTP)

For a http implementation, you can use the [http-template](https://github.com/RaresAil/rpi-pico-esp-cpp/tree/http-template) branch which is no longer maintained in favor
of **MQTT**

ESP & Firmware For HTTP:

- ESP 01S (1MB - 8MBit)
- AT NoOS 1.5.4 & 3.0.5 (1MB - 8MBit)

## ESP Model

Tested with:

- ESP 12F (4MB - 32MBit)

AT Firmware:

- **AT-IDF 2.2.1.0 - 24 Dec 2021** (`wroom02-n-at`) (2MB - 16MBit) (Recommended)

To can use IDF 2.2.1.0 for 12F, i had to use `esp8266-wroom02-n-at` instead of `esp8266-wroom02-at`.

The `esp8266-wroom02-at` uses UART1 and `esp8266-wroom02-n-at` uses UART0 (Same as NoOS version).

## Structure

External libraries:

- https://github.com/nlohmann/json (For JSON parsing)
- https://github.com/stbrumme/hash-library (For Hashing)
- https://github.com/ReneNyffenegger/cpp-base64 (For Base64 encoding)
- https://github.com/rweather/arduinolibs Crypto (For AES-256-CTR encryption)
- https://github.com/Harbys/pico-ssd1306 SSD1306 (For OLED Display)

Features:

- SmartConfig with ESP Touch and AirKiss
- Auto reboot if the esp didn't connected to the mqtt server
- RTC (Time in string format and in ms)
- HMAC SHA256 verification on packets (This or AES)
- AES-256-CTR encryption for packets (This or HMAC)
- AES-256-CTR encryption for datagram socket
- Error handler
- MQTT & DataGram socket for IoT communication with a smart home server

For ESP Touch, i am using the ExpressIf's mobile application

## Services

Current done services:

- Thermostat
  - It requires and external LM35 sensor connected to VSys (5v)
  - It checks every 1 minute the temperature

To add a custom service, create a new file based on one already created,
all of the services must have:

```cpp
// A type what will be received by the server to know what kind of device is this
#define DEVICE_TYPE       "Type"


// `handle_command` will be called automatically when the client received a message of type DATA
// the command param it will be the `data` key.
// e.g. { "type": "DATA", "data": {...} }
void handle_command(const json& command, const std::string& type, bool (*respond)(const std::string&));
```

### Hardware Configuration

| ESP8266                     | RPi Pico |
| --------------------------- | -------- |
| GND                         | GND      |
| IO15 (If the module has it) | GND      |
| RX                          | TX       |
| TX                          | RX       |
| 3.3v                        | 3.3v     |
| CH_PD / EN / CHIP_EN        | 3.3v     |

### Change Project Name

Search for `project_name` in `CMakeLists.txt`, it should be something like `project(project_name C CXX ASM)`.

Replace only `project_name`, e.g. `project(my_project C CXX ASM)`

<details>
<summary>Example console output</summary>

```
~~~~~~~~~~~~~~~RPico-BOOT~~~~~~~~~~~~~~~
~~~~~Made by: 'github.com/RaresAil'~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


[AT-Command]-[GMR]: Sending Command
[AT-Command]-[RST]: Sending Command
[ESP8266]: SDK Version: v3.4-43-ge9516e4c
[AT-Command]-[CWMODE=1]: Sending Command
[Server]: ESP8266 in Station Mode
[AT-Command]-[CWSTATE?]: Sending Command
[Server]: Connecting to last WiFi configuration
[AT-Command]-[CWJAP]: Sending Command
[AT-Command]-[CIFSR]: Sending Command
[Server]: Connected to WiFi
[Server]: IP Address: 'x.x.x.x'
[Server]: Setting the UTC time to RTC
[AT-Command]-[CIPSNTPCFG=1,0,"pool.ntp.org"]: Sending Command
[AT-Command]-[CIPSNTPTIME?]: Sending Command
[AT-Command]-[CIPSNTPTIME?]: Sending Command
[AT-Command]-[CIPSNTPTIME?]: Sending Command
[Server]-[INFO]: UTC time: 'Wed Jan 26 14:07:00 2022'
[Server]: Setting RTC time to 2022-1-26 14:7:0
[AT-Command]-[CIPMUX=1]: Sending Command
[AT-Command]-[CIPSERVER=1,54412]: Sending Command
[Server]: Server running on port '54412'
[AT-Command]-[CIPSTO=5]: Sending Command
[Server]: Client timeout set to 5s

~~~~~~~~~~~~~~~~~~~~~
Debug Signature:
eyJhbGciOiJIUzI1NiIsImV4cCI6MTY0NTg4NDQyMTAwMCwic3ViIjoicnBpLXBpY28tZXNwLXRlbXBsYXRlIn0.
YWRhZjFmYjY1ZjE1OTNkN2U0MmU4OTIwZmRhY2EyMWEyMzVhNGJkZTg1OTJlZDdhZGJlMDA4OTU5OTFjZGEyNg
~~~~~~~~~~~~~~~~~~~~~

[Server]: Serving clients
```

</details>
