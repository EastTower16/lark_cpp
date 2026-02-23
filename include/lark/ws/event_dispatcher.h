#pragma once

#include <functional>

#include "lark/im/v1/model.h"

namespace lark::ws {

class EventDispatcher {
 public:
  using MessageHandler = std::function<void(const lark::im::v1::MessageEvent&)>;

  void OnMessageReceive(MessageHandler handler) { message_handler_ = std::move(handler); }

  void DispatchMessageReceive(const lark::im::v1::MessageEvent& event) const {
    if (message_handler_) {
      message_handler_(event);
    }
  }

 private:
  MessageHandler message_handler_;
};

}  // namespace lark::ws
