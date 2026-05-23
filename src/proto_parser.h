#pragma once
#include <Arduino.h>

struct MeshMessage {
  uint32_t from_node;    // Absender Node-ID
  uint32_t to_node;      // Empfänger (0xFFFFFFFF = Broadcast)
  char     text[201];    // Nachrichtentext
  bool     valid;        // true wenn Text-Nachricht
};

class ProtoParser {
public:
  // Parsed ein FromRadio Protobuf Byte-Array
  static MeshMessage parseFromRadio(const uint8_t* data, size_t len);

  // Baut ein ToRadio Protobuf Paket für eine Text-Nachricht
  // Gibt Länge zurück, buf muss mind. 256 Byte groß sein
  static size_t buildTextMessage(const char* text, uint8_t* buf, size_t bufLen);

  // Node-ID als String (z.B. "!a1b2c3d4")
  static void nodeIdToStr(uint32_t id, char* out);

private:
  static uint64_t readVarint(const uint8_t* d, size_t len, size_t& pos);
  static bool     readField(const uint8_t* d, size_t len, size_t& pos,
                            uint32_t& fnum, uint32_t& wtype,
                            uint64_t& val, const uint8_t*& blob, size_t& blen);
  static MeshMessage parseMeshPacket(const uint8_t* d, size_t len);
  static void        parseData(const uint8_t* d, size_t len, MeshMessage& msg);
  static size_t      writeVarint(uint64_t val, uint8_t* buf, size_t pos);
  static size_t      writeTag(uint32_t field, uint32_t wtype, uint8_t* buf, size_t pos);
};
