# Blynk Dashboard Setup Guide

## Step 1: Create Blynk Account
1. Go to https://blynk.cloud/
2. Sign up for a free account
3. Create a new template or use quickstart

## Step 2: Get Your Auth Token
1. In Blynk.Console, go to your device
2. Copy the **Auth Token**
3. Paste it in `esp32/src/credentials.cpp`:
   ```cpp
   const char* BLYNK_AUTH_TOKEN = "YOUR_TOKEN_HERE";
   ```

## Step 3: Configure Dashboard Widgets

Add these widgets to your Blynk dashboard:

### Widget 1: Recording Status (LED)
- **Type:** LED
- **Virtual Pin:** V0
- **Name:** Recording
- **Color:** Red
- **Shows:** When device is recording audio

### Widget 2: Connection Status (LED)
- **Type:** LED
- **Virtual Pin:** V1
- **Name:** Connected
- **Color:** Green
- **Shows:** WebSocket connection status

### Widget 3: Last AI Response (Label/Terminal)
- **Type:** Label or Terminal
- **Virtual Pin:** V2
- **Name:** AI Response
- **Shows:** Latest response from Gemini AI

### Widget 4: Audio Level (Gauge)
- **Type:** Gauge
- **Virtual Pin:** V3
- **Name:** Audio Level
- **Range:** 0-100
- **Shows:** Current microphone audio level

### Widget 5: Manual Record (Button)
- **Type:** Button (Switch mode)
- **Virtual Pin:** V4
- **Name:** Manual Record
- **Shows:** Allows recording control from Blynk app

### Widget 6: Response Time (Value Display)
- **Type:** Value Display
- **Virtual Pin:** V5
- **Name:** Response Time (ms)
- **Shows:** How long Gemini took to process

### Widget 7: Error Log (Terminal)
- **Type:** Terminal
- **Virtual Pin:** V6
- **Name:** Errors
- **Shows:** System error messages

### Widget 8: WiFi Signal (Gauge)
- **Type:** Gauge
- **Virtual Pin:** V7
- **Name:** WiFi Signal
- **Range:** 0-100
- **Shows:** WiFi signal strength percentage

## Step 4: Upload Code
1. Update your Blynk auth token in `credentials.cpp`
2. Build and upload:
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM32
   ```

## Step 5: Monitor
Open Blynk mobile app or web dashboard to see real-time status!

## Features
✅ **Cloud Processing:** Audio is recorded on ESP32, sent to your server, processed by Gemini AI in Google Cloud
✅ **Real-time Monitoring:** See recording status, audio levels, and WiFi signal
✅ **Remote Control:** Start/stop recording from Blynk app anywhere
✅ **Response Tracking:** See AI responses and processing times
✅ **Error Logging:** Debug issues remotely through Blynk dashboard
