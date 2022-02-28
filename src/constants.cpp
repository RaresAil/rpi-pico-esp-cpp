#define UART_ID           uart0
#define BAUD_RATE         115200
#define DATA_BITS         8
#define STOP_BITS         1
#define PARITY            UART_PARITY_NONE
#define UART_TX_PIN       0
#define UART_RX_PIN       1

#define STATUS_LED_PIN    PICO_DEFAULT_LED_PIN
#define RESTORE_PIN       16

// Configuration of the server
// The SOCKET_PORT is used for Datagram Socket
#define SENDBUFFERLEN     1000
#define RESPONSEBUFFERLEN 1500
#define MICROS_MS         1000
#define SOCKET_PORT       "54412"

// MQTT
// The complete topic is like:
// iot-embedded/data/UUID - Subscribe (Receive data)
// iot-embedded/status/UUID - Send data
#define MQTT_DATA_TOPIC_PREFIX    "iot-embedded/data"
#define MQTT_STATUS_TOPIC_PREFIX  "iot-embedded/status"
#define PACKET_MAX_EXPIRE_TIME_MS 5000
#define PACKET_EXPIRE_TIME_MS     3000

// Board secrets
// 0 -> None | 1 -> HMAC | 2 -> AES-256-CTR
#define BOARD_SECURITY            2
// 32 bytes long (44 chars in base64)
// The IV is randomly generated
// The same key is also used for HMAC
#define ENCRYPTION_KEY             "RmrhgEiJlAQJ/MII+wQmN9SVFY7CZ0A6wsW/2txHqsk="

// Board constants, this should be unique for each board
#define UUID              "9d5437d5-3303-4800-88cd-871ad1f08e01"
#define HOSTNAME          "rpi-pico-esp-template"
#define FIRMWARE_VERSION  "0.0.1"
#define HARDWARE_REVISION "0.1-rev0"
#define MANUFACTURER      "Raspberry Pi Ltd"
#define MODEL             "RP2040-Thermostat"

#define SERVICE_TYPE      1 // None = 0 | Thermostat = 1