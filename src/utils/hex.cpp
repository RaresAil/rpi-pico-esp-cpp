#include <bits/stdc++.h>

unsigned char hexval(unsigned char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  } else if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  } else {
    return '\0';
  }
}

std::string hex2ascii(const std::string& in) {
  std::string output;
  output.reserve(in.length() / 2);

  for (std::string::const_iterator p = in.begin(); p != in.end(); p++) {
    unsigned char c = hexval(*p);
    p++;

    if (p == in.end()) {
      break;
    }

    c = (c << 4) + hexval(*p);
    output.push_back(c);
  }

  return output;
}
