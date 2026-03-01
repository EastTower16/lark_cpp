#include <cstdlib>
#include <iostream>

#include "lark/im/v1/im_service.h"
#include "lark/ws/event_dispatcher.h"
#include "lark/ws/ws_client.h"

int main() {
  lark::core::Config cfg;
  if (const char* app_id = std::getenv("LARK_APP_ID")) {
    cfg.app_id = app_id;
  }
  if (const char* app_secret = std::getenv("LARK_APP_SECRET")) {
    cfg.app_secret = app_secret;
  }

  lark::im::v1::ImService im(cfg);
  lark::ws::EventDispatcher dispatcher;
  dispatcher.OnMessageReceive([&im](const lark::im::v1::MessageEvent& evt) {
    std::cout << "receive message_id=" << evt.message_id << " content=" << evt.content << std::endl;
    std::string receive_id = evt.chat_id.empty() ? evt.sender_id : evt.chat_id;
    std::string receive_id_type = evt.chat_id.empty() ? "open_id" : "chat_id";
    if (!im.CreateMessage(receive_id, "text", R"({"text":"收到"})", receive_id_type)) {
      std::cout << "send reply failed" << std::endl;
    }

    // 示例：上传普通文件并发送
    // std::string file_key;
    // if (im.UploadFile("stream", "/path/to/file.txt", "file.txt", &file_key)) {
    //   im.SendFileMessage(receive_id, file_key, receive_id_type);
    // }

    // 示例：上传音频（需 duration）并发送
    // std::string audio_key;
    // if (im.UploadFile("opus", "/path/to/audio.opus", "audio.opus", &audio_key, 3000)) {
    //   im.SendAudioMessage(receive_id, audio_key, receive_id_type);
    // }

    // 示例：上传视频（需 duration）并发送封面
    // std::string video_key;
    // if (im.UploadFile("mp4", "/path/to/video.mp4", "video.mp4", &video_key, 3000)) {
    //   std::string image_key;
    //   if (im.UploadImage("/path/to/cover.jpg", &image_key)) {
    //     im.SendMediaMessage(receive_id, video_key, image_key, receive_id_type);
    //   }
    // }

    // 示例：下载文件
    // im.DownloadFile("file_v2_xxx", "/path/to/download.bin");

    // 示例：下载图片
    // im.DownloadImage("img_v2_xxx", "/path/to/download.jpg");
  });

  lark::ws::WsClient ws(cfg, dispatcher);
  ws.Start();

  return 0;
}
