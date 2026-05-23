#include "ble_mesh.h"
#include "config.h"
#include <Arduino.h>

BleMesh* BleMesh::_instance = nullptr;
BleMesh  bleMesh;

// ── Scan callback ─────────────────────────────────────────────────────────────

void BleMesh::onResult(NimBLEAdvertisedDevice* dev) {
  const char* devName = dev->getName().c_str();
  bool match = false;

  if (strlen(MESH_DEVICE_NAME) > 0) {
    match = (dev->getName() == MESH_DEVICE_NAME);
  } else {
    // Match any device advertising Meshtastic service UUID
    if (dev->isAdvertisingService(NimBLEUUID(MESHTASTIC_SERVICE_UUID))) {
      match = true;
    }
    // Or device name starts with "Meshtastic_"
    if (!match && strncmp(devName, "Meshtastic_", 11) == 0) {
      match = true;
    }
  }

  if (match) {
    Serial.printf("[BLE] Found: %s\n", devName);
    NimBLEDevice::getScan()->stop();
    _device   = dev;
    _doConnect = true;
    _scanning  = false;
  }
}

// ── Connect / disconnect callbacks ───────────────────────────────────────────

void BleMesh::onConnect(NimBLEClient* client) {
  Serial.println("[BLE] Connected");
  _connected = true;
}

void BleMesh::onDisconnect(NimBLEClient* client) {
  Serial.println("[BLE] Disconnected");
  _connected  = false;
  _toRadio    = nullptr;
  _fromRadio  = nullptr;
  _fromNum    = nullptr;
  _doConnect  = false;
  _device     = nullptr;
  // Restart scan after short delay handled in loop()
}

// ── Notify callback (static → instance) ─────────────────────────────────────

void BleMesh::_notifyCB(NimBLERemoteCharacteristic* c,
                        uint8_t* data, size_t len, bool /*isNotify*/) {
  if (!_instance) return;
  // FromNum changed → queue a FromRadio read
  _instance->_readPend = true;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

void BleMesh::_startScan() {
  Serial.println("[BLE] Starting scan...");
  _scanning = true;
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(this, false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(90);
  scan->start(0, false); // continuous scan
}

void BleMesh::_connect() {
  if (!_device) return;
  Serial.printf("[BLE] Connecting to %s...\n", _device->getAddress().toString().c_str());

  _client = NimBLEDevice::createClient();
  _client->setClientCallbacks(this, false);
  _client->setConnectionParams(12, 12, 0, 51);
  _client->setConnectTimeout(10);

  if (!_client->connect(_device)) {
    Serial.println("[BLE] Connection failed");
    NimBLEDevice::deleteClient(_client);
    _client   = nullptr;
    _device   = nullptr;
    _doConnect = false;
    _startScan();
    return;
  }

#if MESH_BLE_PIN != 0
  _client->secureConnection();
#endif

  NimBLERemoteService* svc = _client->getService(MESHTASTIC_SERVICE_UUID);
  if (!svc) {
    Serial.println("[BLE] Meshtastic service not found");
    _client->disconnect();
    return;
  }

  _toRadio   = svc->getCharacteristic(TORADIO_UUID);
  _fromRadio = svc->getCharacteristic(FROMRADIO_UUID);
  _fromNum   = svc->getCharacteristic(FROMNUM_UUID);

  if (!_toRadio || !_fromRadio || !_fromNum) {
    Serial.println("[BLE] Required characteristics not found");
    _client->disconnect();
    return;
  }

  _subscribeFromRadio();
  Serial.println("[BLE] Ready");
}

void BleMesh::_subscribeFromRadio() {
  if (_fromNum && _fromNum->canNotify()) {
    _fromNum->subscribe(true, BleMesh::_notifyCB);
    Serial.println("[BLE] Subscribed to FromNum");
  }
  // Read all pending FromRadio packets on connect
  _readPend = true;
}

void BleMesh::_readPending() {
  if (!_fromRadio || !_connected) return;
  _readPend = false;

  // Drain all packets (Meshtastic queues multiple packets; read until empty)
  for (int i = 0; i < 64; i++) {
    std::string val = _fromRadio->readValue();
    if (val.empty()) break;

    const uint8_t* data = (const uint8_t*)val.data();
    size_t len = val.size();

    MeshMessage msg = ProtoParser::parseFromRadio(data, len);
    if (msg.valid && _msgCb) {
      _msgCb(msg);
    }
  }
}

// ── Public API ────────────────────────────────────────────────────────────────

void BleMesh::begin(MsgCallback cb) {
  _instance = this;
  _msgCb    = cb;

  NimBLEDevice::init("AtlantisOS-CYD");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  _startScan();
}

void BleMesh::loop() {
  if (_doConnect && !_connected) {
    _doConnect = false;
    _connect();
  }

  if (_connected && _readPend) {
    _readPending();
  }

  // Auto-reconnect: if not connected and not scanning
  if (!_connected && !_scanning && !_doConnect) {
    _startScan();
  }
}

bool BleMesh::sendText(const char* text) {
  if (!_connected || !_toRadio) return false;

  uint8_t buf[256];
  size_t len = ProtoParser::buildTextMessage(text, buf, sizeof(buf));
  if (len == 0) return false;

  bool ok = _toRadio->writeValue(buf, len, false);
  Serial.printf("[BLE] Send '%s' → %s\n", text, ok ? "ok" : "fail");
  return ok;
}
