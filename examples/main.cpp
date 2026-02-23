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
  });

  lark::ws::WsClient ws(cfg, dispatcher);
  ws.Start();

  return 0;
}
