### ESP Model

Tested with:

- **ESP 12F** (Best performance)
- ESP 01S

AT Firmware:

- **AT-IDF 2.2.1.0 - 24 Dec 2021** (`wroom02-n-at`) (2MB - 16MBit) (Recommended)
- AT NoOS 1.5.4 & 3.0.5 (1MB - 8MBit)

To can use IDF 2.2.1.0 for 12F, i had to use `esp8266-wroom02-n-at` instead of `esp8266-wroom02-at`.

The `esp8266-wroom02-at` uses UART1 and `esp8266-wroom02-n-at` uses UART0 (Same as NoOS version).

### Structure

External libraries:

- https://github.com/nlohmann/json (For JSON parsing)

Features:

- SmartConfig with ESP Touch and AirKiss
- Auto reboot if the esp didn't started the http server

For ESP Touch, i am using the ExpressIf's mobile application

The only available `content-type` for `requests` and `responses` is `application/json`.

Right now the project uses only POST and GET, to add a new method, you need to change the files:

- server/constants.cpp
  - `enum class METHOD`
  - `std::string METHODS`
- server/utils.cpp
  - `is_valid_method_with_no_body`
  - `is_valid_method_with_body`
- server/m-core.cpp
  - `core0_sio_irq`

By testing, the fastest response is 500ms and the avg is 750ms

### Hardware Configuration

| ESP8266                     | RPi Pico |
| --------------------------- | -------- |
| GND                         | GND      |
| IO15 (If the module has it) | GND      |
| RX                          | TX       |
| TX                          | RX       |
| 3.3v                        | 3.3v     |
| CH_PD / EN / CHIP_EN        | 3.3v     |

### Coming Soon

- MQTT implementation for IDF-AT Firmware

### Change Project Name

Search for `project_name` in `CMakeLists.txt`, it should be something like `project(project_name C CXX ASM)`.

Replace only `project_name`, e.g. `project(my_project C CXX ASM)`
