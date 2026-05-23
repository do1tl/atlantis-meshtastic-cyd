#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "ble_mesh.h"
#include "chat_ui.h"

TFT_eSPI tft;

static void onMeshMessage(const MeshMessage& msg) {
  chatUI.addMessage(msg, bleMesh.myNodeId());
}

void setup() {
  Serial.begin(115200);
  Serial.println("[AtlantisOS-Meshtastic-CYD] Boot");

  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  delay(50);

  chatUI.begin(tft);
  chatUI.setStatus("Searching for Meshtastic...", false);

  bleMesh.begin(onMeshMessage);
  Serial.println("[main] Setup complete");
}

void loop() {
  bleMesh.loop();

  if (bleMesh.isConnected()) {
    char buf[40];
    snprintf(buf, sizeof(buf), "Connected  !%08x", (unsigned)bleMesh.myNodeId());
    chatUI.setStatus(buf, true);
  } else if (bleMesh.isScanning()) {
    chatUI.setStatus("Scanning...", false);
  } else {
    chatUI.setStatus("Connecting...", false);
  }

  char sendBuf[MAX_INPUT_LEN + 1];
  if (chatUI.getSendText(sendBuf, sizeof(sendBuf))) {
    bool ok = bleMesh.sendText(sendBuf);
    if (ok) {
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
