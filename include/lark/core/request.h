#pragma once

#include <map>
#include <string>
#include <vector>

namespace lark::core {

enum class HttpMethod { kGet, kPost, kPut, kPatch, kDelete };

enum class AccessTokenType { kNone, kTenant, kUser, kApp };

struct BaseRequest {
  HttpMethod method = HttpMethod::kGet;
  std::string uri;
  std::map<std::string, std::string> headers;
  std::map<std::string, std::string> queries;
  std::map<std::string, std::string> paths;
  std::string body;
  struct MultipartField {
    std::string name;
    std::string data;
    std::string file_path;
    std::string filename;
    std::string content_type;
    bool is_file = false;
  };
  std::vector<MultipartField> multipart;
  AccessTokenType token_type = AccessTokenType::kNone;
};

struct RequestOption {
  std::string tenant_access_token;
  std::string user_access_token;
  std::string app_access_token;
  std::string tenant_key;
  std::string app_ticket;
};

}  // namespace lark::core
