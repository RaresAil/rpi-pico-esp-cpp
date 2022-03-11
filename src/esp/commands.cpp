#include "pico/util/datetime.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/rtc.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>
#include <time.h>
#include <tuple>

std::tuple<std::string, std::string> getIpMac() {
  try {
    if (!sendATCommandOK("CIFSR", 2000)) {
      return std::make_tuple("", "");
    }

    std::string ip = "";
    std::string mac = "";

    const char* ipBuff = strstr(responseBuffer, "STAIP");
    if (ipBuff != NULL) {
      ip = getParam(1, '"', '\0', ipBuff);
    }

    const char* macBuff = strstr(responseBuffer, "STAMAC");
    if (macBuff != NULL) {
      mac = getParam(1, '"', '\0', macBuff);;
    }

    return std::make_tuple(ip, mac);
  } catch (...) {
    printf("[Server]:[ERROR]: while getting IP\n");
    return std::make_tuple("", "");
  }
}

std::string getSDKVersion() {
  try {
    if (!sendATCommandOK("GMR", 3000)) {
      return "";
    }

    const char* buff = strstr(responseBuffer, "SDK");
    if (buff == NULL) {
      return "";
    }

    return getParam(1, ':', '\r', buff);
  } catch (...) {
    printf("[Server]:[ERROR]: while getting sdk version\n");
    return "";
  }
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
  try {
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
  } catch (...) {
    printf("[Server]:[ERROR]: while getting wifi state\n");
    return -1;
  }
}

bool setupUTCTime() {
  if (!sendATCommandOK("CIPSNTPCFG=1,0,\"pool.ntp.org\"", 2000)) {
    return false;
  }

  int retries = 10;
  do {
    sleep_ms(1000);

    sendATCommandOK("CIPSNTPTIME?", 2000);
    if (strstr(responseBuffer, "1970") == NULL) {
      std::string s_date = c_getParam(":", '\r');
      std::size_t doubleSpace = s_date.find("  ");
      while (doubleSpace != std::string::npos) {
        s_date.replace(doubleSpace + 1, 1, "0");
        doubleSpace = s_date.find("  ");
      }

      if (!s_date.empty()) {
        printf("[Server]-[INFO]: UTC time: '%s'\n", s_date.c_str());

        std::tm t = {};
        std::istringstream ss(s_date);
        ss >> std::get_time(&t, "%a %b %d %H:%M:%S %Y");

        if (!ss.fail()) {
          datetime_t dt = {
            .year  = (int16_t)(t.tm_year + 1900),
            .month = (int8_t)(t.tm_mon + 1),
            .day   = (int8_t)t.tm_mday,
            .dotw  = (int8_t)t.tm_wday, 
            .hour  = (int8_t)t.tm_hour,
            .min   = (int8_t)t.tm_min,
            .sec   = (int8_t)t.tm_sec
          };

          printf("[Server]: Setting RTC time to %d-%d-%d %d:%d:%d\n", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
          rtc_set_datetime(&dt);
          break;
        } else {
          printf("[Server]:[ERROR]: While parsing UTC time\n");
        }
      }
    } else {
      sleep_ms(4000);
    }
  } while (--retries);

  if (!retries) {
    return false;
  }

  return true;
}
