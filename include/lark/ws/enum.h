#pragma once

namespace lark::ws {

enum class FrameType {
  kControl = 0,
  kData = 1,
};

enum class MessageType {
  kEvent,
  kCard,
  kPing,
  kPong,
};

}  // namespace lark::ws
