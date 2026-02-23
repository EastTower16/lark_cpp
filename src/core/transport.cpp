#include "lark/core/transport.h"

#include <curl/curl.h>

#include <sstream>

namespace lark::core {

static std::string BuildUrl(const std::string& domain,
                            const std::string& uri,
                            const std::map<std::string, std::string>& paths,
                            const std::map<std::string, std::string>& queries) {
  std::string url = domain + uri;
  for (const auto& [key, value] : paths) {
    std::string token = ":" + key;
    auto pos = url.find(token);
    if (pos != std::string::npos) {
      url.replace(pos, token.size(), value);
    }
  }
  if (!queries.empty()) {
    std::ostringstream oss;
    bool first = true;
    for (const auto& [key, value] : queries) {
      oss << (first ? "?" : "&") << key << "=" << value;
      first = false;
    }
    url += oss.str();
  }
  return url;
}

static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* buffer = static_cast<std::string*>(userdata);
  buffer->append(ptr, size * nmemb);
  return size * nmemb;
}

RawResponse Transport::Execute(const Config& cfg, const BaseRequest& req, const RequestOption&) {
  RawResponse resp;
  std::string url = BuildUrl(cfg.domain, req.uri, req.paths, req.queries);

  CURL* curl = curl_easy_init();
  if (!curl) {
    resp.status_code = 500;
    resp.content = "curl init failed";
    return resp;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, cfg.timeout_ms);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.content);

  struct curl_slist* headers = nullptr;
  for (const auto& [key, value] : req.headers) {
    std::string header = key + ": " + value;
    headers = curl_slist_append(headers, header.c_str());
  }

  if (!req.body.empty()) {
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
  }

  if (headers != nullptr) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  switch (req.method) {
    case HttpMethod::kPost:
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
      break;
    case HttpMethod::kPut:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
      break;
    case HttpMethod::kPatch:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
      break;
    case HttpMethod::kDelete:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
      if (!req.body.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
      }
      break;
    case HttpMethod::kGet:
    default:
      break;
  }

  CURLcode code = curl_easy_perform(curl);
  if (code != CURLE_OK) {
    resp.status_code = 500;
    resp.content = curl_easy_strerror(code);
  } else {
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    resp.status_code = static_cast<int>(status);
  }

  if (headers) {
    curl_slist_free_all(headers);
  }
  curl_easy_cleanup(curl);
  return resp;
}

}  // namespace lark::core
