#pragma once

#include <map>
#include <string>

namespace lark::core {

struct RawResponse {
  int status_code = 0;
  std::map<std::string, std::string> headers;
  std::string content;
};

struct BaseResponse {
  int code = -1;
  std::string msg;
  RawResponse raw;
};

}  // namespace lark::core
