#ifndef LIB_BLYNK_H
#define LIB_BLYNK_H

#include <Arduino.h>

// Blynk template configuration
#define BLYNK_TEMPLATE_ID "TMPL000000000"
#define BLYNK_TEMPLATE_NAME "ESP32 Voice Assistant"
#define BLYNK_FIRMWARE_VERSION "1.0.0"
#define BLYNK_PRINT Serial

// Blynk Virtual Pins
#define VPIN_RECORDING_STATUS V0    // LED widget - shows recording status
#define VPIN_CONNECTED_STATUS V1    // LED widget - shows connection status
#define VPIN_LAST_RESPONSE V2       // Terminal/Label - shows last AI response
#define VPIN_AUDIO_LEVEL V3         // Gauge - shows audio level
#define VPIN_MANUAL_RECORD V4       // Button - manual record trigger
#define VPIN_RESPONSE_TIME V5       // Value display - processing time
#define VPIN_ERROR_LOG V6           // Terminal - error messages
#define VPIN_WIFI_SIGNAL V7         // Gauge - WiFi signal strength

// Function declarations
void setupBlynk();
void loopBlynk();
void updateBlynkRecordingStatus(bool recording);
void updateBlynkConnectionStatus(bool connected);
void updateBlynkAudioLevel(int level);
void updateBlynkResponse(const String& response);
void updateBlynkResponseTime(unsigned long timeMs);
void logBlynkError(const String& error);
void updateBlynkWiFiSignal();

#endif
