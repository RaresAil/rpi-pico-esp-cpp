#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

void respond_to_mqtt(const json& data, const TYPE& type) {
  json data_input = json::object();
  data_input["data"] = data;
  data_input["type"] = TYPES(type);

  const u_int64_t input_exp = get_datetime_ms() + PACKET_EXPIRE_TIME_MS;
  data_input["exp"] = input_exp;

  #if BOARD_SECURITY == 1
  const std::string input_sign = generate_sign(TYPES(type) + std::to_string(input_exp));
  data_input["sign"] = input_sign;
  #endif

  #if BOARD_SECURITY == 2
  const std::string pair_str = encrypt_256_aes_ctr(data_input.dump());
  #else
  const std::string pair_str = data_input.dump();
  #endif

  publishMQTTMessage(pair_str);
}

void respond_to_mqtt(const json& data) {
  respond_to_mqtt(data, TYPE::DATA);
}

void handle_mqtt_message(const json& data) {
  try {
    const std::string type = data["type"].get<std::string>();
    const u_int64_t exp = data["exp"].get<u_int64_t>();
    const u_int64_t now = get_datetime_ms();
  
    if (exp < now || exp >= now + PACKET_MAX_EXPIRE_TIME_MS) {
      printf("[MQTT]: Request expired\n");
      return;
    }

    #if BOARD_SECURITY == 1
    if (!verify_sign(data["sign"].get<std::string>(), type + std::to_string(exp))) {
      printf("[MQTT]: Invalid pair signature\n");
      return;
    }

    printf("[MQTT]: Valid pair signature\n");
    #endif

    if (type == TYPES(TYPE::PAIR)) {
      printf("[MQTT]: Paired successful\n");

      json data_input = json::object();
      data_input["hardware"] = HARDWARE_REVISION;
      data_input["firmware"] = FIRMWARE_VERSION;
      data_input["manufacturer"] = MANUFACTURER;
      data_input["hostname"] = HOSTNAME;
      data_input["type"] = DEVICE_TYPE;
      data_input["model"] = MODEL;
      data_input["mac"] = MAC;

      respond_to_mqtt(data_input, TYPE::REGISTER);
    } else if (type == TYPES(TYPE::DATA)) {
      handle_command(data["data"], respond_to_mqtt);
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
      if (starting_mode == START_MODE::PERMISSIVE) {
        cancel_alarm(alarm_id);
        return;
      } else {
        reboot_board();
      }
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

            if (dataLen > 0 && sendATCommand("", 200, "", true, dataLen)) {
              #if BOARD_SECURITY == 2
              const json mqttData = json::parse(decrypt_256_aes_ctr(responseBuffer));
              #else
              const json mqttData = json::parse(responseBuffer);
              #endif

              printf("[MQTT]-[C1]: Incoming data: (%s)\n", mqttData.dump().c_str());
              handle_mqtt_message(mqttData);
            }
          } else {
            if (strstr(responseBuffer, "DISCONNECTED") != NULL) {
              service.update_newtwork("DISCONNECTED");
              printf("[MQTT]:[ERROR]: Disconnected from broker\n");
              alarm_id = add_alarm_in_ms(30 * 1000, reboot_alarm, NULL, true);
            } else if (strstr(responseBuffer, "CONNECTED") != NULL) {
              service.update_newtwork("ON");
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