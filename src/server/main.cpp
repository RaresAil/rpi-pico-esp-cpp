#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>
#include <tuple>

std::string IP = "";
std::string MAC = "";

#include "mqtt/main.cpp"

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

    snprintf(sendBuffer, SENDBUFFERLEN, "CWHOSTNAME=\"%s\"", HOSTNAME);
    if (!sendATCommandOK(sendBuffer, 2000)) {
      return false;
    }
    
    printf("[Server]: Hostname set to %s\n", HOSTNAME);

    const int wifiState = getWifiState();
    printf("[Server]: Network State: %d\n", wifiState);

    if (wifiState <= 0 || wifiState == 4) {
      struct repeating_timer timer;
      service.update_newtwork("PAIR");
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

      service.update_newtwork("...");
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
      service.update_newtwork("...");
      printf("[Server]: Connecting to last WiFi configuration\n");
      if(!sendATCommandOK("CWJAP", 30 * 1000)) {
        return false;
      }
    }

    const std::tuple<std::string, std::string> ipMac = getIpMac();
    IP = std::get<0>(ipMac);
    MAC = std::get<1>(ipMac);

    if (IP == "0.0.0.0" || IP.empty() || MAC.empty()) {
      printf("[Server]: Failed to get IP/MAC address\n");
      return false;
    }

    service.update_newtwork("RTC");

    printf("[Server]: Connected to WiFi\n");
    printf("[Server]: IP Address: '%s'\n", IP.c_str());
    printf("[Server]: MAC Address: '%s'\n", MAC.c_str());

    printf("[Server]: Setting the UTC time to RTC\n");
    if (!setupUTCTime()) {
      printf("[Server]: Failed to set the UTC time to RTC\n");
      return false;
    }

    if (1324512000000 > get_datetime_ms()) {
      printf("[Server]-[RTC]: Check Failed\n");
      return false;
    }

    service.update_newtwork("BIND");

    const bool result = connect_to_mqtt();
    if (!result) {
      printf("[Server]: Failed to connect to MQTT server\n");
      sendATCommandOK("MQTTCLEAN=0", 250, true);
      return false;
    }

    service.update_newtwork("ON");

    gpio_put(STATUS_LED_PIN, 1);
    mutex_exit(&m_esp);
    return true;
  } catch (...) {
    return false;
  }
}
