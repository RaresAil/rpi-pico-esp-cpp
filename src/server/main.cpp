#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

#include "constants.cpp"
#include "utils.cpp"

#include "handler.cpp"
#include "m-core.cpp"
#include "serve.cpp"

bool led_blink_timer(struct repeating_timer *t) {
  try {
    gpio_put(STATUS_LED_PIN, !gpio_get(STATUS_LED_PIN));
    return true;
  } catch (...) {
    printf("[Server]:[ERROR]: while blinking status led\n");
    return false;
  }
}

// Executed on core 0
bool start_server() {
  try {
    mutex_enter_blocking(&m_esp);

    char sendBuffer[SENDBUFFERLEN];

    if (!sendATCommandOK("CWMODE=1", 2000)) {
      return false;
    }

    printf("[Server]: ESP8266 in Station Mode\n");
    const int wifiState = getWifiState();

    if (wifiState <= 0) {
      struct repeating_timer timer;
      add_repeating_timer_ms(500, led_blink_timer, NULL, &timer);

      printf("[Server]: Starting SmartConfig\n");

      /*
        The second parameter is the auth floor:
    
        Wi-Fi authentication mode floor. ESP-AT will not connect to the AP whose authmode is lower than this floor.
        0: OPEN (Default)
        1: WEP
        2: WPA_PSK
        !3: WPA2_PSK (I set it to this one)
        4: WPA_WPA2_PSK
        5: WPA2_ENTERPRISE
        6: WPA3_PSK
        7: WPA2_WPA3_PSK
      */
      if (!sendATCommandOK("CWSTARTSMART=3,3", 5000)) {
        cancel_repeating_timer(&timer);
        gpio_put(STATUS_LED_PIN, 0);

        printf("[Server]: Failed to start SmartConfig\n");
        sendATCommandOK("CWSTOPSMART", 1000);
        return false;
      }

      if(!sendATCommand("", 120 * 1000, "smartconfig connected wifi")) {
        cancel_repeating_timer(&timer);
        gpio_put(STATUS_LED_PIN, 0);

        printf("[Server]: Failed to connect to WiFi using SmartConfig\n");
        sendATCommandOK("CWSTOPSMART", 1000);
        return false;
      }

      cancel_repeating_timer(&timer);
      gpio_put(STATUS_LED_PIN, 0);

      printf("[Server]: SmartConfig Success\n");
      printf("[Server]: Waiting 6s to sync with the mobile device.\n");
      clearATBuffer(6000);

      if (!sendATCommandOK("CWSTOPSMART", 1000)) {
        printf("[Server]: Failed to stop to SmartConfig\n");
        return false;
      }
    } else {
      printf("[Server]: Connecting to last WiFi configuration\n");
      if(!sendATCommandOK("CWJAP", 30 * 1000)) {
        return false;
      }
    }

    const std::string ipAddress = getIp();
    if (ipAddress == "0.0.0.0" || ipAddress.empty()) {
      printf("[Server]: Failed to get IP address\n");
      return false;
    }

    printf("[Server]: Connected to WiFi\n");
    printf("[Server]: IP Address: '%s'\n", ipAddress.c_str());

    printf("[Server]: Setting the UTC time to RTC\n");
    if (!setupUTCTime()) {
      printf("[Server]: Failed to set the UTC time to RTC\n");
      return false;
    }

    if (1324512000000 > get_datetime_ms()) {
      printf("[Server]-[RTC]: Check Failed\n");
      return false;
    }

    if (!sendATCommandOK("CIPMUX=1", 2000)) {
      return false;
    }

    snprintf(sendBuffer, SENDBUFFERLEN, "CIPSERVER=1,%s", SOCKET_PORT);
    if (!sendATCommandOK(sendBuffer, 2000)) {
      return false;
    }

    printf("[Server]: Server running on port '%s'\n", SOCKET_PORT);

    if (!sendATCommandOK("CIPSTO=5", 1000, true)) {
      printf("[Server]: Failed to set client timeout\n");
    } else {
      printf("[Server]: Client timeout set to 5s\n");
    }

    gpio_put(STATUS_LED_PIN, 1);

    multicore_launch_core1(serve_clients);
    irq_set_exclusive_handler(SIO_IRQ_PROC0, core0_sio_irq);
    irq_set_enabled(SIO_IRQ_PROC0, true);

    mutex_exit(&m_esp);
    return true;
  } catch (...) {
    return false;
  }
}
