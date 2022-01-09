#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

std::string getIp() {
  if (!sendATCommandOK("CIFSR", 2000)) {
    return "";
  }

  const char* buff = strstr(responseBuffer, "STAIP");
  if (buff == NULL) {
    return "";
  }

  return getParam(1, '"', '\0', buff);
}

std::string getVersion() {
  if (!sendATCommandOK("GMR", 1000)) {
    return "";
  }

  const char* buff = strstr(responseBuffer, "SDK");
  if (buff == NULL) {
    return "";
  }

  return getParam(1, ':', '\n', buff);
}

/*
 * -1: Error
 * 0: ESP station has not started any Wi-Fi connection.
 * 1: ESP station has connected to an AP, but does not get an IPv4 address yet.
 * 2: ESP station has connected to an AP, and got an IPv4 address.
 * 3: ESP station is in Wi-Fi connecting or reconnecting state.
 * 4: ESP station is in Wi-Fi disconnected state.
 */
int getWifiState() {
  if (!sendATCommandOK("CWSTATE?", 1000)) {
    return -1;
  }

  const char* buff = strstr(responseBuffer, "+CWSTATE:");
  if (buff == NULL) {
    return -1;
  }

  const std::string s_state = getParam(1, ':', ',', buff);
  if (s_state.empty()) {
    return -1;
  }

  return stoi(s_state);
}