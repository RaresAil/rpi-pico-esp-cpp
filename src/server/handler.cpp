#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>

void close_connection(
  const std::string& linkId, 
  const HTTP_STATUS& status
) {
  try {
    std::string message = HTTP_STATUSES(status);
    std::string s_status;
    std::istringstream iss_input(message);
    std::getline(iss_input, s_status, ' ');
    message.erase(0, s_status.length() + 1);

    printf(
      "[Server]: Closing connection for client '%s' with status '%s' and message '%s'\n", 
      linkId.c_str(), 
      s_status.c_str(), 
      message.c_str()
    );

    json jsn = json::object();
    jsn["message"] = message;

    sendResponse(
      linkId.c_str(), 
      HTTP_STATUSES(status).c_str(),
      jsn.dump().c_str()
    );
  } catch (...) {
    printf("[Server]:[ERROR]: while closing connection\n");
  }
}

// Executed on core 1
void handle_request(const std::string& request) {
  const std::string linkId = getParam(1, ',', '\0', request);
  const std::string dataLen = getParam(2, ',', ':', request);

  std::string httpLine = getParam(2, ',', '\n', request);
  httpLine.erase(0, dataLen.length() + 1);

  try {
    while (httpLine.find("\r") != std::string::npos) {
      httpLine.erase(httpLine.find("\r"), 1);
    }

    const std::string method = getParam(0, ' ', '\0', httpLine);
    const std::string route = getParam(1, ' ', '\0', httpLine);

    bool isValidContentType = false;
    const char* contentType = strstr(request.c_str(), "Content-Type");
    if (contentType != NULL) {
      const std::string sContentType = getParam(0, '\n', '\0', contentType);
      isValidContentType = sContentType.find("application/json") != std::string::npos;
    }

    if (
      is_valid_method_with_no_body(method) || (
        is_valid_method_with_body(method) &&
        isValidContentType
      )
    ) {
      printf(
        "[Server]: Incoming '%s' request on '%s' from client '%s'\n", 
        method.c_str(), 
        route.c_str(),
        linkId.c_str()
      );

      const char* _requestBody = strstr(request.c_str(), "{");
      if (
        is_valid_method_with_body(method) &&
        _requestBody == NULL
      ) {
        close_connection(linkId, HTTP_STATUS::BAD_REQUEST);
      } else {
        if (multicore_fifo_wready()) {
          multicore_fifo_push_blocking((uintptr_t)linkId.c_str());
          multicore_fifo_push_blocking((uintptr_t)method.c_str());
          multicore_fifo_push_blocking((uintptr_t)route.c_str());

          if (is_valid_method_with_body(method)) {
            multicore_fifo_push_blocking((uintptr_t)_requestBody);
          }
        }
      }
    } else {
      if (is_valid_method_with_body(method)) {
        close_connection(linkId, HTTP_STATUS::UNSUPPORTED_MEDIA_TYPE);
      } else {
        close_connection(linkId, HTTP_STATUS::METHOD_NOT_ALLOWED);
      }
    }
  } catch (...) {
    printf("[Server]:[ERROR]: while handling request\n");
    close_connection(linkId, HTTP_STATUS::INTERNAL_SERVER_ERROR);
  }
}


// Executed on core 0
void handle_get_request(const std::string& linkId, const std::string& route) {
  // TODO: Implement get requests

  if (route == "/status") {
    json jsn = json::object();
    jsn["message"] = "OK";

    sendResponse(
      linkId.c_str(), 
      HTTP_STATUSES(HTTP_STATUS::OK).c_str(), 
      jsn.dump().c_str()
    );
    return;
  }

  close_connection(linkId, HTTP_STATUS::NOT_FOUND);
}

// Executed on core 0
void handle_post_request(
  const std::string& linkId, 
  const std::string& route,
  const std::string& body
) {
  bool error = false;

  try {
    json data = json::parse(body);

    // TODO: Implement POST logic

    close_connection(linkId, HTTP_STATUS::NOT_FOUND);
  } catch (json::parse_error &e) {
    printf("[Server]: Handler 'Parse Error': %s\n", e.what());
    error = true;
  } catch (json::type_error &e) {
    printf("[Server]: Handler 'Type Error': %s\n", e.what());
    error = true;
  } catch (json::invalid_iterator &e) {
    printf("[Server]: Handler 'Invalid Iterator Error': %s\n", e.what());
    error = true;
  } catch (json::out_of_range &e) {
    printf("[Server]: Handler 'Out of Range Error': %s\n", e.what());
    error = true;
  } catch (json::other_error &e) {
    printf("[Server]: Handler 'Other Error': %s\n", e.what());
    error = true;
  } catch (...) {
    printf("[Server]: Handler 'Unknown Error'\n");
    close_connection(linkId, HTTP_STATUS::INTERNAL_SERVER_ERROR);
    return;
  }

  if (error) {
    close_connection(linkId, HTTP_STATUS::BAD_REQUEST);
  }
}