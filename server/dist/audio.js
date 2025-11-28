"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.AudioManager = exports.SampleRate = void 0;
const wav = __importStar(require("wav"));
const fs = __importStar(require("fs"));
var SampleRate;
(function (SampleRate) {
    SampleRate[SampleRate["RATE_16000"] = 16000] = "RATE_16000";
    SampleRate[SampleRate["RATE_44100"] = 44100] = "RATE_44100";
    SampleRate[SampleRate["RATE_24000"] = 24000] = "RATE_24000";
    SampleRate[SampleRate["RATE_22050"] = 22050] = "RATE_22050";
})(SampleRate || (exports.SampleRate = SampleRate = {}));
class AudioManager {
    constructor() {
        this.configMediumDef = {
            sampleRate: 24000, // Match ESP32 sample rate from audioConfig[44100]
            channels: 1, // Mono audio from audioConfig
            bitDepth: 16 // 16-bit audio for Int16 codec
        };
        this.configLowDef = {
            sampleRate: 16000, // Match ESP32 sample rate from audioConfig[16000]
            channels: 1, // Mono audio from audioConfig
            bitDepth: 16 // 16-bit audio for Int16 codec
        };
        this.configHighDef = {
            sampleRate: 44100, // High definition sample rate
            channels: 1, // Mono audio
            bitDepth: 16 // 16-bit audio for Int16 codec
        };
        this.configUltraHighDef = {
            sampleRate: 96000, // Ultra high definition sample rate
            channels: 1, // Mono audio
            bitDepth: 16 // 16-bit audio for Int16 codec
        };
        this.writeTimeout = null;
        this.isProcessing = false;
        this.config = this.configLowDef;
        this.audioBuffer = Buffer.alloc(0);
        this.WRITE_DELAY = 500; // Reduced to 500ms for more responsive writes
        this.MAX_BUFFER_SIZE = 1024 * 1024; // 1MB max buffer size to prevent memory issues
        this.MIN_BUFFER_SIZE = this.config.sampleRate; // 1 second worth of audio data
        // Don't initialize file writer in constructor
    }
    initializeFileWriter(filename) {
        this.fileWriter = new wav.FileWriter(filename, {
            sampleRate: this.config.sampleRate,
            channels: this.config.channels,
            bitDepth: this.config.bitDepth
        });
    }
    startRecording() {
        // Generate random ID for filename
        const randomId = Math.random().toString(36).substring(2, 15);
        const filename = `recording-${randomId}.wav`;
        // Initialize new file writer with random filename
        this.initializeFileWriter(filename);
    }
    handleAudioBuffer(buffer, ws) {
        try {
            // Check if adding new buffer would exceed max size
            if (this.audioBuffer.length + buffer.length > this.MAX_BUFFER_SIZE) {
                // Process existing buffer before adding more
                this.processAndWriteBuffer();
            }
            // Concatenate incoming buffer with existing data
            this.audioBuffer = Buffer.concat([this.audioBuffer, buffer]);
            // Reset the write timeout
            if (this.writeTimeout) {
                clearTimeout(this.writeTimeout);
            }
            // Set new timeout to trigger write
            this.writeTimeout = setTimeout(() => {
                if (this.audioBuffer.length > 0) {
                    this.processAndWriteBuffer();
                }
            }, this.WRITE_DELAY);
            // If buffer exceeds minimum size, process immediately
            if (this.audioBuffer.length >= this.MIN_BUFFER_SIZE && !this.isProcessing) {
                this.processAndWriteBuffer();
            }
        }
        catch (error) {
            console.error('Error handling audio buffer:', error);
            // Log more details about the error
            if (error instanceof Error) {
                console.error('Error details:', error.message, error.stack);
            }
            this.audioBuffer = Buffer.alloc(0);
            this.isProcessing = false;
        }
    }
    processAndWriteBufferSimple() {
        if (!this.fileWriter || this.audioBuffer.length === 0 || this.isProcessing)
            return;
        try {
            this.isProcessing = true;
            // Write buffer and clear in one atomic operation
            const bufferToWrite = this.audioBuffer;
            this.audioBuffer = Buffer.alloc(0);
            this.fileWriter.write(bufferToWrite);
            // Log successful write
            console.log(`Successfully wrote ${bufferToWrite.length} bytes of audio data`);
        }
        catch (error) {
            console.error('Error writing audio buffer:', error);
        }
        finally {
            this.isProcessing = false;
        }
    }
    getCurrentBuffer() {
        if (!this.fileWriter) {
            console.error('No file writer available');
            return Buffer.alloc(0);
        }
        try {
            const audioFilePath = this.fileWriter.path;
            const fileBuffer = fs.readFileSync(audioFilePath);
            console.log(`Successfully read audio file: ${audioFilePath}`);
            return fileBuffer;
        }
        catch (error) {
            console.error('Error reading current audio buffer:', error);
            return Buffer.alloc(0);
        }
    }
    processAndWriteBuffer() {
        // return this.processAndWriteBufferWithGain();
        return this.processAndWriteBufferSimple();
    }
    processAndWriteBufferWithGain() {
        var _a;
        if (this.isProcessing || this.audioBuffer.length === 0)
            return;
        this.isProcessing = true;
        try {
            // Convert buffer to 16-bit PCM samples
            const samples = new Int16Array(this.audioBuffer.length / 2);
            for (let i = 0; i < this.audioBuffer.length; i += 2) {
                // Read samples directly without distortion
                const sample = this.audioBuffer.readInt16LE(i);
                samples[i / 2] = sample;
            }
            // Process audio only if there's meaningful data
            const maxAmplitude = Math.max(...Array.from(samples).map(Math.abs));
            if (maxAmplitude > 100) {
                const processedBuffer = this.processAudioSamples(samples, maxAmplitude);
                (_a = this.fileWriter) === null || _a === void 0 ? void 0 : _a.write(processedBuffer);
                console.log(`Processed and wrote ${this.audioBuffer.length} bytes of audio data`);
            }
            else {
                console.log('Skipping buffer - insufficient audio level');
            }
            // Clear the buffer after processing
            this.audioBuffer = Buffer.alloc(0);
        }
        finally {
            this.isProcessing = false;
        }
    }
    processAudioSamples(samples, maxAmplitude) {
        const processedSamples = new Int16Array(samples.length);
        const GAIN = 0.1; // Gain factor for normalization
        // Reduce normalization intensity
        const normalizeRatio = maxAmplitude > 0 ? (32767 / maxAmplitude) * GAIN : 1;
        const noiseFloor = 15; // Increased noise floor
        const maxVal = 32767 * 0.6; // Reduced maximum value to prevent clipping
        let prevSample = 0; // For simple low-pass filter
        const smoothingFactor = 0.1; // Adjust between 0 and 1
        for (let i = 0; i < samples.length; i++) {
            if (Math.abs(samples[i]) < noiseFloor) {
                processedSamples[i] = 0;
                continue;
            }
            let normalizedSample = samples[i] * normalizeRatio;
            // Apply simple low-pass filter
            normalizedSample = prevSample + smoothingFactor * (normalizedSample - prevSample);
            prevSample = normalizedSample;
            processedSamples[i] = Math.round(Math.max(Math.min(normalizedSample, maxVal), -maxVal));
        }
        return Buffer.from(processedSamples.buffer);
    }
    // public getCurrentBuffer(): Buffer {
    //     return this.audioBuffer;
    // }
    closeFile() {
        console.log('Closing WAV file writer');
        if (this.writeTimeout) {
            clearTimeout(this.writeTimeout);
        }
        if (this.audioBuffer.length > 0) {
            this.processAndWriteBuffer();
        }
        if (this.fileWriter) {
            this.fileWriter.end();
        }
    }
}
exports.AudioManager = AudioManager;
