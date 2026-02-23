#pragma once

#include <string>

#include "lark/core/config.h"

namespace lark::im::v1 {

class ImService {
 public:
  explicit ImService(const lark::core::Config& cfg);

  bool CreateMessage(const std::string& receive_id,
                     const std::string& msg_type,
                     const std::string& content,
                     const std::string& receive_id_type = "chat_id");

 private:
  lark::core::Config cfg_;
};

}  // namespace lark::im::v1
