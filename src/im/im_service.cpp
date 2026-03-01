#include "lark/im/v1/im_service.h"

#include "lark/core/request.h"
#include "lark/core/token_manager.h"
#include "lark/core/transport.h"

#include <fstream>
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

bool ImService::DownloadImage(const std::string& image_key,
                              const std::string& output_path) {
  if (image_key.empty() || output_path.empty()) {
    std::cerr << "DownloadImage failed: empty image_key/output_path" << std::endl;
    return false;
  }
  lark::core::BaseRequest req;
  req.method = lark::core::HttpMethod::kGet;
  req.uri = "/open-apis/im/v1/images/:image_key";
  req.paths["image_key"] = image_key;
  req.token_type = lark::core::AccessTokenType::kTenant;
  req.headers["Content-Type"] = "application/json; charset=utf-8";

  lark::core::RequestOption opt;
  if (!lark::core::VerifyAndFillToken(cfg_, req, opt)) {
    std::cerr << "DownloadImage failed: cannot fetch tenant token" << std::endl;
    return false;
  }
  auto resp = lark::core::Transport::Execute(cfg_, req, opt);
  if (resp.status_code < 200 || resp.status_code >= 300) {
    std::cerr << "DownloadImage failed: status=" << resp.status_code
              << " resp=" << resp.content << std::endl;
    return false;
  }
  std::ofstream out(output_path, std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "DownloadImage failed: cannot open output file" << std::endl;
    return false;
  }
  out.write(resp.content.data(), static_cast<std::streamsize>(resp.content.size()));
  if (!out.good()) {
    std::cerr << "DownloadImage failed: write failed" << std::endl;
    return false;
  }
  return true;
}

bool ImService::UploadFile(const std::string& file_type,
                           const std::string& file_path,
                           const std::string& file_name,
                           std::string* file_key,
                           int duration_ms,
                           const std::string& content_type) {
  if (file_key == nullptr) {
    std::cerr << "UploadFile failed: file_key is null" << std::endl;
    return false;
  }
  if (file_type.empty() || file_path.empty() || file_name.empty()) {
    std::cerr << "UploadFile failed: empty file_type/file_path/file_name" << std::endl;
    return false;
  }
  lark::core::BaseRequest req;
  req.method = lark::core::HttpMethod::kPost;
  req.uri = "/open-apis/im/v1/files";
  req.token_type = lark::core::AccessTokenType::kTenant;
  req.headers["Content-Type"] = "multipart/form-data; boundary=---7MA4YWxkTrZu0gW";

  req.multipart.push_back({"file_type", file_type, "", "", "", false});
  if (duration_ms > 0) {
    req.multipart.push_back({"duration", std::to_string(duration_ms), "", "", "", false});
  }
  req.multipart.push_back({"file_name", file_name, "", "", "", false});
  lark::core::BaseRequest::MultipartField file_field;
  file_field.name = "file";
  file_field.file_path = file_path;
  file_field.filename = file_name;
  file_field.content_type = content_type;
  file_field.is_file = true;
  req.multipart.push_back(file_field);

  lark::core::RequestOption opt;
  if (!lark::core::VerifyAndFillToken(cfg_, req, opt)) {
    std::cerr << "UploadFile failed: cannot fetch tenant token" << std::endl;
    return false;
  }

  auto resp = lark::core::Transport::Execute(cfg_, req, opt);
  if (resp.status_code < 200 || resp.status_code >= 300) {
    std::cerr << "UploadFile failed: status=" << resp.status_code
              << " resp=" << resp.content << std::endl;
    return false;
  }
  nlohmann::json body = nlohmann::json::parse(resp.content, nullptr, false);
  if (body.is_discarded() || !body.contains("code") || body["code"].get<int>() != 0) {
    std::cerr << "UploadFile failed: invalid response " << resp.content << std::endl;
    return false;
  }
  if (!body.contains("data") || !body["data"].contains("file_key")) {
    std::cerr << "UploadFile failed: missing file_key" << std::endl;
    return false;
  }
  *file_key = body["data"]["file_key"].get<std::string>();
  return true;
}

bool ImService::UploadImage(const std::string& image_path,
                            std::string* image_key,
                            const std::string& image_type,
                            const std::string& content_type) {
  if (image_key == nullptr) {
    std::cerr << "UploadImage failed: image_key is null" << std::endl;
    return false;
  }
  if (image_path.empty()) {
    std::cerr << "UploadImage failed: empty image_path" << std::endl;
    return false;
  }
  lark::core::BaseRequest req;
  req.method = lark::core::HttpMethod::kPost;
  req.uri = "/open-apis/im/v1/images";
  req.token_type = lark::core::AccessTokenType::kTenant;
  req.headers["Content-Type"] = "multipart/form-data; boundary=---7MA4YWxkTrZu0gW";
  req.multipart.push_back({"image_type", image_type, "", "", "", false});

  lark::core::BaseRequest::MultipartField image_field;
  image_field.name = "image";
  image_field.file_path = image_path;
  image_field.content_type = content_type;
  image_field.is_file = true;
  req.multipart.push_back(image_field);

  lark::core::RequestOption opt;
  if (!lark::core::VerifyAndFillToken(cfg_, req, opt)) {
    std::cerr << "UploadImage failed: cannot fetch tenant token" << std::endl;
    return false;
  }
  auto resp = lark::core::Transport::Execute(cfg_, req, opt);
  if (resp.status_code < 200 || resp.status_code >= 300) {
    std::cerr << "UploadImage failed: status=" << resp.status_code
              << " resp=" << resp.content << std::endl;
    return false;
  }
  nlohmann::json body = nlohmann::json::parse(resp.content, nullptr, false);
  if (body.is_discarded() || !body.contains("code") || body["code"].get<int>() != 0) {
    std::cerr << "UploadImage failed: invalid response " << resp.content << std::endl;
    return false;
  }
  if (!body.contains("data") || !body["data"].contains("image_key")) {
    std::cerr << "UploadImage failed: missing image_key" << std::endl;
    return false;
  }
  *image_key = body["data"]["image_key"].get<std::string>();
  return true;
}

bool ImService::DownloadFile(const std::string& file_key,
                             const std::string& output_path) {
  if (file_key.empty() || output_path.empty()) {
    std::cerr << "DownloadFile failed: empty file_key/output_path" << std::endl;
    return false;
  }
  lark::core::BaseRequest req;
  req.method = lark::core::HttpMethod::kGet;
  req.uri = "/open-apis/im/v1/files/:file_key";
  req.paths["file_key"] = file_key;
  req.token_type = lark::core::AccessTokenType::kTenant;
  req.headers["Content-Type"] = "application/json; charset=utf-8";

  lark::core::RequestOption opt;
  if (!lark::core::VerifyAndFillToken(cfg_, req, opt)) {
    std::cerr << "DownloadFile failed: cannot fetch tenant token" << std::endl;
    return false;
  }
  auto resp = lark::core::Transport::Execute(cfg_, req, opt);
  if (resp.status_code < 200 || resp.status_code >= 300) {
    std::cerr << "DownloadFile failed: status=" << resp.status_code
              << " resp=" << resp.content << std::endl;
    return false;
  }
  std::ofstream out(output_path, std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "DownloadFile failed: cannot open output file" << std::endl;
    return false;
  }
  out.write(resp.content.data(), static_cast<std::streamsize>(resp.content.size()));
  if (!out.good()) {
    std::cerr << "DownloadFile failed: write failed" << std::endl;
    return false;
  }
  return true;
}

bool ImService::SendFileMessage(const std::string& receive_id,
                                const std::string& file_key,
                                const std::string& receive_id_type) {
  nlohmann::json content;
  content["file_key"] = file_key;
  return CreateMessage(receive_id, "file", content.dump(), receive_id_type);
}

bool ImService::SendAudioMessage(const std::string& receive_id,
                                 const std::string& file_key,
                                 const std::string& receive_id_type) {
  nlohmann::json content;
  content["file_key"] = file_key;
  return CreateMessage(receive_id, "audio", content.dump(), receive_id_type);
}

bool ImService::SendMediaMessage(const std::string& receive_id,
                                 const std::string& file_key,
                                 const std::string& image_key,
                                 const std::string& receive_id_type) {
  nlohmann::json content;
  content["file_key"] = file_key;
  content["image_key"] = image_key;
  return CreateMessage(receive_id, "media", content.dump(), receive_id_type);
}

}  // namespace lark::im::v1
