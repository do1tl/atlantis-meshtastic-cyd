#pragma once

// ── Meshtastic Gerät ──────────────────────────────────────────────────────────
// Name des Heltec V3 wie er im BLE Scan erscheint (Meshtastic_XXXX)
// Leer lassen = verbindet mit erstem gefundenem Meshtastic Gerät
#define MESH_DEVICE_NAME    ""

// BLE PIN falls gesetzt (6 Stellen, 0 = kein PIN)
#define MESH_BLE_PIN        0

// ── Meshtastic BLE UUIDs ──────────────────────────────────────────────────────
// Mit nRF Connect App verifizieren falls Verbindung fehlschlägt
#define MESHTASTIC_SERVICE_UUID  "6ba4xxxx-1c70-4a47-9b3c-9f8e3b0a6987"
#define TORADIO_UUID             "f75c76d2-129e-4dad-a1dd-7866124401e7"
#define FROMRADIO_UUID           "2c55e69e-4993-11ed-b878-0242ac120002"
#define FROMNUM_UUID             "ed9da18c-a800-4f66-a670-aa7547e34453"

// ── CYD Touch Pins ────────────────────────────────────────────────────────────
#define TOUCH_CS    33
#define TOUCH_IRQ   36
#define TOUCH_MOSI  32
#define TOUCH_MISO  39
#define TOUCH_CLK   25

// Touch Kalibrierung (ggf. anpassen)
#define TOUCH_X_MIN  200
#define TOUCH_X_MAX  3700
#define TOUCH_Y_MIN  300
#define TOUCH_Y_MAX  3800

// ── Display ───────────────────────────────────────────────────────────────────
#define SCREEN_W    320
#define SCREEN_H    240
#define TFT_BL_PIN  21

// ── UI Layout ─────────────────────────────────────────────────────────────────
#define STATUS_H    18
#define INPUT_H     22
#define KBD_H       96
#define MSG_H       (SCREEN_H - STATUS_H - INPUT_H - KBD_H)   // 104px

// ── Farben ────────────────────────────────────────────────────────────────────
#define C_BG        0x0841   // Dunkelgrau
#define C_STATUS    0x1082   // Etwas heller
#define C_INPUT_BG  0x18C3
#define C_KEY_BG    0x2945
#define C_KEY_TXT   0xFFFF
#define C_KEY_ACT   0x07FF   // Cyan aktiv
#define C_MSG_ME    0x07FF   // Cyan eigene Nachrichten
#define C_MSG_OTHER 0xFFFF   // Weiß fremde Nachrichten
#define C_NODE_ID   0xFD20   // Orange Node-ID
#define C_CONN_OK   0x07E0   // Grün
#define C_CONN_NO   0xF800   // Rot
#define C_DIVIDER   0x39C7

// ── Chat ──────────────────────────────────────────────────────────────────────
#define MAX_MESSAGES    20
#define MAX_MSG_LEN     200
#define MAX_INPUT_LEN   200
#define MY_NODE_NAME    "CYD"
