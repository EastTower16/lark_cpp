# 飞书IM api C++ 封装

本目录提供基于 **长连接(WebSocket)收消息 + HTTP 发消息** 的最小 C++ 框架

## 能力范围
- 长连接收消息事件（im.message.receive）
- HTTP 发送消息（CreateMessage）

## 依赖建议
- HTTP: libcurl
- JSON: nlohmann/json
- WebSocket: websocketpp (可替换为 Boost.Beast)
- Protobuf: 无需（已实现最小 proto2 编解码器）

## 目录结构
```
include/
  lark/core/        # 基础设施（Config/Request/Transport/Token）
  lark/ws/          # WS 客户端、帧与事件分发
  lark/im/v1/       # IM v1 事件模型与服务入口
src/
  core/
  ws/
  im/
```

## 快速使用（伪代码）
```cpp
#include "lark/im/v1/im_service.h"
#include "lark/ws/ws_client.h"

int main() {
  lark::core::Config cfg;
  cfg.app_id = "your_app_id";
  cfg.app_secret = "your_app_secret";

  lark::ws::EventDispatcher dispatcher;
  dispatcher.OnMessageReceive([](const lark::im::v1::MessageEvent& evt) {
    // TODO: 处理消息
  });

  lark::ws::WsClient ws(cfg, dispatcher);
  ws.Start();

  // 发送消息
  lark::im::v1::ImService im(cfg);
  im.CreateMessage("chat_id", "text", R"({"text":"hello"})");
}
```

