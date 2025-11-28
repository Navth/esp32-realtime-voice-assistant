#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>
#include <math.h>
#include "mic.h"
#include "config.h"
#include "lib_wifi.h"
#include "utils.h"
#include "lib_speaker.h"
#include "lib_button.h"
#include "lib_websocket.h"
#include "lib_blynk.h"

int16_t sBuffer[bufferLen];
ButtonChecker button;
bool manualRecordTrigger = false;
int16_t maxAudioLevel = 0;
unsigned long recordingStartTime = 0;
bool currentlyRecording = false;

// Function declarations
void setupLEDs();
void setupAudioIO();
void triggerManualRecord(bool state);

void triggerManualRecord(bool state) {
  manualRecordTrigger = state;
}

void setupLEDs()
{
  pinMode(LED_MIC, OUTPUT);
  pinMode(LED_SPKR, OUTPUT);
  digitalWrite(LED_MIC, LOW);
  digitalWrite(LED_SPKR, LOW);
}
void setupAudioIO()
{
  // Uninstall any existing I2S drivers
  // i2s_driver_uninstall(I2S_PORT_SPEAKER);
  i2s_driver_uninstall(I2S_PORT_MIC);
  
  // Setup speaker first
  setupSpeakerI2S();
  // i2s_start(I2S_PORT_SPEAKER);
  delay(200);  // Increased delay for better initialization
  
  // Then setup microphone
  setupMicrophone();
    delay(200);  // Increased delay for better initialization

  // i2s_start(I2S_PORT_MIC);
  delay(200);  // Increased delay for better initialization
}
void setup()
{
  Serial.begin(115200);
  setupLEDs();
  connectToWiFi();
  
  // Initialize Blynk after WiFi - DISABLED DUE TO MEMORY CONSTRAINTS
  // setupBlynk();
  // updateBlynkConnectionStatus(false);
  
  connectToWebSocket();
  // updateBlynkConnectionStatus(true);
  
  setRecording(false);
  setupAudioIO();

  xTaskCreatePinnedToCore(micTask, "micTask", 4096, NULL, 1, NULL, 0);  // Reduced stack
}

void loop()
{
  button.loop();
  // loopBlynk(); // Process Blynk events - DISABLED DUE TO MEMORY CONSTRAINTS
  
  // Handle physical button or manual Blynk trigger
  bool shouldStartRecording = button.justPressed() || (manualRecordTrigger && !currentlyRecording);
  bool shouldStopRecording = button.justReleased() || (!manualRecordTrigger && currentlyRecording && recordingStartTime > 0);
  
  if (shouldStartRecording)
  {
    Serial.println("Recording...");
    sendMessage("START_RECORD");
    sendButtonState(1);
    recordingStartTime = millis();
    maxAudioLevel = 0;
    
    // Update Blynk status - DISABLED
    // updateBlynkRecordingStatus(true);
    
    // Stop speaker and clear buffer before starting mic
    i2s_stop(I2S_PORT_SPEAKER);
    i2s_zero_dma_buffer(I2S_PORT_SPEAKER);
    delay(100);
    
    i2s_start(I2S_PORT_MIC);
    delay(100);
    
    setRecording(true);
    currentlyRecording = true;
    Serial.println("Recording ready.");
  }
  else if (shouldStopRecording)
  {
    Serial.println("Stopped recording.");
    sendButtonState(0);
    sendMessage("STOP_RECORD");
    
    unsigned long processingTime = millis() - recordingStartTime;
    // updateBlynkResponseTime(processingTime); // DISABLED
    // updateBlynkRecordingStatus(false); // DISABLED
    
    setRecording(false);
    currentlyRecording = false;
    recordingStartTime = 0;

    // Stop microphone and clear buffer before starting speaker
    i2s_stop(I2S_PORT_MIC);
    i2s_zero_dma_buffer(I2S_PORT_MIC);
    delay(100);
    
    i2s_start(I2S_PORT_SPEAKER);
    delay(100);
  }

  loopWebsocket();
}