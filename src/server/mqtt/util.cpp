#include <stdio.h>
#include <cstring>
#include <string>

bool publishMQTTMessage(const std::string& message) {
  try {
    char sendBuffer[SENDBUFFERLEN];

    snprintf(
      sendBuffer,
      SENDBUFFERLEN,
      "MQTTPUBRAW=0,\"%s/%s\",%d,0,0",
      MQTT_STATUS_TOPIC_PREFIX,
      UUID,
      message.length()
    );

    sendATCommand(sendBuffer, 250, ">", true, 0, true);
    uart_puts(UART_ID, message.c_str());
    return sendATCommandOK("", 500, true);
  } catch (...) {
    printf("[MQTT]:[ERROR]: while publishing to MQTT broker\n");
    return false;
  }
}