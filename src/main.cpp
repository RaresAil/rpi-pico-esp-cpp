#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/rtc.h"
#include "hardware/irq.h"
#include "constants.cpp"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string>

#include "cpp-base64/base64.cpp"
#include "stbrumme/sha256.cpp"
#include "stbrumme/hmac.h"

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#include "nlohmann/json.hpp"
#endif

using json = nlohmann::json;

#include "utils/hmac.cpp"

#include "esp/main.cpp"
#include "server/main.cpp"

#ifdef IS_DEBUG_MODE
#include "pico/bootrom.h"
#endif

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
  rtc_init();

  stdio_filter_driver(&stdio_usb);

#ifdef IS_DEBUG_MODE
  const uint BOOTSEL_BUTTON = 15;
  gpio_init(BOOTSEL_BUTTON);
  gpio_set_dir(BOOTSEL_BUTTON, GPIO_IN);
  if (gpio_get(BOOTSEL_BUTTON)) {
    printf("[MAIN]: Bootsel button pressed\n");
    reset_usb_boot(0, 0);
  }
#endif

  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
  gpio_put(STATUS_LED_PIN, 1);

#ifdef IS_DEBUG_MODE
  sleep_ms(4000);
#else
  sleep_ms(500);
#endif

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