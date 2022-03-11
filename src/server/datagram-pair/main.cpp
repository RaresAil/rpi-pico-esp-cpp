#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

#ifndef __DATAGRAM_PAIR_H__
#define __DATAGRAM_PAIR_H__

bool start_datagrarm_socket() {
  char sendBuffer[SENDBUFFERLEN];

  snprintf(
    sendBuffer,
    SENDBUFFERLEN,
    "CIPSTART=\"UDP\",\"255.255.255.255\",%s",
    SOCKET_PORT
  );

  printf("[Server]-[Datagram] Disableing multi-connections\n");
  if (!sendATCommandOK("CIPMUX=0", 2000)) {
    return false;
  }

  printf("[Server]-[Datagram] Starting socket\n");
  if (!sendATCommandOK(sendBuffer, 2000)) {
    return false;
  }

  printf("[Server]-[Datagram] Socket started\n");
  return true;
}

void datagrarm_wait_for_response(const char* input, bool (*callback)(const char*)) {
  char sendBuffer[SENDBUFFERLEN];
  char inputBuffer[SENDBUFFERLEN];
  printf("[Server]-[Datagram] Waiting for incoming data\n");

  snprintf(inputBuffer, SENDBUFFERLEN, "%s\0", input);
  snprintf(sendBuffer, SENDBUFFERLEN, "CIPSENDEX=%d", strlen(inputBuffer));

  while(1) {
    sendATCommand(sendBuffer, 250, ">", true, 0, true);
    uart_puts(UART_ID, inputBuffer);
    sendATCommandOK("", 250, true);

    if (
      sendATCommand("", 250, "+IPD,", true) && 
      sendATCommand("", 100, ":", true)
    ) {
      const std::string dataLen = getParam(0, ':', '\0', responseBuffer);
      printf("[Server]-[Datagram] Incoming data size: (%s)\n", dataLen.c_str());

      if (sendATCommand("", 250, "", true, std::stoi(dataLen))) {
        if (callback(responseBuffer)) {
          break;
        }
      }
    }

    sleep_ms(1000);
  }


  if (!sendATCommandOK("CIPCLOSE", 2000)) {
    printf("[Server]-[Datagram] Failed to close socket\n");
    return;
  }

  printf("[Server]-[Datagram] Socket closed\n");
}

#endif