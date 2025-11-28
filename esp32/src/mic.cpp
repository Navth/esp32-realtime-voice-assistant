#include <Arduino.h>
#include <driver/i2s.h>
#include "lib_speaker.h"
#include "lib_websocket.h"
#include "utils.h"
#include "config.h"
#include <esp_task_wdt.h>

// Global flags for system state
bool isSpeakerBusy = false;
bool isWebSocketConnected = true;
int16_t soundBuffer[bufferLen];

bool isRecording = false;

void setRecording(bool recording)
{
  isRecording = recording;
}

void detectSound(int16_t *buffer, size_t length)
{
  if (!buffer || length == 0)
  {
    return;
  }

  for (const auto &lt : ledThresholds)
  {
    bool soundDetected = false;
    int16_t maxAmplitude = 0;

    // Find maximum amplitude in buffer
    for (size_t i = 0; i < length; i++)
    {
      int16_t amplitude = abs(buffer[i]);
      maxAmplitude = max(maxAmplitude, amplitude);

      if (amplitude > lt.threshold)
      {
        soundDetected = true;
      }
    }

    // Update LED state
    digitalWrite(lt.ledPin, soundDetected ? HIGH : LOW);

    // Update Blynk with audio level
    extern void updateBlynkAudioLevel(int level);
    if (isRecording && maxAmplitude > 0) {
      updateBlynkAudioLevel(maxAmplitude);
    }
  }
}

esp_err_t setupMicrophone()
{
  // i2s_driver_uninstall(I2S_PORT_MIC);
  i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = AUDIO_QUALITY_MIC,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 2,  // Further reduced for memory
      .dma_buf_len = 128,  // Further reduced for memory
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD};

  // Validate I2S port
  if (I2S_PORT_MIC < I2S_NUM_0 || I2S_PORT_MIC >= I2S_NUM_MAX)
  {
    Serial.println("Invalid I2S port number");
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t result = i2s_driver_install(I2S_PORT_MIC, &i2s_config, 0, NULL);
  if (result != ESP_OK)
  {
    Serial.printf("Error installing I2S driver: %s\n", esp_err_to_name(result));
    return result;
  }

  result = i2s_set_pin(I2S_PORT_MIC, &pin_config);
  if (result != ESP_OK)
  {
    Serial.printf("Error setting I2S pins: %s\n", esp_err_to_name(result));
    i2s_driver_uninstall(I2S_PORT_MIC);
    return result;
  }

  Serial.println("I2S microphone initialized successfully");
  return ESP_OK;
}

esp_err_t handleMicrophone()
{
  size_t bytes_read = 0;
  const size_t bufferSize = bufferLen;
  int16_t *buffer = (int16_t *)audio_malloc(bufferSize * sizeof(int16_t));

  if (!buffer)
  {
    Serial.println("Failed to allocate memory for audio buffer");
    return ESP_ERR_NO_MEM;
  }

  esp_err_t result = i2s_read(I2S_PORT_MIC, buffer, bufferSize * sizeof(int16_t), &bytes_read, portMAX_DELAY);
  if (result == ESP_OK && bytes_read > 0)
  {
    detectSound(buffer, bytes_read / sizeof(int16_t));
  }

  free(buffer);
  return result;
}

void micTask(void *parameter) {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1));
        
        if (isRecording) {
            size_t bytesIn = 0;
            esp_err_t result = i2s_read(I2S_PORT_MIC, &soundBuffer, bufferLen, &bytesIn, portMAX_DELAY);
            
            if (result == ESP_OK) {
                detectSound(soundBuffer, bytesIn / sizeof(int16_t));
                if (isWebSocketConnected) {
                    sendBinaryData(soundBuffer, bytesIn);
                }
            } else {
                Serial.printf("I2S read error: %d\n", result);
                delay(100);  // Add delay on error
            }
            
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(1));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
// void micTask(void *parameter)
// {
//   size_t bytesIn = 0;
//   const size_t bufferSize = bufferLen;
//   int32_t *rawBuffer = (int32_t *)audio_malloc(bufferSize * sizeof(int32_t));
//   int16_t *processedBuffer = (int16_t *)audio_malloc(bufferSize * sizeof(int16_t));
//   int16_t *resampledBuffer = (int16_t *)audio_malloc(bufferSize * 3 * sizeof(int16_t)); // Buffer for resampled audio

//   if (!rawBuffer || !processedBuffer || !resampledBuffer)
//   {
//     Serial.println("Failed to allocate memory for audio buffers");
//     if (rawBuffer) free(rawBuffer);
//     if (processedBuffer) free(processedBuffer);
//     if (resampledBuffer) free(resampledBuffer);
//     vTaskDelete(NULL);
//     return;
//   }

//   // Resampling state
//   float resampleRatio = 44100.0f / AUDIO_QUALITY_SPEAKER;
//   size_t accumulatedSamples = 0;

//   while (1)
//   {
//     if (isSpeakerBusy)
//     {
//       vTaskDelay(pdMS_TO_TICKS(10));
//       continue;
//     }

//     esp_err_t result = i2s_read(I2S_PORT_MIC, rawBuffer, bufferSize * sizeof(int32_t), &bytesIn, portMAX_DELAY);
//     if (result == ESP_OK && bytesIn > 0)
//     {
//       // Convert 32-bit samples to 16-bit and normalize
//       size_t samples = bytesIn / sizeof(int32_t);
//       for (size_t i = 0; i < samples; i++) {
//         processedBuffer[i] = (int16_t)(rawBuffer[i] >> 16);
//       }

//       detectSound(processedBuffer, samples);

//       if (isWebSocketConnected)
//       {
//         // Resample to 44.1kHz using linear interpolation
//         size_t resampledCount = 0;
//         for (size_t i = 0; i < samples - 1; i++)
//         {
//           float inputIndex = i * resampleRatio;
//           size_t inputIndexInt = (size_t)inputIndex;
//           float fraction = inputIndex - inputIndexInt;

//           // Linear interpolation between adjacent samples
//           int16_t sample1 = processedBuffer[i];
//           int16_t sample2 = processedBuffer[i + 1];
//           int16_t interpolatedSample = (int16_t)(sample1 + fraction * (sample2 - sample1));

//           resampledBuffer[resampledCount++] = interpolatedSample;
//         }

//         // Accumulate samples until we have enough for a proper buffer
//         if (accumulatedSamples + resampledCount >= bufferSize)
//         {
//           // Send full buffer worth of samples
//           size_t samplesToSend = bufferSize;
//           client.sendBinary((const char *)resampledBuffer, samplesToSend * sizeof(int16_t));

//           // Keep remaining samples for next buffer
//           if (accumulatedSamples + resampledCount > bufferSize) {
//             memmove(resampledBuffer,
//                    resampledBuffer + samplesToSend,
//                    (accumulatedSamples + resampledCount - bufferSize) * sizeof(int16_t));
//             accumulatedSamples = accumulatedSamples + resampledCount - bufferSize;
//           } else {
//             accumulatedSamples = 0;
//           }
//         } else {
//           accumulatedSamples += resampledCount;
//         }
//       }
//     }
//     else if (result != ESP_OK)
//     {
//       Serial.printf("Error reading from I2S: %d\n", result);
//       vTaskDelay(pdMS_TO_TICKS(10));
//     }
//     taskYIELD();
//   }

//   free(rawBuffer);
//   free(processedBuffer);
//   vTaskDelete(NULL);
// }
