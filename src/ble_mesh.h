#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "proto_parser.h"
#include "config.h"

// Callback: called from BLE task when a text message arrives
typedef void (*MsgCallback)(const MeshMessage& msg);

class BleMesh : public NimBLEClientCallbacks,
                public NimBLEAdvertisedDeviceCallbacks {
public:
  void begin(MsgCallback cb);
  void loop();

  bool sendText(const char* text);
  bool isConnected() const { return _connected; }
  bool isScanning()  const { return _scanning; }
  uint32_t myNodeId() const { return _myNodeId; }

  // NimBLEAdvertisedDeviceCallbacks
  void onResult(NimBLEAdvertisedDevice* dev) override;

  // NimBLEClientCallbacks
  void onConnect(NimBLEClient* client) override;
  void onDisconnect(NimBLEClient* client) override;

private:
  void _startScan();
  void _connect();
  void _subscribeFromRadio();
  void _readPending();
  static void _notifyCB(NimBLERemoteCharacteristic* c,
                        uint8_t* data, size_t len, bool isNotify);

  MsgCallback    _msgCb      = nullptr;
  NimBLEClient*  _client     = nullptr;
  NimBLERemoteCharacteristic* _toRadio   = nullptr;
  NimBLERemoteCharacteristic* _fromRadio = nullptr;
  NimBLERemoteCharacteristic* _fromNum   = nullptr;
  NimBLEAdvertisedDevice*     _device    = nullptr;

  bool     _connected   = false;
  bool     _scanning    = false;
  bool     _doConnect   = false;
  bool     _readPend    = false;
  uint32_t _myNodeId    = 0;
  uint32_t _lastFromNum = 0;

  static BleMesh* _instance;
};

extern BleMesh bleMesh;
