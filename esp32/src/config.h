#ifndef CONFIG_H
#define CONFIG_H

// WiFi credentials

const int WEBSOCKET_PORT = 8888;
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const char* WEBSOCKET_HOST;
extern const char* BLYNK_AUTH_TOKEN;
// I2S Microphone pins
#define I2S_SD 32  // Serial Data (Blue wire)
#define I2S_WS 15  // Word Select (LRCLK) (Green wire)
#define I2S_SCK 14 // Serial Clock (Yellow wire)

// Speaker pins
#define I2S_SPEAKER_BCLK 14 // Bit Clock (Yellow wire - shared with mic SCK)
#define I2S_SPEAKER_LRC 15  // Left Right Clock (Word Select) (Green wire - shared with mic WS)
#define I2S_SPEAKER_DIN 22  // Data Input (Purple wire)

// LED pins - ESP32 Dev Kit typically has built-in LED on GPIO 2
// You can connect external LEDs to these pins if needed
#define LED_MIC 2   // Built-in LED for microphone activity (or connect external LED)
#define LED_SPKR 2  // Use same LED or connect external LED to different pin
// Button pin
#define BUTTON_PIN 4  // White wire

// I2S Microphone configuration
// #define SAMPLE_RATE 44100
#define SAMPLE_RATE 16000
#define SAMPLE_BITS 32
#define CHANNELS 1

#define I2S_PORT_MIC I2S_NUM_0
#define I2S_PORT_SPEAKER I2S_NUM_1

// Buffer configuration
#define bufferCnt 10
#define bufferLen 1024

// Audio detection thresholds
#define MIC_THRESHOLD 2300 // Adjust based on testing
#define LED_DELAY 1        // ms to keep LED on after sound stops

// Test tone configuration
#define TONE_FREQUENCY 440 // Hz (A4 note)
#define TONE_DURATION 2000 // ms
#define TONE_INTERVAL 5000 // ms
#define SAMPLES_PER_BUFFER 1024

struct LedThreshold
{
    int ledPin;
    int threshold;
};

const LedThreshold ledThresholds[] = {
    {LED_MIC, 100},
    // {LED_SPKR, 200},
};

enum AudioQuality
{
  LOW_DEFINITION = 16000,
  OPENAI_DEFINITION = 22050,
  MID_DEFINITION = 24000,
  HIGH_DEFINITION = 44100,
  ULTRA_HIGH_DEFINITION = 96000
};

const AudioQuality AUDIO_QUALITY_SPEAKER = AudioQuality::LOW_DEFINITION;
const AudioQuality AUDIO_QUALITY = AudioQuality::HIGH_DEFINITION;
const AudioQuality AUDIO_QUALITY_MIC = AudioQuality::HIGH_DEFINITION;


#endif