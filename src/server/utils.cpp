bool is_valid_method_with_no_body(const std::string& method) {
  if (
    method == METHODS(METHOD::GET)
  ) {
    return true;
  }

  return false;
}

/*
  In handler, the methods verified in this are also verified to be
  application/json.
*/
bool is_valid_method_with_body(const std::string& method) {
  if (
    method == METHODS(METHOD::POST)
  ) {
    return true;
  }

  return false;
}
