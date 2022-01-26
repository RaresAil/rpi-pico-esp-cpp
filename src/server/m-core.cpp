#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

// Executed on core 0
void __not_in_flash_func (core0_sio_irq)() {
  while (multicore_fifo_rvalid()) {
    try {
      const uint64_t time = 100 * MICROS_MS;
      const char* linkId = (char(*))multicore_fifo_pop_blocking();

      char* method;
      char* route;
      if (
        multicore_fifo_pop_timeout_us(time, (uint32_t(*))&method) &&
        multicore_fifo_pop_timeout_us(time, (uint32_t(*))&route)
      ) {
        printf("[Server]: Processing '%s' request on '%s' from client '%s'\n", 
          method, 
          route,
          linkId
        );

        if (strstr(method, METHODS(METHOD::POST).c_str()) != NULL) {
          char* _requestBody;

          if (multicore_fifo_pop_timeout_us(time, (uint32_t(*))&_requestBody)) {
            const std::string requestBody = extractJson(_requestBody).c_str();
            
            if (!requestBody.empty()) {
              handle_post_request(linkId, route, requestBody);
            } else {
              close_connection(linkId, HTTP_STATUS::BAD_REQUEST);
            }
          } else {
            printf("[Server]: Timeout while processing '%s' request for client '%s'\n", method, linkId);
            close_connection(linkId, HTTP_STATUS::INTERNAL_SERVER_ERROR);
          }

        } else if (strstr(method, METHODS(METHOD::GET).c_str()) != NULL) {
          handle_get_request(linkId, route);
        } else {
          close_connection(linkId, HTTP_STATUS::METHOD_NOT_ALLOWED);
        }
      } else {
        printf("[Server]: Timeout while processing UNKNOWN request for client '%s'\n", linkId);
        close_connection(linkId, HTTP_STATUS::INTERNAL_SERVER_ERROR);
      }
    } catch (...) {
      printf("[Server]:[ERROR]: while handling HTTP request\n");
    }
  }

  multicore_fifo_clear_irq();
}
