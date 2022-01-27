#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

void handle_mqtt_message(const json& data) {
  try {
    const std::string type = data["type"].get<std::string>();

    if (type == TYPES(TYPE::PAIR)) {
      printf("[MQTT]: Paired successful\n");

      json input = json::object();

      json data_input = json::object();
      data_input["hardware"] = HARDWARE_REVISION;
      data_input["firmware"] = FIRMWARE_VERSION;
      data_input["manufacturer"] = MANUFACTURER;
      data_input["hostname"] = HOSTNAME;
      data_input["type"] = DEVICE_TYPE;
      data_input["model"] = MODEL;
      data_input["mac"] = MAC;

      input["type"] = TYPES(TYPE::REGISTER);
      input["data"] = data_input;

      publishMQTTMessage(input.dump());
    } else if (type == TYPES(TYPE::DATA)) {
      handle_command(data["data"], type, publishMQTTMessage);
    } else {
      printf("[MQTT]:[ERROR]: Unknown type\n");
    }
  } catch (...) {
    printf("[MQTT]:[ERROR]: while handling MQTT message\n");
  }
}

bool ping_timeout = false;

int64_t reboot_alarm(alarm_id_t id, void *user_data) {
  printf("[MQTT]:[ERROR]: Ping timeout!\n");
  ping_timeout = true;
  return 0;
}

// Executed on core 1
void serve_broker() {
  alarm_id_t alarm_id;

  printf("[Server]: Serving broker\n");
  const std::string check_response = 
    "\"" + 
    std::string(MQTT_DATA_TOPIC_PREFIX) + 
    "/" + 
    std::string(UUID) + 
    "\",";

  while(1) {
    if (ping_timeout) {
      reboot_board();
    }

    if (uart_is_readable(UART_ID)) {
      try {
        if (sendATCommand("", 500, "+MQTT", true)) {
          if (
            sendATCommand("", 500, "SUBRECV", true) &&
            sendATCommand("", 100, check_response.c_str(), true) &&
            sendATCommand("", 100, ",", true)
          ) {
            uint16_t dataLen = atoi(responseBuffer);

            if (dataLen > 0 && sendATCommand("", 100, "", true, dataLen)) {
              const json mqttData = json::parse(responseBuffer);
              printf("[MQTT]-[C1]: Incoming data: (%s)\n", mqttData.dump().c_str());
              handle_mqtt_message(mqttData);
            }
          } else {
            if (strstr(responseBuffer, "DISCONNECTED") != NULL) {
              printf("[MQTT]:[ERROR]: Disconnected from broker\n");
              alarm_id = add_alarm_in_ms(30 * 1000, reboot_alarm, NULL, true);
            } else if (strstr(responseBuffer, "CONNECTED") != NULL) {
              printf("[MQTT]: Reconnected to MQTT broker\n");
              cancel_alarm(alarm_id);
            }
          }
        }
      } catch (...) {
        printf("[MQTT]:[ERROR]: while handling client request\n");
      }
    }
  }
}