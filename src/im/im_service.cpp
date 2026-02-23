#include "lark/im/v1/im_service.h"

#include "lark/core/request.h"
#include "lark/core/token_manager.h"
#include "lark/core/transport.h"

#include <iostream>

#include <nlohmann/json.hpp>

namespace lark::im::v1 {

ImService::ImService(const lark::core::Config& cfg) : cfg_(cfg) {}

bool ImService::CreateMessage(const std::string& receive_id,
                              const std::string& msg_type,
                              const std::string& content,
                              const std::string& receive_id_type) {
  if (receive_id.empty()) {
    std::cerr << "CreateMessage failed: empty receive_id" << std::endl;
    return false;
  }
  lark::core::BaseRequest req;
  req.method = lark::core::HttpMethod::kPost;
  req.uri = "/open-apis/im/v1/messages";
  req.queries["receive_id_type"] = receive_id_type;
  req.token_type = lark::core::AccessTokenType::kTenant;
  nlohmann::json body;
  body["receive_id"] = receive_id;
  body["msg_type"] = msg_type;
  body["content"] = content;
  req.body = body.dump();

  lark::core::RequestOption opt;
  if (!lark::core::VerifyAndFillToken(cfg_, req, opt)) {
    std::cerr << "CreateMessage failed: cannot fetch tenant token" << std::endl;
    return false;
  }
  auto resp = lark::core::Transport::Execute(cfg_, req, opt);
  if (resp.status_code < 200 || resp.status_code >= 300) {
    std::cerr << "CreateMessage failed: status=" << resp.status_code
              << " resp=" << resp.content << std::endl;
    return false;
  }
  return true;
}

}  // namespace lark::im::v1
