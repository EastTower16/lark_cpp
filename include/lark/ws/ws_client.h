#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <queue>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include "lark/core/config.h"
#include "lark/ws/event_dispatcher.h"
#include "lark/ws/model.h"

namespace lark::ws {

class WsClient {
 public:
  WsClient(const lark::core::Config& cfg, EventDispatcher& dispatcher);
  ~WsClient();
  void Start();

 private:
  Endpoint FetchEndpoint();
  void Connect(const std::string& url);
  void StartPingLoop();
  void Stop();
  void DoRead();
  void DoWrite(const std::string& payload);

  lark::core::Config cfg_;
  EventDispatcher& dispatcher_;
  std::atomic<bool> running_{false};
  std::atomic<bool> connected_{false};
  std::thread ping_thread_;
  int ping_interval_sec_ = 120;
  int reconnect_count_ = -1;
  int reconnect_interval_sec_ = 120;
  int service_id_ = 0;

  boost::asio::io_context ioc_;
  boost::asio::ssl::context ssl_ctx_{boost::asio::ssl::context::tlsv12_client};
  std::unique_ptr<boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>> ws_;
  boost::beast::flat_buffer buffer_;

  std::mutex write_mutex_;
};

}  // namespace lark::ws
