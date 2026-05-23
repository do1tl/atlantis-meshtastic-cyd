#include <Arduino.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"
#include "ble_mesh.h"
#include "chat_ui.h"

TFT_eSPI          tft;
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ── BLE message callback (called from BLE task) ───────────────────────────────
static void onMeshMessage(const MeshMessage& msg) {
  chatUI.addMessage(msg, bleMesh.myNodeId());
}

// ── Arduino setup / loop ──────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  Serial.println("\n[AtlantisOS-Meshtastic-CYD] Boot");

  // Backlight
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);

  // Display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Touch (uses dedicated SPI bus via XPT2046_Touchscreen)
  SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin();
  ts.setRotation(1);

  // UI
  chatUI.begin(tft, ts);
  chatUI.setStatus("Searching for Meshtastic...", false);

  // BLE
  bleMesh.begin(onMeshMessage);

  Serial.println("[main] Setup complete");
}

void loop() {
  bleMesh.loop();

  // Update status bar
  if (bleMesh.isConnected()) {
    char buf[40];
    snprintf(buf, sizeof(buf), "Connected  !%08x", (unsigned)bleMesh.myNodeId());
    chatUI.setStatus(buf, true);
  } else if (bleMesh.isScanning()) {
    chatUI.setStatus("Scanning...", false);
  } else {
    chatUI.setStatus("Connecting...", false);
  }

  // Check for outgoing message
  char sendBuf[MAX_INPUT_LEN + 1];
  if (chatUI.getSendText(sendBuf, sizeof(sendBuf))) {
    bool ok = bleMesh.sendText(sendBuf);
    if (ok) {
      // Add our own message to chat
      MeshMessage m = {};
      m.from_node = bleMesh.myNodeId();
      m.to_node   = 0xFFFFFFFF;
      strncpy(m.text, sendBuf, 200);
      m.text[200] = '\0';
      m.valid = true;
      chatUI.addMessage(m, bleMesh.myNodeId());
    } else {
      chatUI.addSystemMsg("Send failed - not connected");
    }
  }

  chatUI.loop();

  delay(10);
}
