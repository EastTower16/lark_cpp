#pragma once

#include <string>

namespace lark::core {

struct Config {
  std::string app_id;
  std::string app_secret;
  std::string domain = "https://open.feishu.cn";
  long timeout_ms = 10000;
};

}  // namespace lark::core
