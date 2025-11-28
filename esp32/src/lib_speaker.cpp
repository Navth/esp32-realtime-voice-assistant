#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include "config.h"
#include "lib_speaker.h"
#include "lib_websocket.h"
#include "lib_button.h"
#include "mic.h"
#include "audioMemoryBuffer.h"
// At the top of the file
static bool is_speaker_installed = false;
static bool is_mic_installed = false;
// Constants
const size_t SAMPLES_PER_WRITE = 1024;
const float TONE_VOLUME_PERCENT = 0.02;
const float MAX_AMPLITUDE = 32767.0f;

// Global state variables
unsigned long lastMicActivity = 0;
unsigned long lastSpkrActivity = 0;
unsigned long lastToneTime = 0;
bool isPlayingTone = false;
unsigned long toneStartTime = 0;
AudioMemoryBuffer audioMemoryBuffer = AudioMemoryBuffer();
uint8_t speakerdata0[1024 * 1];
int speaker_offset;
int data_offset;
void writeToAudioBuffer(int16_t *buffer, size_t samples)
{
  // Write samples to both left and right channels
  int16_t stereoBuffer[samples * 2];
  for (size_t i = 0; i < samples; i++)
  {
    stereoBuffer[i * 2] = buffer[i];     // Left channel
    stereoBuffer[i * 2 + 1] = buffer[i]; // Right channel
  }

  size_t bytes_written = 0;
  esp_err_t result = i2s_write(I2S_PORT_SPEAKER, stereoBuffer, samples * 4, &bytes_written, portMAX_DELAY);

  if (result != ESP_OK)
  {
    Serial.println("Error writing to I2S speaker");
  }

  lastSpkrActivity = millis();
}

// I2S configuration helper
esp_err_t configureI2S(const i2s_config_t &config, const i2s_pin_config_t &pins)
{
  esp_err_t result = i2s_driver_install(I2S_PORT_SPEAKER, &config, 0, NULL);
  if (result != ESP_OK)
  {
    Serial.println("Error installing I2S speaker driver");
    return result;
  }

  result = i2s_set_pin(I2S_PORT_SPEAKER, &pins);
  if (result != ESP_OK)
  {
    Serial.println("Error setting I2S speaker pins");
    return result;
  }

  return ESP_OK;
}

/*
void InitI2SSpeakerOrMic(AudioMode mode)
{
    Serial.printf("Initializing I2S for mode: %s\n", mode == MODE_MIC ? "Microphone" : "Speaker");
    try {
        // if (mode == MODE_MIC && is_speaker_installed) {
        //     i2s_stop(I2S_PORT_SPEAKER);
        //     i2s_driver_uninstall(I2S_PORT_SPEAKER);
        //     is_speaker_installed = false;
        // } else if (mode == MODE_SPK && is_mic_installed) {
        //     i2s_stop(I2S_PORT_MIC);
        //     i2s_driver_uninstall(I2S_PORT_MIC);
        //     is_mic_installed = false;
        // }
        
        delay(50);

        if (mode == MODE_MIC && !is_mic_installed)
        {
            if (setupMicrophone() == ESP_OK) {
                i2s_start(I2S_PORT_MIC);
                is_mic_installed = true;
                digitalWrite(LED_MIC, HIGH);
                digitalWrite(LED_SPKR, LOW);
            }
        }
        else if (mode == MODE_SPK && !is_speaker_installed)
        {
            if (setupSpeakerI2S() == ESP_OK) {
                i2s_start(I2S_PORT_SPEAKER);
                is_speaker_installed = true;
                digitalWrite(LED_MIC, LOW);
                digitalWrite(LED_SPKR, HIGH);
            }
        }
    } catch (const std::exception& e) {
        Serial.printf("Error initializing I2S: %s\n", e.what());
    }
}
*/
// ... existing code ...

void InitI2SSpeakerOrMic(AudioMode mode)
{
    Serial.printf("Initializing I2S for mode: %s\n", mode == MODE_MIC ? "Microphone" : "Speaker");
    esp_err_t err = ESP_OK;

    // Use different ports for mic and speaker
    i2s_port_t port = (mode == MODE_MIC) ? I2S_PORT_MIC : I2S_PORT_SPEAKER;
    
    // Uninstall existing driver for the specific port only
    i2s_driver_uninstall(port);

    // Base I2S config
    i2s_config_t i2s_config = {
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
#else
        .communication_format = I2S_COMM_FORMAT_I2S,
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = bufferCnt,
        .dma_buf_len = bufferLen,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    // Mode-specific configurations
    if (mode == MODE_MIC)
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
        i2s_config.sample_rate = AUDIO_QUALITY_MIC;
        i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.sample_rate = AUDIO_QUALITY_SPEAKER;
        i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    }

    // Install I2S driver for the specific port
    err = i2s_driver_install(port, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S driver: %s\n", esp_err_to_name(err));
        return;
    }

    // Configure pins
    i2s_pin_config_t pin_config;
#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
    pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif

    if (mode == MODE_MIC) {
        pin_config.bck_io_num = I2S_SCK;
        pin_config.ws_io_num = I2S_WS;
        pin_config.data_out_num = I2S_PIN_NO_CHANGE;
        pin_config.data_in_num = I2S_SD;
    } else {
        pin_config.bck_io_num = I2S_SPEAKER_BCLK;
        pin_config.ws_io_num = I2S_SPEAKER_LRC;
        pin_config.data_out_num = I2S_SPEAKER_DIN;
        pin_config.data_in_num = I2S_PIN_NO_CHANGE;
    }

    err = i2s_set_pin(port, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S pins: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(port);
        return;
    }

    // Set clock
    err = i2s_set_clk(port, 
                      (mode == MODE_MIC) ? AUDIO_QUALITY_MIC : AUDIO_QUALITY_SPEAKER,
                      I2S_BITS_PER_SAMPLE_16BIT,
                      (mode == MODE_MIC) ? I2S_CHANNEL_MONO : I2S_CHANNEL_STEREO);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S clock: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(port);
        return;
    }

    // Start I2S
    err = i2s_start(port);
    if (err != ESP_OK) {
        Serial.printf("Failed to start I2S: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(port);
        return;
    }

    // Update state flags
    if (mode == MODE_MIC) {
        is_mic_installed = true;
        digitalWrite(LED_MIC, HIGH);
    } else {
        is_speaker_installed = true;
        digitalWrite(LED_SPKR, HIGH);
    }

    Serial.println("I2S initialized successfully");
}
esp_err_t setupSpeakerI2S()
{
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = AUDIO_QUALITY_SPEAKER,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 2,  // Further reduced for memory
      .dma_buf_len = 256,  // Further reduced for memory
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SPEAKER_BCLK,
      .ws_io_num = I2S_SPEAKER_LRC,
      .data_out_num = I2S_SPEAKER_DIN,
      .data_in_num = I2S_PIN_NO_CHANGE};

  // Check if I2S port is valid
  if (I2S_PORT_SPEAKER < I2S_NUM_0 || I2S_PORT_SPEAKER >= I2S_NUM_MAX)
  {
    Serial.println("Invalid I2S port");
    return ESP_ERR_INVALID_ARG;
  }

  // Install I2S driver
  esp_err_t i2s_err = i2s_driver_install(I2S_PORT_SPEAKER, &i2s_config, 0, NULL);
  if (i2s_err != ESP_OK)
  {
    Serial.printf("Failed to install I2S driver: %s\n", esp_err_to_name(i2s_err));
    return i2s_err;
  }

  // Set I2S pins
  i2s_err = i2s_set_pin(I2S_PORT_SPEAKER, &pin_config);
  if (i2s_err != ESP_OK)
  {
    Serial.printf("Failed to set I2S pins: %s\n", esp_err_to_name(i2s_err));
    i2s_driver_uninstall(I2S_PORT_SPEAKER);
    return i2s_err;
  }

  Serial.println("I2S initialized successfully");
  return ESP_OK;
}

// void setupSpeaker()
// {
//   // First uninstall any existing driver
//   i2s_driver_uninstall(I2S_PORT_SPEAKER);

//   i2s_config_t i2s_speaker_config = {
//       .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // Make sure this is TX only
//       .sample_rate = AUDIO_QUALITY_SPEAKER,
//       .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
//       .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
//       .communication_format = I2S_COMM_FORMAT_STAND_I2S, // Update to STAND_I2S
//       .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
//       .dma_buf_count = bufferCnt,
//       .dma_buf_len = bufferLen,
//       .use_apll = false,
//       .tx_desc_auto_clear = true,
//       .fixed_mclk = 0};

//   i2s_pin_config_t speaker_pins = {
//       .bck_io_num = I2S_SPEAKER_BCLK,
//       .ws_io_num = I2S_SPEAKER_LRC,
//       .data_out_num = I2S_SPEAKER_DIN,
//       .data_in_num = I2S_PIN_NO_CHANGE};

//   // Validate I2S port
//   if (I2S_PORT_SPEAKER < I2S_NUM_0 || I2S_PORT_SPEAKER >= I2S_NUM_MAX)
//   {
//     Serial.println("Invalid I2S port number");
//     return;
//   }

//   esp_err_t result = i2s_driver_install(I2S_PORT_SPEAKER, &i2s_speaker_config, 0, NULL);
//   if (result != ESP_OK)
//   {
//     Serial.printf("Error installing I2S driver: %s\n", esp_err_to_name(result));
//     return;
//   }

//   result = i2s_set_pin(I2S_PORT_SPEAKER, &speaker_pins);
//   if (result != ESP_OK)
//   {
//     Serial.printf("Error setting I2S pins: %s\n", esp_err_to_name(result));
//     // Clean up if pin config fails
//     i2s_driver_uninstall(I2S_PORT_SPEAKER);
//     return;
//   }

//   Serial.println("I2S speaker initialized successfully");
// }

void generateSimpleTone(int16_t *buffer, size_t samples)
{
  const float amplitude = MAX_AMPLITUDE * TONE_VOLUME_PERCENT;
  const float angular_frequency = 2 * PI * TONE_FREQUENCY;
  static float phase = 0;

  int16_t tempBuffer[samples];
  for (size_t i = 0; i < samples; i++)
  {
    tempBuffer[i] = amplitude * sin(phase);
    phase += angular_frequency / AUDIO_QUALITY_SPEAKER;
    if (phase >= 2 * PI)
    {
      phase -= 2 * PI;
    }
  }

  audioMemoryBuffer.write(tempBuffer, samples);
  audioMemoryBuffer.read(buffer, samples);
}
void generateTone(int16_t *buffer, size_t samples)
{
  const float amplitude = MAX_AMPLITUDE * TONE_VOLUME_PERCENT;
  static float time = 0;
  static float phase = 0;

  // Base frequency modulated by a slow sine wave
  const float base_freq = 440.0f;  // Base frequency in Hz
  const float mod_freq = 0.5f;     // Modulation frequency in Hz
  const float freq_depth = 200.0f; // Frequency deviation in Hz

  int16_t tempBuffer[samples];
  for (size_t i = 0; i < samples; i++)
  {
    // Calculate current frequency using sinusoidal modulation
    float current_freq = base_freq + freq_depth * sin(2 * PI * mod_freq * time);

    tempBuffer[i] = amplitude * sin(phase);
    phase += (2 * PI * current_freq) / AUDIO_QUALITY_SPEAKER;
    if (phase >= 2 * PI)
    {
      phase -= 2 * PI;
    }

    time += 1.0f / AUDIO_QUALITY_SPEAKER;
  }

  audioMemoryBuffer.write(tempBuffer, samples);
  audioMemoryBuffer.read(buffer, samples); // Uncommented this line to properly output the sound
}
void playBufferWithOffset(uint8_t *payload, size_t length)
{
  memcpy(speakerdata0 + speaker_offset, payload, length);
  speaker_offset += length;
  size_t bytes_written;
  i2s_write(I2S_PORT_SPEAKER, speakerdata0, speaker_offset, &bytes_written, portMAX_DELAY);
  speaker_offset = 0;
}

void playBuffer(int16_t *buffer, size_t samples)
{
  size_t bytes_written = 0;
  esp_err_t result = i2s_write(I2S_PORT_SPEAKER, buffer, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
  // sendBinaryData(buffer, samples * sizeof(int16_t));

  if (result != ESP_OK)
  {
    Serial.println("Error writing to I2S");
  }
}
void speaker_play(uint8_t *payload, uint32_t len)
{
  const float volume = 0.7f;
  const float pitch = 0.8f; // 1.0 = normal speed, >1 = faster, <1 = slower
  Serial.printf("received %lu bytes", len);
  Serial.println();
  size_t bytes_written;

  // Create a buffer to store modified samples
  int16_t *samples = (int16_t *)payload;
  size_t num_samples = len / sizeof(int16_t);

  // Calculate new buffer size based on pitch
  size_t new_num_samples = (size_t)(num_samples / pitch);
  int16_t *pitched_samples = new int16_t[new_num_samples];

  // Resample audio for pitch/speed adjustment
  for (size_t i = 0; i < new_num_samples; i++)
  {
    float original_index = i * pitch;
    size_t index = (size_t)original_index;
    if (index < num_samples)
    {
      pitched_samples[i] = (int16_t)(samples[index] * volume);
    }
  }

  // InitI2SSpeakerOrMic(MODE_SPK);
  i2s_write(I2S_PORT_SPEAKER, pitched_samples, new_num_samples * sizeof(int16_t),
            &bytes_written, portMAX_DELAY);

  delete[] pitched_samples;

  // After playback completes, switch back to mic mode
  // InitI2SSpeakerOrMic(MODE_MIC);
  Serial.println("Playback complete, switched back to mic mode");
}

void updateToneState()
{
  unsigned long currentTime = millis();

  if (!isPlayingTone && currentTime - lastToneTime >= TONE_INTERVAL)
  {
    isPlayingTone = true;
    toneStartTime = currentTime;
    lastToneTime = currentTime;
    digitalWrite(LED_SPKR, HIGH);
  }

  if (isPlayingTone && currentTime - toneStartTime >= TONE_DURATION)
  {
    isPlayingTone = false;
    digitalWrite(LED_SPKR, LOW);
  }
}
// Audio library functions - DISABLED to save memory
// void setupAudio()
// {
//   // setupSpeakerI2S();  // Call this first
//   delay(100);

//   audio.setPinout(I2S_SPEAKER_BCLK, I2S_SPEAKER_LRC, I2S_SPEAKER_DIN);
//   audio.setVolume(2);
//   // audio.
//   audio.connecttohost("http://vis.media-ice.musicradio.com/CapitalMP3");
// }

// void loopAudio()
// {
//   audio.loop();
// }

// void setVolume(int volume)
// {
//   if (volume >= 0 && volume <= 21)
//   {
//     audio.setVolume(volume);
//     Serial.printf("Volume set to %d\n", volume);
//   }
//   else
//   {
//     Serial.println("Invalid volume level. Please use a value between 0 and 21.");
//   }
// }

void playTestTone()
{
  static int16_t tone_buffer[SAMPLES_PER_WRITE];

  updateToneState();

  if (isPlayingTone)
  {
    generateTone(tone_buffer, SAMPLES_PER_WRITE);
    playBuffer(tone_buffer, SAMPLES_PER_WRITE);

    // delayMicroseconds(100);
  }
}

void handleSpeaker()
{
  static int16_t audio_buffer[SAMPLES_PER_WRITE];

  if (audioMemoryBuffer.available() > 0 && audioMemoryBuffer.read(audio_buffer, SAMPLES_PER_WRITE))
  {
    // playBuffer(audio_buffer, SAMPLES_PER_WRITE);
  }
  else
  {
    playTestTone();
  }
  // delay(10);
}
