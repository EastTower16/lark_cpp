#pragma once

#include <string>

#include "lark/core/config.h"
#include "lark/core/request.h"
#include "lark/core/response.h"

namespace lark::core {

class Transport {
 public:
  static RawResponse Execute(const Config& cfg, const BaseRequest& req, const RequestOption& opt);
};

}  // namespace lark::core
