#include <unordered_map>
#include <string>

enum class METHOD {
  POST,
  GET
};

enum class HTTP_STATUS {
  UNSUPPORTED_MEDIA_TYPE,
  INTERNAL_SERVER_ERROR,
  METHOD_NOT_ALLOWED,
  UNAUTHORIZED,
  BAD_REQUEST,
  NOT_FOUND,
  OK
};

std::string METHODS(const METHOD& method) {
  switch (method) {
    case METHOD::POST:
      return "POST";
    case METHOD::GET:
      return "GET";
    default:
      return "";
  }
}

std::string HTTP_STATUSES(const HTTP_STATUS& status) {
  switch (status) {
    case HTTP_STATUS::UNSUPPORTED_MEDIA_TYPE:
      return "415 Unsupported Media Type";
    case HTTP_STATUS::INTERNAL_SERVER_ERROR:
      return "500 Internal Server Error";
    case HTTP_STATUS::METHOD_NOT_ALLOWED:
      return "405 Method Not Allowed";
    case HTTP_STATUS::UNAUTHORIZED:
      return "401 Unauthorized";
    case HTTP_STATUS::BAD_REQUEST:
      return "400 Bad Request";
    case HTTP_STATUS::NOT_FOUND:
      return "404 Not Found";
    case HTTP_STATUS::OK:
      return "200 OK";
    default:
      return "500 Internal Server Error";
  }
}