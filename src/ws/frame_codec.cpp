#include "lark/ws/frame_codec.h"

#include <cstddef>

namespace lark::ws {

namespace {

bool ReadVarint(const std::string& data, size_t& offset, uint64_t& value) {
  value = 0;
  int shift = 0;
  while (offset < data.size() && shift < 64) {
    uint8_t byte = static_cast<uint8_t>(data[offset++]);
    value |= static_cast<uint64_t>(byte & 0x7F) << shift;
    if ((byte & 0x80) == 0) {
      return true;
    }
    shift += 7;
  }
  return false;
}

void WriteVarint(uint64_t value, std::string& out) {
  while (value >= 0x80) {
    out.push_back(static_cast<char>((value & 0x7F) | 0x80));
    value >>= 7;
  }
  out.push_back(static_cast<char>(value));
}

void WriteKey(uint32_t field, uint32_t wire, std::string& out) {
  WriteVarint((field << 3) | wire, out);
}

bool ReadLengthDelimited(const std::string& data, size_t& offset, std::string& out) {
  uint64_t len = 0;
  if (!ReadVarint(data, offset, len)) {
    return false;
  }
  if (offset + len > data.size()) {
    return false;
  }
  out.assign(data.data() + offset, data.data() + offset + len);
  offset += len;
  return true;
}

bool DecodeHeader(const std::string& data, FrameHeader& header) {
  size_t offset = 0;
  while (offset < data.size()) {
    uint64_t key = 0;
    if (!ReadVarint(data, offset, key)) {
      return false;
    }
    uint32_t field = static_cast<uint32_t>(key >> 3);
    uint32_t wire = static_cast<uint32_t>(key & 0x07);
    if (wire == 2) {
      std::string value;
      if (!ReadLengthDelimited(data, offset, value)) {
        return false;
      }
      if (field == 1) {
        header.key = value;
      } else if (field == 2) {
        header.value = value;
      }
    } else if (wire == 0) {
      uint64_t skip = 0;
      if (!ReadVarint(data, offset, skip)) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

std::string EncodeHeader(const FrameHeader& header) {
  std::string out;
  WriteKey(1, 2, out);
  WriteVarint(header.key.size(), out);
  out.append(header.key);
  WriteKey(2, 2, out);
  WriteVarint(header.value.size(), out);
  out.append(header.value);
  return out;
}

}  // namespace

bool DecodeFrame(const std::string& data, Frame& frame) {
  size_t offset = 0;
  while (offset < data.size()) {
    uint64_t key = 0;
    if (!ReadVarint(data, offset, key)) {
      return false;
    }
    uint32_t field = static_cast<uint32_t>(key >> 3);
    uint32_t wire = static_cast<uint32_t>(key & 0x07);
    if (wire == 0) {
      uint64_t value = 0;
      if (!ReadVarint(data, offset, value)) {
        return false;
      }
      if (field == 1) {
        frame.seq_id = value;
      } else if (field == 2) {
        frame.log_id = value;
      } else if (field == 3) {
        frame.service = static_cast<int32_t>(value);
      } else if (field == 4) {
        frame.method = static_cast<int32_t>(value);
      }
    } else if (wire == 2) {
      std::string value;
      if (!ReadLengthDelimited(data, offset, value)) {
        return false;
      }
      if (field == 5) {
        FrameHeader header;
        if (!DecodeHeader(value, header)) {
          return false;
        }
        frame.headers.push_back(std::move(header));
      } else if (field == 8) {
        frame.payload = std::move(value);
      }
    } else {
      return false;
    }
  }
  return true;
}

std::string EncodeFrame(const Frame& frame) {
  std::string out;
  WriteKey(1, 0, out);
  WriteVarint(frame.seq_id, out);
  WriteKey(2, 0, out);
  WriteVarint(frame.log_id, out);
  WriteKey(3, 0, out);
  WriteVarint(static_cast<uint64_t>(frame.service), out);
  WriteKey(4, 0, out);
  WriteVarint(static_cast<uint64_t>(frame.method), out);
  for (const auto& header : frame.headers) {
    std::string encoded = EncodeHeader(header);
    WriteKey(5, 2, out);
    WriteVarint(encoded.size(), out);
    out.append(encoded);
  }
  if (!frame.payload.empty()) {
    WriteKey(8, 2, out);
    WriteVarint(frame.payload.size(), out);
    out.append(frame.payload);
  }
  return out;
}

}  // namespace lark::ws
