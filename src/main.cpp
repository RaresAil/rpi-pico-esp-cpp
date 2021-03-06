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

enum class START_MODE {
  ONLINE,
  OFFLINE,
  PERMISSIVE,
};

using json = nlohmann::json;

bool alarm_triggered = false;

#include "utils/hex.cpp"
#include "esp/utils.cpp"
#include "utils/aes.cpp"
#include "esp/main.cpp"
#include "utils/hmac.cpp"

#if SERVICE_TYPE == 1
#include "services/Thermostat.cpp"
#endif

void reboot_board() {
  service.center_message("Rebooting", true);
  printf("[MAIN]: Rebooting board\n");
  sleep_ms(1000);

  if (watchdog_caused_reboot()) {
    sleep_ms(1000);
  }

  watchdog_enable(10, false);
  while (1);
}

START_MODE starting_mode = START_MODE::ONLINE;

#include "server/main.cpp"

#ifdef IS_DEBUG_MODE
#include "pico/bootrom.h"
#endif

int64_t alarm_callback(alarm_id_t id, void *user_data) {
  alarm_triggered = true;
  return TRIGGER_INERVAL_MS * 1000;
}

int main() {
#ifdef IS_DEBUG_MODE
  stdio_usb_init();
  stdio_filter_driver(&stdio_usb);

  const uint BOOTSEL_BUTTON = 16;
  gpio_init(BOOTSEL_BUTTON);
  gpio_set_dir(BOOTSEL_BUTTON, GPIO_IN);
  if (gpio_get(BOOTSEL_BUTTON)) {
    printf("[MAIN]: Bootsel button pressed\n");
    reset_usb_boot(0, 0);
  }
#endif

  rtc_init();

  gpio_init(STATUS_LED_PIN);
  gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
  gpio_put(STATUS_LED_PIN, 1);


  gpio_init(OFFLINE_MODE_PIN);
  gpio_set_dir(OFFLINE_MODE_PIN, GPIO_IN);
  gpio_init(ALLOW_ERROR_PIN);
  gpio_set_dir(ALLOW_ERROR_PIN, GPIO_IN);

  initialize_uart();

  sleep_ms(450);

  std::string init_message = "Initializing";

  service.setup_service();

  if (gpio_get(OFFLINE_MODE_PIN)) {
    printf("[MAIN]: Offline mode\n");
    init_message = "Init Offline";
    starting_mode = START_MODE::OFFLINE;
  } else if (gpio_get(ALLOW_ERROR_PIN)) {
    printf("[MAIN]: Allow Error mode\n");
    init_message = "Init Permissive";
    starting_mode = START_MODE::PERMISSIVE;
  }

  service.center_message(init_message);
  sleep_ms(50);

  gpio_init(RESTORE_PIN);
  gpio_set_dir(RESTORE_PIN, GPIO_IN);
  if (gpio_get(RESTORE_PIN) && starting_mode != START_MODE::OFFLINE) {
    int8_t counter = 3;
    while (counter > 0) {
      service.center_message(std::string("Restoring in ") + std::to_string(counter));
      sleep_ms(1000);
      counter--;
    }


    service.center_message("Restoring...");

    printf("[MAIN]: Restore button pressed\n");
    sendATCommandOK("RESTORE", 2000);
    service.center_message(init_message);
  }

#ifdef IS_DEBUG_MODE
  sleep_ms(3500);
#endif

  gpio_put(STATUS_LED_PIN, 0);
  printf("\n\n\n~~~~~~~~~~~~~~~RPico-BOOT~~~~~~~~~~~~~~~\n");
  printf("~~~~~Made by: 'github.com/RaresAil'~~~~~\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n");

  bool esp_failed = false;

  if (starting_mode != START_MODE::OFFLINE && !initialize_esp()) {
    esp_failed = true;

    if (starting_mode != START_MODE::PERMISSIVE) {
      reboot_board();
      return -1;
    }
  }

  service.trigger_data_update();
  service.ready();

  if(starting_mode != START_MODE::OFFLINE && !esp_failed && !start_server()) {
    printf("[Server]: Failed to start server\n");
    service.update_newtwork("");

    if (starting_mode != START_MODE::PERMISSIVE) {
      reboot_board();
      return -1;
    }
  }

  add_alarm_in_ms(TRIGGER_INERVAL_MS, alarm_callback, NULL, false);
  while(1) {
    service.loop();
    if (alarm_triggered) {
      alarm_triggered = false;
      service.trigger_data_update();
    }
  }
}