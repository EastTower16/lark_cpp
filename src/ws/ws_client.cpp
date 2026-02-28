#include "lark/ws/ws_client.h"

#include <iostream>
#include <regex>
#include <string>

#include <nlohmann/json.hpp>

#include "lark/core/request.h"
#include "lark/core/transport.h"
#include "lark/ws/const.h"
#include "lark/ws/enum.h"
#include "lark/ws/frame_codec.h"

namespace lark::ws {

namespace {

int ParseQueryInt(const std::string& url, const std::string& key) {
  std::regex re(key + "=([0-9]+)");
  std::smatch match;
  if (std::regex_search(url, match, re) && match.size() > 1) {
    return std::stoi(match[1].str());
  }
  return 0;
}

Frame NewPingFrame(int service_id) {
  Frame frame;
  frame.method = static_cast<int>(FrameType::kControl);
  frame.headers.push_back({kHeaderType, "ping"});
  return frame;
}

std::string ParseHost(const std::string& url) {
  std::regex re("wss?://([^/:]+)");
  std::smatch match;
  if (std::regex_search(url, match, re) && match.size() > 1) {
    return match[1].str();
  }
  return "";
}

std::string ParsePort(const std::string& url) {
  std::regex re("wss?://[^/:]+:([0-9]+)");
  std::smatch match;
  if (std::regex_search(url, match, re) && match.size() > 1) {
    return match[1].str();
  }
  if (url.find("wss://") == 0) {
    return "443";
  }
  return "80";
}

}  // namespace

WsClient::WsClient(const lark::core::Config& cfg, EventDispatcher& dispatcher)
    : cfg_(cfg), dispatcher_(dispatcher) {
  ssl_ctx_.set_default_verify_paths();
  ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_none);
}

WsClient::~WsClient() {
  Stop();
}

void WsClient::Start() {
  int attempts = 0;
  running_ = true;
  while (running_) {
    Endpoint endpoint = FetchEndpoint();
    if (endpoint.client_config.ping_interval) {
      ping_interval_sec_ = *endpoint.client_config.ping_interval;
    }
    if (endpoint.client_config.reconnect_count) {
      reconnect_count_ = *endpoint.client_config.reconnect_count;
    }
    if (endpoint.client_config.reconnect_interval) {
      reconnect_interval_sec_ = *endpoint.client_config.reconnect_interval;
    }

    if (!endpoint.url.empty()) {
      Connect(endpoint.url);
    }
    if (reconnect_count_ >= 0 && ++attempts > reconnect_count_) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(reconnect_interval_sec_));
  }
}

Endpoint WsClient::FetchEndpoint() {
  lark::core::BaseRequest req;
  req.method = lark::core::HttpMethod::kPost;
  req.uri = "/callback/ws/endpoint";
  req.headers["locale"] = "zh";

  nlohmann::json body = {
      {"AppID", cfg_.app_id},
      {"AppSecret", cfg_.app_secret},
  };
  req.body = body.dump();

  lark::core::RequestOption opt;
  auto resp = lark::core::Transport::Execute(cfg_, req, opt);
  
  Endpoint endpoint;
  if (resp.status_code < 200 || resp.status_code >= 300) {
    return endpoint;
  }

  nlohmann::json json = nlohmann::json::parse(resp.content, nullptr, false);
  if (json.is_discarded() || !json.contains("data")) {
    return endpoint;
  }

  auto data = json["data"];
  if (data.contains("URL")) {
    endpoint.url = data["URL"].get<std::string>();
  } else if (data.contains("url")) {
    endpoint.url = data["url"].get<std::string>();
  }

  if (data.contains("ClientConfig")) {
    auto conf = data["ClientConfig"];
    if (conf.contains("ReconnectCount")) {
      endpoint.client_config.reconnect_count = conf["ReconnectCount"].get<int>();
    }
    if (conf.contains("ReconnectInterval")) {
      endpoint.client_config.reconnect_interval = conf["ReconnectInterval"].get<int>();
    }
    if (conf.contains("PingInterval")) {
      endpoint.client_config.ping_interval = conf["PingInterval"].get<int>();
    }
  }
  return endpoint;
}

void WsClient::Connect(const std::string& url) {
  service_id_ = ParseQueryInt(url, "service_id");

  try {
    std::string host = ParseHost(url);
    std::string port = ParsePort(url);

    boost::asio::ip::tcp::resolver resolver(ioc_);
    auto const results = resolver.resolve(host, port);

    ws_ = std::make_unique<boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>>(
        ioc_, ssl_ctx_);
    
    if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host.c_str())) {
      return;
    }

    boost::asio::connect(ws_->next_layer().next_layer(), results.begin(), results.end());
    ws_->next_layer().handshake(boost::asio::ssl::stream_base::client);

    ws_->set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& req) {
            req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        }));

    ws_->handshake(host, url);
    ws_->binary(true);

    connected_ = true;
    StartPingLoop();

    std::unordered_map<std::string, std::vector<std::string>> chunk_map;

    auto handle_event_payload = [this](const std::string& payload) {
      nlohmann::json json = nlohmann::json::parse(payload, nullptr, false);
      if (json.is_discarded()) {
        return;
      }
      lark::im::v1::MessageEvent event;
      if (json.contains("event")) {
        auto evt = json["event"];
        if (evt.contains("message")) {
          auto message = evt["message"];
          if (message.contains("message_id")) {
            event.message_id = message["message_id"].get<std::string>();
          }
          if (message.contains("chat_id")) {
            event.chat_id = message["chat_id"].get<std::string>();
          }
          if (message.contains("msg_type")) {
            event.msg_type = message["msg_type"].get<std::string>();
          }
          if (message.contains("content")) {
            event.content = message["content"].get<std::string>();
          }
          if (message.contains("sender") && message["sender"].contains("sender_id")) {
            auto sender_id = message["sender"]["sender_id"];
            if (sender_id.is_string()) {
              event.sender_id = sender_id.get<std::string>();
            } else if (sender_id.is_object()) {
              if (sender_id.contains("open_id") && sender_id["open_id"].is_string()) {
                event.sender_id = sender_id["open_id"].get<std::string>();
              } else if (sender_id.contains("user_id") && sender_id["user_id"].is_string()) {
                event.sender_id = sender_id["user_id"].get<std::string>();
              } else if (sender_id.contains("union_id") && sender_id["union_id"].is_string()) {
                event.sender_id = sender_id["union_id"].get<std::string>();
              }
            }
          }
        }
      }
      dispatcher_.DispatchMessageReceive(event);
    };

    while (connected_ && running_) {
      buffer_.consume(buffer_.size());
      boost::system::error_code ec;
      ws_->read(buffer_, ec);

      if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::eof) {
        break;
      }
      if (ec) {
        std::cerr << "WebSocket read error: " << ec.message() << std::endl;
        break;
      }

      std::string data = boost::beast::buffers_to_string(buffer_.data());
      Frame frame;
      if (!DecodeFrame(data, frame)) {
        continue;
      }

      FrameType frame_type = static_cast<FrameType>(frame.method);
      if (frame_type == FrameType::kControl) {
        std::string type;
        for (const auto& header : frame.headers) {
          if (header.key == kHeaderType) {
            type = header.value;
            break;
          }
        }
        if (type == "ping") {
          continue;
        }
        if (type == "pong" && !frame.payload.empty()) {
          nlohmann::json conf = nlohmann::json::parse(frame.payload, nullptr, false);
          if (!conf.is_discarded() && conf.contains("PingInterval")) {
            ping_interval_sec_ = conf["PingInterval"].get<int>();
          }
        }
        continue;
      }

      std::string msg_id;
      int sum = 1;
      int seq = 0;
      std::string type;
      for (const auto& header : frame.headers) {
        if (header.key == kHeaderMessageId) {
          msg_id = header.value;
        } else if (header.key == kHeaderSum) {
          sum = std::stoi(header.value);
        } else if (header.key == kHeaderSeq) {
          seq = std::stoi(header.value);
        } else if (header.key == kHeaderType) {
          type = header.value;
        }
      }

      std::string payload = frame.payload;
      if (sum > 1 && !msg_id.empty()) {
        auto& chunks = chunk_map[msg_id];
        if (chunks.empty()) {
          chunks.resize(sum);
        }
        if (seq >= 0 && seq < static_cast<int>(chunks.size())) {
          chunks[seq] = payload;
        }
        bool complete = true;
        std::string merged;
        for (const auto& chunk : chunks) {
          if (chunk.empty()) {
            complete = false;
            break;
          }
          merged += chunk;
        }
        if (!complete) {
          continue;
        }
        payload = merged;
        chunk_map.erase(msg_id);
      }

      if (type == "event") {
        handle_event_payload(payload);
      }

      nlohmann::json resp_json = {
          {"code", 200},
      };
      Frame resp_frame = frame;
      resp_frame.payload = resp_json.dump();
      std::string resp_payload = EncodeFrame(resp_frame);
      DoWrite(resp_payload);
    }

  } catch (std::exception const& e) {
    std::cerr << "WebSocket connect error: " << e.what() << std::endl;
  }
  
  StopPingLoop();
}

void WsClient::DoWrite(const std::string& payload) {
  if (!connected_ || !ws_) return;
  try {
    std::lock_guard<std::mutex> lock(write_mutex_);
    ws_->write(boost::asio::buffer(payload));
  } catch (std::exception const& e) {
    std::cerr << "WebSocket write error: " << e.what() << std::endl;
    connected_ = false;
  }
}

void WsClient::StartPingLoop() {
  if (ping_thread_.joinable()) {
    return;
  }
  ping_thread_ = std::thread([this]() {
    while (running_) {
      if (!connected_) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(ping_interval_sec_));
      if (!running_ || !connected_) {
        break;
      }
      if (service_id_ <= 0) {
        continue;
      }
      Frame ping = NewPingFrame(service_id_);
      std::string payload = EncodeFrame(ping);
      DoWrite(payload);
    }
  });
}

void WsClient::StopPingLoop() {
  connected_ = false;
  if (ping_thread_.joinable()) {
    ping_thread_.join();
  }
}

void WsClient::Stop() {
  running_ = false;
  StopPingLoop();
  if (ws_) {
    try {
      boost::system::error_code ec;
      ws_->close(boost::beast::websocket::close_code::normal, ec);
    } catch (...) {}
  }
  if (ping_thread_.joinable()) {
    ping_thread_.join();
  }
}

}  // namespace lark::ws
