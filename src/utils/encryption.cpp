#include <bits/stdc++.h>

std::string encrypt(std::string s) {
	int l = s.length();
	int b = ceil(sqrt(l));
	int a = floor(sqrt(l));
	std::string encrypted;

	if (b * a < l) {
		if (std::min(b, a) == b) {
			b = b + 1;
		} else {
			a = a + 1;
		}
	}

	char arr[a][b];
	memset(arr, ' ', sizeof(arr));
	int k = 0;
	
	for (int j = 0; j < a; j++) {
		for (int i = 0; i < b; i++) {
			if (k < l){
				arr[j][i] = s[k];
			}

			k++;
		}
	}

	for (int j = 0; j < b; j++) {
		for (int i = 0; i < a; i++) {
			encrypted = encrypted + arr[i][j];
		}
	}

	return encrypted;
}

std::string decrypt(std::string s) {
	int l = s.length();
	int b = ceil(sqrt(l));
	int a = floor(sqrt(l));
	std::string decrypted;

	char arr[a][b];
	memset(arr, ' ', sizeof(arr));
	int k = 0;
	
	for (int j = 0; j < b; j++) {
		for (int i = 0; i < a; i++) {
			if (k < l) {
				arr[j][i] = s[k];
			}

			k++;
		}
	}

	for (int j = 0; j < a; j++) {
		for (int i = 0; i < b; i++) {
			decrypted = decrypted +	arr[i][j];
		}
	}

	return decrypted;
}

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
