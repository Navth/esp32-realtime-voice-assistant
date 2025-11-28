#include "lib_blynk.h"
#include "config.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Blynk credentials - ADD YOUR BLYNK AUTH TOKEN HERE
// Get it from Blynk.Console: https://blynk.cloud/
extern const char* BLYNK_AUTH_TOKEN;

static BlynkTimer blynkTimer;
static bool blynkConnected = false;
static unsigned long lastResponseTime = 0;

// Blynk event handlers
BLYNK_CONNECTED() {
    Serial.println("Connected to Blynk server");
    blynkConnected = true;
    Blynk.syncAll(); // Sync all virtual pins
}

BLYNK_DISCONNECTED() {
    Serial.println("Disconnected from Blynk server");
    blynkConnected = false;
}

// Manual record button handler
BLYNK_WRITE(VPIN_MANUAL_RECORD) {
    int buttonState = param.asInt();
    Serial.printf("Manual record button: %d\n", buttonState);
    
    if (buttonState == 1) {
        Serial.println("Starting manual recording via Blynk...");
        // Trigger recording - this will be handled in main loop
        extern void triggerManualRecord(bool state);
        triggerManualRecord(true);
    } else {
        Serial.println("Stopping manual recording via Blynk...");
        extern void triggerManualRecord(bool state);
        triggerManualRecord(false);
    }
}

void setupBlynk() {
    Serial.println("Initializing Blynk...");
    
    // Initialize Blynk
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
    
    // Set up periodic WiFi signal update (every 5 seconds)
    blynkTimer.setInterval(5000L, updateBlynkWiFiSignal);
    
    Serial.println("Blynk initialized");
}

void loopBlynk() {
    if (blynkConnected || Blynk.connected()) {
        Blynk.run();
        blynkTimer.run();
    }
}

void updateBlynkRecordingStatus(bool recording) {
    if (!blynkConnected) return;
    
    if (recording) {
        Blynk.virtualWrite(VPIN_RECORDING_STATUS, 255); // LED ON (red)
    } else {
        Blynk.virtualWrite(VPIN_RECORDING_STATUS, 0);   // LED OFF
    }
}

void updateBlynkConnectionStatus(bool connected) {
    if (!blynkConnected) return;
    
    if (connected) {
        Blynk.virtualWrite(VPIN_CONNECTED_STATUS, 255); // LED ON (green)
    } else {
        Blynk.virtualWrite(VPIN_CONNECTED_STATUS, 0);   // LED OFF
    }
}

void updateBlynkAudioLevel(int level) {
    if (!blynkConnected) return;
    
    // Map audio level (0-32767) to 0-100 for gauge
    int percentage = map(level, 0, 32767, 0, 100);
    Blynk.virtualWrite(VPIN_AUDIO_LEVEL, percentage);
}

void updateBlynkResponse(const String& response) {
    if (!blynkConnected) return;
    
    // Update terminal with timestamp
    String timestamp = String(millis() / 1000) + "s: ";
    Blynk.virtualWrite(VPIN_LAST_RESPONSE, timestamp + response);
    
    // Also update label widget
    Blynk.virtualWrite(VPIN_LAST_RESPONSE, response);
}

void updateBlynkResponseTime(unsigned long timeMs) {
    if (!blynkConnected) return;
    
    lastResponseTime = timeMs;
    Blynk.virtualWrite(VPIN_RESPONSE_TIME, timeMs);
}

void logBlynkError(const String& error) {
    if (!blynkConnected) return;
    
    String timestamp = String(millis() / 1000) + "s: ";
    Blynk.virtualWrite(VPIN_ERROR_LOG, timestamp + error + "\n");
}

void updateBlynkWiFiSignal() {
    if (!blynkConnected) return;
    
    int rssi = WiFi.RSSI();
    // Convert RSSI to percentage (better signal = higher percentage)
    // -30 dBm = excellent, -90 dBm = poor
    int signalPercentage = map(rssi, -90, -30, 0, 100);
    signalPercentage = constrain(signalPercentage, 0, 100);
    
    Blynk.virtualWrite(VPIN_WIFI_SIGNAL, signalPercentage);
}
