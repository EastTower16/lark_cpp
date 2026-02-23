#pragma once

#include <string>

namespace lark::im::v1 {

struct MessageEvent {
  std::string message_id;
  std::string chat_id;
  std::string sender_id;
  std::string msg_type;
  std::string content;
};

}  // namespace lark::im::v1
