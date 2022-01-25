#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "time.h"
#include <string>

std::string generate_signature(const uint32_t& ms_expire) {
  json payload = json::object();
  payload["exp"] = to_ms_since_boot(get_absolute_time()) + ms_expire;
  payload["sub"] = HOSTNAME;
  payload["alg"] = "HS256";

  std::string b64_payload = base64_encode(payload.dump(), true);
  const int payload_trim_pos = b64_payload.find_first_of('.');
  if (payload_trim_pos != std::string::npos) {
    b64_payload.erase(payload_trim_pos, b64_payload.size() - payload_trim_pos);
  }

  std::string signature = base64_encode(
    hmac<SHA256>(b64_payload, HMAC_SECRET), 
    true
  );

  const int sig_trim_pos = signature.find_first_of('.');
  if (sig_trim_pos != std::string::npos) {
    signature.erase(sig_trim_pos, signature.size() - sig_trim_pos);
  }

  return b64_payload + "." + signature;
}