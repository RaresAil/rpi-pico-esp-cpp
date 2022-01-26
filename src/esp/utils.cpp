#include "pico/util/datetime.h"
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>
#include <time.h>

std::string get_datetime() {
  char datetimeStr[50];
  datetime_t dt;
  rtc_get_datetime(&dt);
  datetime_to_str(datetimeStr, sizeof(datetimeStr), &dt);
  return std::string(datetimeStr);
}

u_int64_t get_datetime_ms() {
  std::tm epoch_start;
  epoch_start.tm_sec = 0;
  epoch_start.tm_min = 0;
  epoch_start.tm_hour = 0;
  epoch_start.tm_mday = 1;
  epoch_start.tm_mon = 0;
  epoch_start.tm_year = 1970 - 1900;

  std::time_t basetime = std::mktime(&epoch_start);

  datetime_t dt;
  rtc_get_datetime(&dt);

  std::tm now = {};
  now.tm_year = dt.year - 1900;
  now.tm_mon = dt.month - 1;
  now.tm_mday = dt.day;
  now.tm_hour = dt.hour;
  now.tm_min = dt.min;
  now.tm_sec = dt.sec;

  const u_int64_t ms = std::difftime(std::mktime(&now), basetime);
  return ms * 1000;
}
