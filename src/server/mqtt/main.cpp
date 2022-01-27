#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

#include "constants.cpp"

#include "../datagram-pair/main.cpp"
#include "util.cpp"

#include "serve.cpp"

int schema = 0;
std::string host;
std::string port;
std::string user;
std::string password;

bool validate_datagram_response(const char* response) {
  try {
    json j_response = json::parse(response);
    const std::string uuid = j_response["uuid"].get<std::string>();
    const std::string encryptedData = j_response["data"].get<std::string>();

    if (uuid != UUID) {
      return false;
    }

    json j_data = json::parse(hex2ascii(decrypt(encryptedData)));
    const std::string connHost = j_data["host"].get<std::string>();
    password = j_data["password"].get<std::string>();
    user = j_data["user"].get<std::string>();

    if (connHost.empty()) {
      return false;
    }

    const std::string protocol = getParam(0, ':', '\0', connHost);
    host = getParam(1, ':', '\0', connHost);
    port = getParam(2, ':', '\0', connHost);
    host.erase(0, 2);
  

    // On ESP8266, the TLS is no working should use ESP32 instead
    if (protocol == "mqtt" || protocol == "tcp") {
      schema = 1;
    }  else if (protocol == "mqtts" || protocol == "tls") {
      schema = 2;
    }  else if (protocol == "ws") {
      schema = 6;
    }  else if (protocol == "wss") {
      schema = 7;
    } else {
      return false;
    }

    printf(
      "[MQTT]: Datagram validated (Host: (Protocol: %s, Schema: %d, Host: %s, Port: %s), User: %s, Password: %s)\n", 
      protocol.c_str(),
      schema,
      host.c_str(),
      port.c_str(),
      user.c_str(),
      password.c_str()
    );
    return true;
  } catch (...) {
    printf("[MQTT]: Error parsing datagram response\n");
    return false;
  }
}

bool connect_to_mqtt() {
  char sendBuffer[SENDBUFFERLEN];
  printf("[MQTT]: Connecting to MQTT broker\n");

  if (!start_datagrarm_socket()) {
    return false;
  }
  
  json jsn = json::object();
  jsn["uuid"] = UUID;
  jsn["type"] = TYPES(TYPE::PAIR);

  datagrarm_wait_for_response(jsn.dump().c_str(), validate_datagram_response);

  snprintf(
    sendBuffer,
    SENDBUFFERLEN,
    "MQTTUSERCFG=0,%d,\"pico_%s\",\"%s\",\"%s\",0,0,\"\"",
    schema,
    UUID,
    user.c_str(),
    password.c_str()
  );

  if (!sendATCommandOK(sendBuffer, 2000)) {
    printf("[MQTT]: Failed to set MQTT User configuration\n");
    return false;
  }

  if (!sendATCommandOK("MQTTCONNCFG=0,60,0,\"\",\"\",0,0", 2000)) {
    printf("[MQTT]: Failed to set MQTT Configuration\n");
    return false;
  }

  snprintf(
    sendBuffer,
    SENDBUFFERLEN,
    "MQTTCONN=0,\"%s\",%s,1",
    host.c_str(),
    port.c_str()
  );

  if (!sendATCommandOK(sendBuffer, 30000)) {
    printf("[MQTT]: Failed to connect to MQTT broker\n");
    return false;
  }

  printf("[MQTT]: Connected to MQTT broker\n");

  snprintf(
    sendBuffer,
    SENDBUFFERLEN,
    "MQTTSUB=0,\"%s/%s\",0",
    MQTT_DATA_TOPIC_PREFIX,
    UUID
  );
  if (!sendATCommandOK(sendBuffer, 2000)) {
    printf("[MQTT]: Failed to subscribe to MQTT\n");
    return false;
  }

  printf("[MQTT]: Subscribed to '%s/%s'\n", MQTT_DATA_TOPIC_PREFIX, UUID);

  json pair_j = json::object();
  pair_j["type"] = TYPES(TYPE::PAIR);
  if (!publishMQTTMessage(pair_j.dump())) {
    printf("[MQTT]: Failed to publish pair message\n");
    return false;
  }

  printf("[MQTT]: Waiting for pairing\n");

  multicore_launch_core1(serve_broker);
  return true;
}