#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace lark::core {

inline std::string Dump(const std::string& raw) {
  auto json = nlohmann::json::parse(raw, nullptr, false);
  return json.is_discarded() ? raw : json.dump();
}

}  // namespace lark::core
