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
- RTC (Time in string format and in ms)
- HMAC SHA256 authorization
- Error handler
- GET `/status` endpoint

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

### Performance

~~By testing, the fastest response is 500ms and the avg is 750ms~~

After some more optimizations, the avg response times are:

- For errors: 150 - 200 ms
- GET Requests: 300 - 600 ms
- POST Requests: 500 - 900 ms
- If the ESP does not respond with the http request the timeout util connection close is 5 seconds

### Hardware Configuration

| ESP8266                     | RPi Pico |
| --------------------------- | -------- |
| GND                         | GND      |
| IO15 (If the module has it) | GND      |
| RX                          | TX       |
| TX                          | RX       |
| 3.3v                        | 3.3v     |
| CH_PD / EN / CHIP_EN        | 3.3v     |

### Coming Soon (if requested)

- MQTT implementation for IDF-AT Firmware

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
