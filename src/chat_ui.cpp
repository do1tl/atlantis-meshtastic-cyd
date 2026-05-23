#include "chat_ui.h"
#include <string.h>
#include <stdio.h>

ChatUI chatUI;

// ── Keyboard rows ─────────────────────────────────────────────────────────────
// Normal mode (lower)
static const char* ROW0_LO[] = {"q","w","e","r","t","y","u","i","o","p", nullptr};
static const char* ROW1_LO[] = {"a","s","d","f","g","h","j","k","l",    nullptr};
static const char* ROW2_LO[] = {"SHF","z","x","c","v","b","n","m","<",  nullptr};
static const char* ROW3[]    = {"123"," ",".",",","SND",                 nullptr};

// Shifted (upper)
static const char* ROW0_UP[] = {"Q","W","E","R","T","Y","U","I","O","P", nullptr};
static const char* ROW1_UP[] = {"A","S","D","F","G","H","J","K","L",    nullptr};
static const char* ROW2_UP[] = {"SHF","Z","X","C","V","B","N","M","<",  nullptr};

// Number mode
static const char* ROW0_NM[] = {"1","2","3","4","5","6","7","8","9","0", nullptr};
static const char* ROW1_NM[] = {"-","/",":","(",")","@","\"","'","!",   nullptr};
static const char* ROW2_NM[] = {"ABC","#","%","^","*","+","=","?","<",  nullptr};

// ── Geometry constants ────────────────────────────────────────────────────────
#define KBD_Y   (STATUS_H + MSG_H + INPUT_H)
#define ROW_H   (KBD_H / 4)

// ── Helpers ───────────────────────────────────────────────────────────────────


bool ChatUI::_hitTest(int tx, int ty, int x, int y, int w, int h) {
  return tx >= x && tx < x + w && ty >= y && ty < y + h;
}

// ── Init ──────────────────────────────────────────────────────────────────────

Touch touch;

void ChatUI::begin(TFT_eSPI& tft) {
  _tft = &tft;
  touch.begin();
  _tft->fillScreen(C_BG);
  _dirtyStatus = _dirtyMsgs = _dirtyInput = _dirtyKbd = true;
}

// ── Status ────────────────────────────────────────────────────────────────────

void ChatUI::setStatus(const char* text, bool connected) {
  if (strcmp(_statusText, text) == 0 && _statusConn == connected) return;
  strncpy(_statusText, text, sizeof(_statusText) - 1);
  _statusText[sizeof(_statusText) - 1] = '\0';
  _statusConn   = connected;
  _dirtyStatus  = true;
}

// ── Message management ────────────────────────────────────────────────────────

void ChatUI::addMessage(const MeshMessage& msg, uint32_t myNodeId) {
  ChatMessage& m = _msgs[_msgHead % MAX_MESSAGES];
  strncpy(m.text, msg.text, MAX_MSG_LEN);
  m.text[MAX_MSG_LEN] = '\0';
  m.from_node = msg.from_node;
  m.mine      = (msg.from_node == myNodeId);
  _msgHead++;
  if (_msgCount < MAX_MESSAGES) _msgCount++;
  _dirtyMsgs = true;
}

void ChatUI::addSystemMsg(const char* text) {
  ChatMessage& m = _msgs[_msgHead % MAX_MESSAGES];
  strncpy(m.text, text, MAX_MSG_LEN);
  m.text[MAX_MSG_LEN] = '\0';
  m.from_node = 0;
  m.mine      = false;
  _msgHead++;
  if (_msgCount < MAX_MESSAGES) _msgCount++;
  _dirtyMsgs = true;
}

// ── Send ──────────────────────────────────────────────────────────────────────

bool ChatUI::getSendText(char* outBuf, size_t bufLen) {
  if (!_hasSend) return false;
  _hasSend = false;
  strncpy(outBuf, _sendBuf, bufLen - 1);
  outBuf[bufLen - 1] = '\0';
  return true;
}

// ── Drawing ───────────────────────────────────────────────────────────────────

void ChatUI::_drawStatus() {
  _tft->fillRect(0, 0, SCREEN_W, STATUS_H, C_STATUS);
  uint16_t dot = _statusConn ? C_CONN_OK : C_CONN_NO;
  _tft->fillCircle(6, STATUS_H / 2, 4, dot);
  _tft->setTextColor(C_KEY_TXT, C_STATUS);
  _tft->setTextSize(1);
  _tft->setCursor(16, (STATUS_H - 8) / 2);
  _tft->print(_statusText);
  _dirtyStatus = false;
}

void ChatUI::_drawMessages() {
  int y0 = STATUS_H;
  _tft->fillRect(0, y0, SCREEN_W, MSG_H, C_BG);

  if (_msgCount == 0) {
    _dirtyMsgs = false;
    return;
  }

  // Show last N messages that fit
  int lineH = 12;
  int maxLines = MSG_H / lineH;
  int start = (_msgHead - _msgCount);
  int total = _msgCount;
  int show  = total < maxLines ? total : maxLines;
  int from  = total - show;

  int y = y0 + MSG_H - show * lineH;

  for (int i = from; i < total; i++) {
    const ChatMessage& m = _msgs[(start + i) % MAX_MESSAGES];
    if (m.from_node == 0) {
      // System message
      _tft->setTextColor(C_DIVIDER, C_BG);
    } else if (m.mine) {
      _tft->setTextColor(C_MSG_ME, C_BG);
    } else {
      _tft->setTextColor(C_MSG_OTHER, C_BG);
    }
    _tft->setTextSize(1);
    _tft->setCursor(2, y);

    // Show node-id prefix for received messages
    if (m.from_node != 0 && !m.mine) {
      char nid[12];
      ProtoParser::nodeIdToStr(m.from_node, nid);
      char line[220];
      snprintf(line, sizeof(line), "%.9s: %.200s", nid, m.text);
      _tft->print(line);
    } else {
      _tft->print(m.text);
    }
    y += lineH;
  }

  _dirtyMsgs = false;
}

void ChatUI::_drawInput() {
  int y = STATUS_H + MSG_H;
  _tft->fillRect(0, y, SCREEN_W, INPUT_H, C_INPUT_BG);
  _tft->drawRect(0, y, SCREEN_W, INPUT_H, C_DIVIDER);
  _tft->setTextColor(C_KEY_TXT, C_INPUT_BG);
  _tft->setTextSize(1);
  _tft->setCursor(4, y + (INPUT_H - 8) / 2);

  // Show last ~38 chars if input is long
  const char* txt = _input;
  int len = _inputLen;
  int maxChars = 38;
  if (len > maxChars) txt += len - maxChars;

  _tft->print(txt);

  // Cursor blink
  int cx = 4 + strlen(txt) * 6;
  _tft->fillRect(cx, y + 3, 2, INPUT_H - 6,
                 (millis() / 500) & 1 ? C_KEY_TXT : C_INPUT_BG);

  _dirtyInput = false;
}

void ChatUI::_drawKey(int x, int y, int w, int h, const char* label, bool active) {
  uint16_t bg = active ? C_KEY_ACT : C_KEY_BG;
  _tft->fillRoundRect(x + 1, y + 1, w - 2, h - 2, 3, bg);
  _tft->setTextColor(C_KEY_TXT, bg);
  _tft->setTextSize(1);
  int tx = x + (w - strlen(label) * 6) / 2;
  int ty = y + (h - 8) / 2;
  _tft->setCursor(tx, ty);
  _tft->print(label);
}

void ChatUI::_drawKeyboard() {
  _tft->fillRect(0, KBD_Y, SCREEN_W, KBD_H, C_BG);

  // Row 0: 10 letters
  const char** r0 = _numMode ? ROW0_NM : (_shifted ? ROW0_UP : ROW0_LO);
  const char** r1 = _numMode ? ROW1_NM : (_shifted ? ROW1_UP : ROW1_LO);
  const char** r2 = _numMode ? ROW2_NM : (_shifted ? ROW2_UP : ROW2_LO);

  int kw = SCREEN_W / 10;
  for (int i = 0; r0[i]; i++)
    _drawKey(i * kw, KBD_Y,            kw, ROW_H, r0[i], false);
  for (int i = 0; r1[i]; i++)
    _drawKey(i * kw + kw/2, KBD_Y + ROW_H, kw, ROW_H, r1[i], false);
  for (int i = 0; r2[i]; i++) {
    int w = (strcmp(r2[i],"SHF")==0 || strcmp(r2[i],"ABC")==0) ? kw*2 : kw;
    int x = (i == 0) ? 0 : (kw*2 + (i-1)*kw);
    _drawKey(x, KBD_Y + ROW_H*2, w, ROW_H, r2[i],
             (strcmp(r2[i],"SHF")==0 && _shifted) || (strcmp(r2[i],"ABC")==0 && !_numMode));
  }

  // Row 3: 123/ABC  SPACE  .  ,  SEND
  int x = 0;
  _drawKey(x, KBD_Y + ROW_H*3, kw*2,  ROW_H, _numMode ? "ABC" : "123", false); x += kw*2;
  _drawKey(x, KBD_Y + ROW_H*3, kw*4,  ROW_H, "SPC", false);                    x += kw*4;
  _drawKey(x, KBD_Y + ROW_H*3, kw,    ROW_H, ".",   false);                    x += kw;
  _drawKey(x, KBD_Y + ROW_H*3, kw,    ROW_H, ",",   false);                    x += kw;
  _drawKey(x, KBD_Y + ROW_H*3, kw*2,  ROW_H, "SND", _inputLen > 0);

  _dirtyKbd = false;
}

// ── Touch handling ────────────────────────────────────────────────────────────

void ChatUI::_handleTouch(int tx, int ty) {
  const char** r0 = _numMode ? ROW0_NM : (_shifted ? ROW0_UP : ROW0_LO);
  const char** r1 = _numMode ? ROW1_NM : (_shifted ? ROW1_UP : ROW1_LO);
  const char** r2 = _numMode ? ROW2_NM : (_shifted ? ROW2_UP : ROW2_LO);
  int kw = SCREEN_W / 10;
  bool hit = false;

  // Row 0
  for (int i = 0; r0[i] && !hit; i++) {
    if (_hitTest(tx, ty, i*kw, KBD_Y, kw, ROW_H)) {
      if (_inputLen < MAX_INPUT_LEN) { _input[_inputLen++] = r0[i][0]; _input[_inputLen] = '\0'; }
      _shifted = false; hit = true; _dirtyInput = true; _dirtyKbd = true;
    }
  }
  // Row 1
  for (int i = 0; r1[i] && !hit; i++) {
    if (_hitTest(tx, ty, i*kw + kw/2, KBD_Y + ROW_H, kw, ROW_H)) {
      if (_inputLen < MAX_INPUT_LEN) { _input[_inputLen++] = r1[i][0]; _input[_inputLen] = '\0'; }
      _shifted = false; hit = true; _dirtyInput = true; _dirtyKbd = true;
    }
  }
  // Row 2
  for (int i = 0; r2[i] && !hit; i++) {
    bool isSpecial = (strcmp(r2[i],"SHF")==0 || strcmp(r2[i],"ABC")==0);
    int w = isSpecial ? kw*2 : kw;
    int x = (i == 0) ? 0 : (kw*2 + (i-1)*kw);
    if (_hitTest(tx, ty, x, KBD_Y + ROW_H*2, w, ROW_H)) {
      if (strcmp(r2[i],"SHF")==0) { _shifted = !_shifted; }
      else if (strcmp(r2[i],"ABC")==0) { _numMode = false; }
      else if (strcmp(r2[i],"<")==0) {
        if (_inputLen > 0) { _inputLen--; _input[_inputLen] = '\0'; }
      } else {
        if (_inputLen < MAX_INPUT_LEN) { _input[_inputLen++] = r2[i][0]; _input[_inputLen] = '\0'; }
        _shifted = false;
      }
      hit = true; _dirtyInput = true; _dirtyKbd = true;
    }
  }

  // Row 3
  if (!hit) {
    int ry = KBD_Y + ROW_H*3;
    if      (_hitTest(tx, ty, 0,      ry, kw*2, ROW_H)) { _numMode = !_numMode; hit = true; _dirtyKbd = true; }
    else if (_hitTest(tx, ty, kw*2,   ry, kw*4, ROW_H)) {
      // Space
      if (_inputLen < MAX_INPUT_LEN) { _input[_inputLen++] = ' '; _input[_inputLen] = '\0'; }
      hit = true; _dirtyInput = true;
    }
    else if (_hitTest(tx, ty, kw*6,   ry, kw,   ROW_H)) {
      if (_inputLen < MAX_INPUT_LEN) { _input[_inputLen++] = '.'; _input[_inputLen] = '\0'; }
      hit = true; _dirtyInput = true;
    }
    else if (_hitTest(tx, ty, kw*7,   ry, kw,   ROW_H)) {
      if (_inputLen < MAX_INPUT_LEN) { _input[_inputLen++] = ','; _input[_inputLen] = '\0'; }
      hit = true; _dirtyInput = true;
    }
    else if (_hitTest(tx, ty, kw*8,   ry, kw*2, ROW_H)) {
      // SEND
      if (_inputLen > 0) {
        strncpy(_sendBuf, _input, MAX_INPUT_LEN);
        _sendBuf[MAX_INPUT_LEN] = '\0';
        _inputLen = 0; _input[0] = '\0';
        _hasSend = true;
        hit = true; _dirtyInput = true; _dirtyKbd = true;
      }
    }
  }
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void ChatUI::loop() {
  // Redraw dirty regions
  if (_dirtyStatus)  _drawStatus();
  if (_dirtyMsgs)    _drawMessages();
  if (_dirtyInput || (millis() / 500) & 1) { _drawInput(); } // cursor blink
  if (_dirtyKbd)     _drawKeyboard();

  // Touch via bit-bang
  uint32_t now = millis();
  if (now - _lastTouch > 150) {
    int16_t tx, ty;
    if (touch.read(tx, ty)) {
      _lastTouch = now;
      if (ty >= KBD_Y) _handleTouch(tx, ty);
    }
  }
}
