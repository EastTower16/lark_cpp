#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lark::ws {

struct FrameHeader {
  std::string key;
  std::string value;
};

struct Frame {
  uint64_t seq_id = 0;
  uint64_t log_id = 0;
  int32_t service = 0;
  int32_t method = 0;
  std::vector<FrameHeader> headers;
  std::string payload;
};

bool DecodeFrame(const std::string& data, Frame& frame);
std::string EncodeFrame(const Frame& frame);

}  // namespace lark::ws
