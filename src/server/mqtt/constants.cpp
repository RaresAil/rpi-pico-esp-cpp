#include <string>

enum class TYPE {
  REGISTER,
  PAIR,
  DATA,
};

enum class STATUS {
  SUCCESS,
};

std::string TYPES(const TYPE& type) {
  switch (type) {
    case TYPE::REGISTER:
      return "REGISTER";
    case TYPE::PAIR:
      return "PAIR";
    case TYPE::DATA:
      return "DATA";
    default:
      return "";
  }
}

std::string STATUSES(const STATUS& status) {
  switch (status) {
    case STATUS::SUCCESS:
      return "SUCCESS";
    default:
      return "";
  }
}