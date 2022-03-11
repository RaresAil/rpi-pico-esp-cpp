#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/rtc.h"
#include "hardware/adc.h"
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

bool alarm_triggered = false;


#if SERVICE_TYPE == 1
#include "services/Thermostat.cpp"
#endif

#include "utils/hex.cpp"
#include "esp/utils.cpp"
#include "utils/aes.cpp"
#include "esp/main.cpp"
#include "utils/hmac.cpp"

void reboot_board() {
  printf("[MAIN]: Rebooting board\n");
  sleep_ms(1000);

  if (watchdog_caused_reboot()) {
    sleep_ms(1000);
  }

  watchdog_enable(10, false);
  while (1);
}

#include "server/main.cpp"

#ifdef IS_DEBUG_MODE
#include "pico/bootrom.h"
#endif

// TODO: Add switch to force summer/winter mode (In case of a faulty sensor)
// TODO: Switch to allow offline mode
// TODO: OLED Display and Buttons for manual control

int64_t alarm_callback(alarm_id_t id, void *user_data) {
  alarm_triggered = true;
  return TRIGGER_INERVAL_MS * 1000;
}

int main() {
#ifdef IS_DEBUG_MODE
  stdio_usb_init();
  stdio_filter_driver(&stdio_usb);

  const uint BOOTSEL_BUTTON = 15;
  gpio_init(BOOTSEL_BUTTON);
  gpio_set_dir(BOOTSEL_BUTTON, GPIO_IN);
  if (gpio_get(BOOTSEL_BUTTON)) {
    printf("[MAIN]: Bootsel button pressed\n");
    reset_usb_boot(0, 0);
  }
#endif

  adc_init();
  rtc_init();

  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
  gpio_put(STATUS_LED_PIN, 1);

  initialize_uart();

  sleep_ms(500);

  gpio_init(RESTORE_PIN);
  gpio_set_dir(RESTORE_PIN, GPIO_IN);
  if (gpio_get(RESTORE_PIN)) {
    printf("[MAIN]: Restore button pressed\n");
    sendATCommandOK("RESTORE", 2000);
  }

#ifdef IS_DEBUG_MODE
  sleep_ms(3500);
#endif

  gpio_put(STATUS_LED_PIN, 0);
  printf("\n\n\n~~~~~~~~~~~~~~~RPico-BOOT~~~~~~~~~~~~~~~\n");
  printf("~~~~~Made by: 'github.com/RaresAil'~~~~~\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n");

  if (!initialize_esp()) {
    reboot_board();
    return -1;
  }

  service.trigger_data_update();

  if(!start_server()) {
    printf("[Server]: Failed to start server\n");
    reboot_board();
    return -1;
  }

  add_alarm_in_ms(TRIGGER_INERVAL_MS, alarm_callback, NULL, false);
  while(1) {
    while(!alarm_triggered) {
      tight_loop_contents();
    }

    alarm_triggered = false;
    service.trigger_data_update();
  }
}