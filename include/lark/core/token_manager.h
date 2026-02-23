#pragma once

#include <string>

#include "lark/core/config.h"
#include "lark/core/request.h"

namespace lark::core {

class TokenManager {
 public:
  static std::string GetTenantToken(const Config& cfg, const RequestOption& opt);
};

bool VerifyAndFillToken(const Config& cfg, BaseRequest& req, RequestOption& opt);

}  // namespace lark::core
