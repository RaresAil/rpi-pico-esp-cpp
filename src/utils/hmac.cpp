#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "time.h"
#include <string>

bool verify_sign(const std::string& sign, const std::string& data) {
  return sign == hmac<SHA256>(data, HMAC_SECRET);
}

std::string generate_sign(const std::string& data) {
  return hmac<SHA256>(data, HMAC_SECRET);
}
