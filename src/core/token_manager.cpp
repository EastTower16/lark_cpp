#include "lark/core/token_manager.h"

#include <chrono>

#include <nlohmann/json.hpp>

#include "lark/core/request.h"
#include "lark/core/transport.h"

namespace lark::core {

namespace {
struct CachedToken {
  std::string token;
  std::chrono::steady_clock::time_point expire_at;
};

CachedToken g_cached_tenant_token;
}  // namespace

std::string TokenManager::GetTenantToken(const Config& cfg, const RequestOption&) {
  auto now = std::chrono::steady_clock::now();
  if (!g_cached_tenant_token.token.empty() && now < g_cached_tenant_token.expire_at) {
    return g_cached_tenant_token.token;
  }

  if (cfg.app_id.empty() || cfg.app_secret.empty()) {
    return "";
  }

  BaseRequest req;
  req.method = HttpMethod::kPost;
  req.uri = "/open-apis/auth/v3/tenant_access_token/internal";
  req.headers["Content-Type"] = "application/json; charset=utf-8";

  nlohmann::json body = {
      {"app_id", cfg.app_id},
      {"app_secret", cfg.app_secret},
  };
  req.body = body.dump();

  RequestOption opt;
  RawResponse raw = Transport::Execute(cfg, req, opt);
  if (raw.status_code < 200 || raw.status_code >= 300) {
    return "";
  }

  nlohmann::json resp = nlohmann::json::parse(raw.content, nullptr, false);
  if (resp.is_discarded()) {
    return "";
  }
  if (resp.contains("code") && resp["code"].is_number_integer() && resp["code"].get<int>() != 0) {
    return "";
  }
  if (!resp.contains("tenant_access_token")) {
    return "";
  }

  std::string token = resp["tenant_access_token"].get<std::string>();
  int expire = 0;
  if (resp.contains("expire") && resp["expire"].is_number_integer()) {
    expire = resp["expire"].get<int>();
  }

  g_cached_tenant_token.token = token;
  g_cached_tenant_token.expire_at = now + std::chrono::seconds(expire > 600 ? expire - 600 : expire);
  return token;
}

bool VerifyAndFillToken(const Config& cfg, BaseRequest& req, RequestOption& opt) {
  if (req.token_type == AccessTokenType::kNone) {
    return true;
  }
  if (req.token_type == AccessTokenType::kTenant) {
    if (opt.tenant_access_token.empty()) {
      opt.tenant_access_token = TokenManager::GetTenantToken(cfg, opt);
    }
    if (opt.tenant_access_token.empty()) {
      return false;
    }
    req.headers["Authorization"] = "Bearer " + opt.tenant_access_token;
    return true;
  }
  return false;
}

}  // namespace lark::core
