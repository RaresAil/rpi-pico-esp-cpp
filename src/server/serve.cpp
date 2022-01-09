#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

void __not_in_flash_func (core1_sio_irq)() {
  while (multicore_fifo_rvalid()) {
    uintptr_t* out = (uintptr_t(*))multicore_fifo_pop_blocking();
    printf("CORE 1 %s\n", out);
  }

  multicore_fifo_clear_irq();
}

// Executed on core 1
void serve_clients() {
  mutex_enter_blocking(&m_esp);
  printf("[Server]: Serving clients\n");
  multicore_fifo_clear_irq();
  irq_set_exclusive_handler(SIO_IRQ_PROC1, core1_sio_irq);
  irq_set_enabled(SIO_IRQ_PROC1, true);
  mutex_exit(&m_esp);

  while(1) {
    if (uart_is_readable(UART_ID)) {
      try {
        char responseBuff[RESPONSEBUFFERLEN * 2] = "";

        // For requests with a JSON body, we split the response into two parts:
        if (sendATCommand("", 250, "{", true)) {
          strcpy(responseBuff, responseBuffer);
          sendATCommand("", 250, "", true);
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