#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "time.h"
#include <string>

const char VALID_ALG[] = "HS256";
std::string generate_signature(const uint32_t& ms_expire) {
  json payload = json::object();
  payload["exp"] = get_datetime_ms() + ms_expire;
  payload["sub"] = HOSTNAME;
  payload["alg"] = VALID_ALG;

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

bool verify_signature(const std::string& data) {
  try {
    const std::string b64_payload = getParam(0, '.', '\0', data);
    const std::string b64_signature = getParam(1, '.', '\0', data);

    if (b64_payload.empty() || b64_signature.empty()) {
      return false;
    }

    const std::string signature = base64_decode(b64_signature);
    const std::string expected_signature = hmac<SHA256>(b64_payload, HMAC_SECRET);
    if (expected_signature != signature) {
      return false;
    }

    const json payload = json::parse(base64_decode(b64_payload));
    const u_int64_t exp = payload["exp"].get<u_int64_t>();
    if (exp < get_datetime_ms()) {
      return false;
    }

    const std::string alg = payload["alg"].get<std::string>();
    if (alg != VALID_ALG) {
      return false;
    }

    const std::string sub = payload["sub"].get<std::string>();
    if (sub != HOSTNAME) {
      return false;
    }

    return true;
  } catch (...) {
    return false;
  }
}