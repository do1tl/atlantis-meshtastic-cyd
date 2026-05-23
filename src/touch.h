#pragma once
#include <Arduino.h>
#include "config.h"

class Touch {
public:
  void begin() {
    pinMode(TOUCH_CS,   OUTPUT); digitalWrite(TOUCH_CS,   HIGH);
    pinMode(TOUCH_CLK,  OUTPUT); digitalWrite(TOUCH_CLK,  LOW);
    pinMode(TOUCH_MOSI, OUTPUT); digitalWrite(TOUCH_MOSI, LOW);
    pinMode(TOUCH_MISO, INPUT);
  }

  bool read(int16_t &sx, int16_t &sy) {
    uint16_t z1 = _cmd(0xB0);
    if (z1 < 60) return false;

    uint32_t rx = 0, ry = 0;
    for (int i = 0; i < 4; i++) {
      rx += _cmd(0xD0);
      ry += _cmd(0x90);
    }
    rx >>= 2; ry >>= 2;
    if (ry < 100) return false; // junk

    sx = (int16_t)map((int32_t)rx, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W - 1);
    sy = (int16_t)map((int32_t)ry, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H - 1);
    sx = constrain(sx, 0, SCREEN_W - 1);
    sy = constrain(sy, 0, SCREEN_H - 1);
    return true;
  }

private:
  // XPT2046: 8 cmd bits, 1 null/busy bit, 12 data bits
  // Reading 13 bits total then >> 1 discards the null bit correctly
  uint16_t _cmd(uint8_t cmd) {
    uint16_t result = 0;
    digitalWrite(TOUCH_CS, LOW);
    delayMicroseconds(1);
    for (int i = 7; i >= 0; i--) {
      digitalWrite(TOUCH_CLK, LOW);
      digitalWrite(TOUCH_MOSI, (cmd >> i) & 1);
      delayMicroseconds(1);
      digitalWrite(TOUCH_CLK, HIGH);
      delayMicroseconds(1);
    }
    // Read 13 bits (null + 12 data), then shift out the null bit
    for (int i = 12; i >= 0; i--) {
      digitalWrite(TOUCH_CLK, LOW); delayMicroseconds(1);
      result |= ((uint16_t)digitalRead(TOUCH_MISO) << i);
      digitalWrite(TOUCH_CLK, HIGH); delayMicroseconds(1);
    }
    digitalWrite(TOUCH_CLK, LOW);
    digitalWrite(TOUCH_CS, HIGH);
    return result >> 1; // discard null bit, gives proper 12-bit (0-4095)
  }
};

extern Touch touch;
