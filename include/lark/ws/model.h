#pragma once

#include <optional>
#include <string>

namespace lark::ws {

struct ClientConfig {
  std::optional<int> reconnect_count;
  std::optional<int> reconnect_interval;
  std::optional<int> reconnect_nonce;
  std::optional<int> ping_interval;
};

struct Endpoint {
  std::string url;
  ClientConfig client_config;
};

}  // namespace lark::ws
