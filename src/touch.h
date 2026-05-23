#pragma once
#include <Arduino.h>
#include "config.h"

// Bit-bang XPT2046 touch controller
// Uses dedicated pins independent of display SPI bus

class Touch {
public:
  void begin() {
    pinMode(TOUCH_CS,   OUTPUT); digitalWrite(TOUCH_CS,   HIGH);
    pinMode(TOUCH_CLK,  OUTPUT); digitalWrite(TOUCH_CLK,  LOW);
    pinMode(TOUCH_MOSI, OUTPUT); digitalWrite(TOUCH_MOSI, LOW);
    pinMode(TOUCH_MISO, INPUT);
  }

  // Returns true when touched, fills screen-space x/y
  bool read(int16_t &sx, int16_t &sy) {
    uint16_t z1 = _cmd(0xB0); // Z1 pressure
    if (z1 < 60) return false; // not touched

    // Average 4 readings for stability
    uint32_t rx = 0, ry = 0;
    for (int i = 0; i < 4; i++) {
      rx += _cmd(0xD0); // X
      ry += _cmd(0x90); // Y
    }
    rx >>= 2; ry >>= 2;

    // Filter junk readings (Y=2047 = invalid)
    if (ry > 4000) return false;

    sx = (int16_t)map((int32_t)rx, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W - 1);
    sy = (int16_t)map((int32_t)ry, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H - 1);
    sx = constrain(sx, 0, SCREEN_W - 1);
    sy = constrain(sy, 0, SCREEN_H - 1);
    return true;
  }

private:
  uint16_t _cmd(uint8_t cmd) {
    uint16_t result = 0;
    digitalWrite(TOUCH_CS, LOW);
    delayMicroseconds(1);
    // 8 command bits
    for (int i = 7; i >= 0; i--) {
      digitalWrite(TOUCH_CLK, LOW);
      digitalWrite(TOUCH_MOSI, (cmd >> i) & 1);
      delayMicroseconds(1);
      digitalWrite(TOUCH_CLK, HIGH);
      delayMicroseconds(1);
    }
    // Skip busy bit
    digitalWrite(TOUCH_CLK, LOW); delayMicroseconds(2);
    digitalWrite(TOUCH_CLK, HIGH); delayMicroseconds(1);
    // 12 data bits
    for (int i = 11; i >= 0; i--) {
      digitalWrite(TOUCH_CLK, LOW); delayMicroseconds(1);
      result |= ((uint16_t)digitalRead(TOUCH_MISO) << i);
      digitalWrite(TOUCH_CLK, HIGH); delayMicroseconds(1);
    }
    digitalWrite(TOUCH_CLK, LOW);
    digitalWrite(TOUCH_CS, HIGH);
    return result;
  }
};

extern Touch touch;
