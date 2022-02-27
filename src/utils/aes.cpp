#include <vector>
#include <random>
#include <climits>
#include <algorithm>
#include <functional>

#include "Crypto/BlockCipher.cpp"
#include "Crypto/AESCommon.cpp"
#include "Crypto/Crypto.cpp"
#include "Crypto/AES256.cpp"
#include "Crypto/Cipher.cpp"
#include "Crypto/CTR.cpp"

u_int8_t* randomBytes(u_int8_t size) {
  std::random_device r;
  std::default_random_engine randomEngine(r());
  std::uniform_int_distribution<int> uniformDist(CHAR_MIN, CHAR_MAX);

  std::vector<u_int8_t> data(size);
  std::generate(data.begin(), data.end(), [&uniformDist, &randomEngine] () {
    return uniformDist(randomEngine);
  });

  u_int8_t* buffer = new u_int8_t[size];

  for (u_int8_t i = 0; i < size; i++) {
    buffer[i] = u_int8_t(data[i]);
  }

  return buffer;
}

std::string encrypt_256_aes_ctr(const std::string& value) {
  u_int8_t plaintext[value.length() + 1];

  memcpy(plaintext, value.c_str(), value.length());
  plaintext[value.length()] = '\0';

  u_int8_t key[32];
  memcpy(key, base64_decode(std::string(AES256CTR_KEY)).c_str(), 32);

  const u_int8_t* iv = randomBytes(16);

  CTR<AES256> ctr;
  ctr.clear();
  ctr.setKey(key, 32);
  ctr.setIV(iv, 16);
  ctr.setCounterSize(4);

  u_int8_t output[value.length() + 1];
  ctr.encrypt(output, plaintext, value.length());
  output[value.length()] = '\0';

  return base64_encode(std::string(iv, iv + 16) + std::string(output, output + value.length()));
}

std::string decrypt_256_aes_ctr(const std::string& value) {
  const std::string decoded_value = base64_decode(value);

  const std::string s_iv = decoded_value.substr(0, 16);
  const std::string raw = decoded_value.substr(16, decoded_value.length());

  u_int8_t chifertext[raw.length() + 1];

  memcpy(chifertext, raw.c_str(), raw.length());
  chifertext[raw.length()] = '\0';

  u_int8_t iv[16];
  memcpy(iv, s_iv.c_str(), 16);

  u_int8_t key[32];
  memcpy(key, base64_decode(std::string(AES256CTR_KEY)).c_str(), 32);

  CTR<AES256> ctr;
  ctr.clear();
  ctr.setKey(key, 32);
  ctr.setIV(iv, 16);
  ctr.setCounterSize(4);
  
  u_int8_t output[raw.length() + 1];
  ctr.encrypt(output, chifertext, raw.length());
  output[raw.length()] = '\0';

  return std::string(output, output + raw.length());
}
