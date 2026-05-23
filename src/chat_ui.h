#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"
#include "proto_parser.h"

struct ChatMessage {
  char    text[MAX_MSG_LEN + 1];
  uint32_t from_node;
  bool    mine;
};

class ChatUI {
public:
  void begin(TFT_eSPI& tft, XPT2046_Touchscreen& ts);
  void loop();

  void addMessage(const MeshMessage& msg, uint32_t myNodeId);
  void addSystemMsg(const char* text);

  // Returns true + fills outBuf when user presses Send
  bool getSendText(char* outBuf, size_t bufLen);

  void setStatus(const char* text, bool connected);

private:
  TFT_eSPI*            _tft = nullptr;
  XPT2046_Touchscreen* _ts  = nullptr;

  // Chat messages
  ChatMessage _msgs[MAX_MESSAGES];
  uint8_t     _msgCount   = 0;
  uint8_t     _msgHead    = 0; // ring buffer head

  // Input buffer
  char    _input[MAX_INPUT_LEN + 1] = {};
  uint8_t _inputLen = 0;

  // Status bar
  char _statusText[40] = "Searching...";
  bool _statusConn     = false;

  // Pending send
  bool _hasSend   = false;
  char _sendBuf[MAX_INPUT_LEN + 1] = {};

  // Touch debounce
  uint32_t _lastTouch = 0;

  // Dirty flags
  bool _dirtyStatus = true;
  bool _dirtyMsgs   = true;
  bool _dirtyInput  = true;
  bool _dirtyKbd    = true;

  // ── Keyboard layout ────────────────────────────────────────────────────────
  // 3 rows, each key: label (max 3 chars), width multiplier (1=normal, 2=wide)
  struct Key { const char* label; uint8_t w; }; // w = relative width units

  bool _shifted   = false;
  bool _numMode   = false;

  // ── Drawing ────────────────────────────────────────────────────────────────
  void _drawStatus();
  void _drawMessages();
  void _drawInput();
  void _drawKeyboard();
  void _drawKey(int x, int y, int w, int h, const char* label, bool active);

  // ── Touch ──────────────────────────────────────────────────────────────────
  void _handleTouch(int tx, int ty);
  bool _hitTest(int tx, int ty, int x, int y, int w, int h);

  int16_t _mapTouchX(uint16_t raw);
  int16_t _mapTouchY(uint16_t raw);
};

extern ChatUI chatUI;
