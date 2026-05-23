#include "proto_parser.h"
#include <string.h>

// ── Protobuf wire types ───────────────────────────────────────────────────────
#define WT_VARINT  0
#define WT_64BIT   1
#define WT_LEN     2
#define WT_32BIT   5

// ── Low-level helpers ─────────────────────────────────────────────────────────

uint64_t ProtoParser::readVarint(const uint8_t* d, size_t len, size_t& pos) {
  uint64_t val = 0;
  int shift = 0;
  while (pos < len && shift < 64) {
    uint8_t b = d[pos++];
    val |= (uint64_t)(b & 0x7F) << shift;
    if (!(b & 0x80)) break;
    shift += 7;
  }
  return val;
}

bool ProtoParser::readField(const uint8_t* d, size_t len, size_t& pos,
                            uint32_t& fnum, uint32_t& wtype,
                            uint64_t& val, const uint8_t*& blob, size_t& blen) {
  if (pos >= len) return false;
  uint64_t tag = readVarint(d, len, pos);
  fnum  = (uint32_t)(tag >> 3);
  wtype = (uint32_t)(tag & 0x07);
  val   = 0; blob = nullptr; blen = 0;

  switch (wtype) {
    case WT_VARINT: val = readVarint(d, len, pos); break;
    case WT_64BIT:
      if (pos + 8 > len) return false;
      memcpy(&val, d + pos, 8); pos += 8; break;
    case WT_LEN: {
      blen = (size_t)readVarint(d, len, pos);
      if (pos + blen > len) return false;
      blob = d + pos; pos += blen; break;
    }
    case WT_32BIT:
      if (pos + 4 > len) return false;
      memcpy(&val, d + pos, 4); pos += 4; break;
    default: return false;
  }
  return true;
}

size_t ProtoParser::writeVarint(uint64_t val, uint8_t* buf, size_t pos) {
  while (val > 0x7F) {
    buf[pos++] = (uint8_t)((val & 0x7F) | 0x80);
    val >>= 7;
  }
  buf[pos++] = (uint8_t)(val & 0x7F);
  return pos;
}

size_t ProtoParser::writeTag(uint32_t field, uint32_t wtype, uint8_t* buf, size_t pos) {
  return writeVarint(((uint64_t)field << 3) | wtype, buf, pos);
}

// ── Parsing ───────────────────────────────────────────────────────────────────

// Data payload (field 1 = portnum varint, field 2 = payload bytes)
void ProtoParser::parseData(const uint8_t* d, size_t len, MeshMessage& msg) {
  size_t pos = 0;
  uint32_t fnum, wtype; uint64_t val; const uint8_t* blob; size_t blen;
  bool isText = false;

  while (readField(d, len, pos, fnum, wtype, val, blob, blen)) {
    if (fnum == 1 && wtype == WT_VARINT) {
      isText = (val == 1); // portnum TEXT_MESSAGE_APP = 1
    } else if (fnum == 2 && wtype == WT_LEN && isText) {
      size_t copy = blen < 200 ? blen : 200;
      memcpy(msg.text, blob, copy);
      msg.text[copy] = '\0';
      msg.valid = true;
    }
  }
}

// MeshPacket: field 1 = to (uint32), field 2 = from (uint32),
//             field 6 = decoded Data (embedded), field 8 = decoded Data (alt)
MeshMessage ProtoParser::parseMeshPacket(const uint8_t* d, size_t len) {
  MeshMessage msg = {};
  size_t pos = 0;
  uint32_t fnum, wtype; uint64_t val; const uint8_t* blob; size_t blen;

  while (readField(d, len, pos, fnum, wtype, val, blob, blen)) {
    switch (fnum) {
      case 1: if (wtype == WT_VARINT) msg.to_node   = (uint32_t)val; break;
      case 2: if (wtype == WT_VARINT) msg.from_node = (uint32_t)val; break;
      case 6: if (wtype == WT_LEN)    parseData(blob, blen, msg); break;
      case 8: if (wtype == WT_LEN)    parseData(blob, blen, msg); break;
      default: break;
    }
  }
  return msg;
}

// FromRadio wrapper: field 2 = MeshPacket (embedded message)
MeshMessage ProtoParser::parseFromRadio(const uint8_t* data, size_t len) {
  MeshMessage msg = {};
  size_t pos = 0;
  uint32_t fnum, wtype; uint64_t val; const uint8_t* blob; size_t blen;

  while (readField(data, len, pos, fnum, wtype, val, blob, blen)) {
    if (fnum == 2 && wtype == WT_LEN) {
      msg = parseMeshPacket(blob, blen);
    }
  }
  return msg;
}

// ── Building ToRadio ──────────────────────────────────────────────────────────

// ToRadio { field 1 = MeshPacket { field 1=to, field 3=channel, field 6=Data { field 1=portnum, field 2=text } } }
size_t ProtoParser::buildTextMessage(const char* text, uint8_t* buf, size_t bufLen) {
  if (!text || bufLen < 16) return 0;

  uint8_t tmp[256];
  size_t p = 0;

  // ── Data payload ──────────────────────────────────────────────────────────
  uint8_t data[220]; size_t dp = 0;
  // field 1: portnum = 1 (TEXT_MESSAGE_APP)
  dp = writeTag(1, WT_VARINT, data, dp);
  dp = writeVarint(1, data, dp);
  // field 2: payload = text bytes
  size_t tlen = strlen(text);
  if (tlen > 200) tlen = 200;
  dp = writeTag(2, WT_LEN, data, dp);
  dp = writeVarint(tlen, data, dp);
  memcpy(data + dp, text, tlen); dp += tlen;

  // ── MeshPacket ────────────────────────────────────────────────────────────
  // field 1: to = 0xFFFFFFFF (broadcast)
  p = writeTag(1, WT_VARINT, tmp, p);
  p = writeVarint(0xFFFFFFFFULL, tmp, p);
  // field 3: channel = 0
  p = writeTag(3, WT_VARINT, tmp, p);
  p = writeVarint(0, tmp, p);
  // field 6: decoded Data (embedded)
  p = writeTag(6, WT_LEN, tmp, p);
  p = writeVarint(dp, tmp, p);
  memcpy(tmp + p, data, dp); p += dp;
  // field 10: want_ack = false (0) – omit (default)

  // ── ToRadio wrapper ───────────────────────────────────────────────────────
  size_t out = 0;
  out = writeTag(1, WT_LEN, buf, out);
  out = writeVarint(p, buf, out);
  if (out + p > bufLen) return 0;
  memcpy(buf + out, tmp, p); out += p;
  return out;
}

// ── Utility ───────────────────────────────────────────────────────────────────

void ProtoParser::nodeIdToStr(uint32_t id, char* out) {
  snprintf(out, 12, "!%08x", (unsigned int)id);
}
