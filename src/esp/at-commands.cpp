#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <sstream>
#include <string>

char responseBuffer[RESPONSEBUFFERLEN];

int getATdata(int buffPtr) {
  bool tooLong = false;
  while (uart_is_readable(UART_ID)) {
    responseBuffer[buffPtr] = uart_getc(UART_ID);

    if (buffPtr >= RESPONSEBUFFERLEN - 1) {
      tooLong = true; 
    } else {
      buffPtr++;
    }
  }

  responseBuffer[buffPtr] = 0;
  return (tooLong) ? -1 : buffPtr;
}

bool sendATCommand(const char* command, int64_t allowTimeMs, const char* successMsg, const bool& surpressOutput = false) {
  absolute_time_t start = get_absolute_time();
  allowTimeMs *= MICROS_MS;

  char sendBuffer[SENDBUFFERLEN];
  bool runCommand = true;
  int buffPtr = 0;
  
  if (strlen(command) > 0) {
    snprintf(sendBuffer, SENDBUFFERLEN, "AT+%s\r\n", command);
  }

  memset(responseBuffer, 0, RESPONSEBUFFERLEN);

  while (absolute_time_diff_us(start, get_absolute_time()) < allowTimeMs) {
    if (runCommand && strlen(command) > 0) {
      uart_puts(UART_ID, sendBuffer); 
      printf("[AT-Command]-[%s]: Sending Command\n", command);
  
      runCommand = false;
      buffPtr = 0;
    }

    buffPtr = getATdata(buffPtr);

    if (buffPtr == -1) {
      if (!surpressOutput) {
        printf("[AT-Command]-[%s]: Response to command is too long: (%s)\n", command, responseBuffer);
      }

      return false;
    }

    if ((strlen(successMsg) > 0) && (strstr(responseBuffer, successMsg) != NULL)) {
      return true;
    }

    if (strstr(responseBuffer, "busy p...") != NULL) {
      if (!surpressOutput) {
        printf("[AT-Command]-[%s]: ESP8266 busy, retry command\n", command);
      }

      memset(responseBuffer, 0, RESPONSEBUFFERLEN);
      sleep_ms(100);
      runCommand = true;
    }

    if (strstr(responseBuffer, "link is not valid") != NULL) {
      return false;
    }
  }

  if (strlen(successMsg) > 0) {
    if (buffPtr > 0) {
      if (strstr(responseBuffer, "busy p...") != NULL) {
        if (!surpressOutput) {
          printf("[AT-Command]-[%s]: Timed out waiting on ESP8266 busy\n", command);
        }

        return false;
      }

      if (strlen(responseBuffer) == 0) {
        if (!surpressOutput) {
          printf("[AT-Command]-[%s]: No ESP8266 response\n", command);
        }

        return false;
      }

      if (strstr(responseBuffer, "busy s...") != NULL) {
        if (!surpressOutput) {
          printf("[AT-Command]-[%s]: ESP8266 unable to receive\n", command);
        }

        return false;
      }

      if (strstr(responseBuffer, "ERROR") != NULL) {
        if (!surpressOutput) {
          printf("[AT-Command]-[%s]: ESP8266 Error\n", command);
        }

        return false;
      }

      if (!surpressOutput) {
        printf("[AT-Command]-[%s]: Command got unexpected response\n", command);
      }

      return false;
    }

    if (!surpressOutput) {
      printf("[AT-Command]-[%s]: Timed out waiting for ESP8266 response\n", command);
    }

    return false;
  }

  return (buffPtr > 0) ? true : false;
}

bool sendATCommandOK(const char* command, const int64_t allowTimeMs, const bool& surpressOutput = false) {
  return sendATCommand(command, allowTimeMs, "OK", surpressOutput);
}

bool clearATBuffer(const int64_t allowTimeMs) {
  return sendATCommand("", allowTimeMs, "");
}

const std::string extractJson(
  const char* from
) {
  const char* c_start = strstr(from, "{");
  if (c_start == NULL) {
    return "";
  }

  const std::string json(c_start);

  const int last = json.find_last_of("}");
  if (last == std::string::npos) {
    return "";
  }

  return json.substr(0, last + 1);
}

std::string getParam(
  const int pos, 
  const char start, 
  const char end = '\0',
  const std::string from = responseBuffer
) {
  std::string line;
  std::istringstream iss_input(from);
  std::getline(iss_input, line, start);

  for (int i = 0; i < pos; i++) {
    std::getline(iss_input, line, start);
  }

  if (end != '\0') {
    std::getline(std::istringstream(line), line, end);
  }

  return line;
}


static const char serverError[] = "HTTP/1.0 500 Internal Server Error\r\n\r\n";

void sendResponse(const char* id, const char* statusCode, const char* responseData) {
  char sendBuffer[SENDBUFFERLEN];
  char data[SENDBUFFERLEN];

  snprintf(
    data, SENDBUFFERLEN, 
    "HTTP/1.0 %s\r\n%s\r\n%s%s",
    statusCode, 
    "Access-Control-Allow-Origin: *\r\nHost: RPi-Pico",
    "Content-type: application/json\r\n\r\n", 
    responseData, 
    "\r\n"
  );
  snprintf(sendBuffer, SENDBUFFERLEN, "CIPSEND=%s,%d", id, strlen(data));
  sendATCommand(sendBuffer, 250, ">", true);
  uart_puts(UART_ID, data);
  clearATBuffer(250);

  snprintf(sendBuffer, SENDBUFFERLEN, "CIPCLOSE=%s", id);
  sendATCommandOK(sendBuffer, 500, true);
}
