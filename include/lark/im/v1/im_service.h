#pragma once

#include <string>

#include "lark/core/config.h"

namespace lark::im::v1 {

class ImService {
 public:
  explicit ImService(const lark::core::Config& cfg);

  bool CreateMessage(const std::string& receive_id,
                     const std::string& msg_type,
                     const std::string& content,
                     const std::string& receive_id_type = "chat_id");

  bool UploadFile(const std::string& file_type,
                  const std::string& file_path,
                  const std::string& file_name,
                  std::string* file_key,
                  int duration_ms = 0,
                  const std::string& content_type = "");

  bool UploadImage(const std::string& image_path,
                   std::string* image_key,
                   const std::string& image_type = "message",
                   const std::string& content_type = "");

  bool DownloadFile(const std::string& file_key,
                    const std::string& output_path);

  bool DownloadImage(const std::string& image_key,
                     const std::string& output_path);

  bool SendFileMessage(const std::string& receive_id,
                       const std::string& file_key,
                       const std::string& receive_id_type = "chat_id");

  bool SendAudioMessage(const std::string& receive_id,
                        const std::string& file_key,
                        const std::string& receive_id_type = "chat_id");

  bool SendMediaMessage(const std::string& receive_id,
                        const std::string& file_key,
                        const std::string& image_key,
                        const std::string& receive_id_type = "chat_id");

 private:
  lark::core::Config cfg_;
};

}  // namespace lark::im::v1
