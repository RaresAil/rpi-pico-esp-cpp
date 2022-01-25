#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "constants.cpp"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string>

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#include "nlohmann/json.hpp"
#endif

using json = nlohmann::json;

#include "esp/main.cpp"
#include "server/main.cpp"

void reboot_board() {
  printf("[MAIN]: Rebooting board\n");
  sleep_ms(1000);

  if (watchdog_caused_reboot()) {
    sleep_ms(1000);
  }

  watchdog_enable(10, false);
  while (1);
}

int main() {
  stdio_usb_init();
  stdio_filter_driver(&stdio_usb);

  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
  gpio_put(STATUS_LED_PIN, 1);

  sleep_ms(4000);
  gpio_put(STATUS_LED_PIN, 0);

  printf("\n\n\n~~~~~~~~~~~~~~~RPico-BOOT~~~~~~~~~~~~~~~\n");
  printf("~~~~~Made by: 'github.com/RaresAil'~~~~~\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n");

  if (
    !initialize_uart() || 
    !initialize_esp()
  ) {
    reboot_board();
    return -1;
  }

  if(!start_server()) {
    printf("[Server]: Failed to start server\n");
    reboot_board();
    return -1;
  }

  while(1) {
    tight_loop_contents();
  }
}