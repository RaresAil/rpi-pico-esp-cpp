#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

// Executed on core 1
void serve_clients() {
  printf("[Server]: Serving clients\n");

  while(1) {
    if (uart_is_readable(UART_ID)) {
      try {
        char responseBuff[RESPONSEBUFFERLEN * 2] = "";
        sendATCommand("", 250, "{", true, true);
        bool hasBody = false;

        const char* c_temp_ipd = strstr(responseBuffer, "IPD");
        if (c_temp_ipd != NULL) {
          const std::string httpMethod = getParam(1, ':', ' ', c_temp_ipd);
          hasBody = is_valid_method_with_body(httpMethod);
        }

        if (hasBody) {
          strcpy(responseBuff, responseBuffer);
          sendATCommand("", 250, "}\r\n\r\n", true);
        }

        snprintf(responseBuff, RESPONSEBUFFERLEN * 2, "%s%s", responseBuff, responseBuffer);
        const std::string response(responseBuff);

        const int connLoc = response.find("CONNECT");
        if (connLoc != std::string::npos) {
          printf("[Server]: Client '%c' connected\n", response[connLoc - 2]);
        }

        const int clsLoc = response.find("CLOSED");
        if (clsLoc != std::string::npos) {
          printf("[Server]: Client '%c' disconnected\n", response[clsLoc - 2]);
        }

        const char* c_ipdData = strstr(responseBuff, "+IPD");
        if (c_ipdData != NULL) {
          handle_request(c_ipdData);
        }
      } catch (...) {
        printf("[Server]:[ERROR]: while handling client request\n");
      }
    }
  }
}