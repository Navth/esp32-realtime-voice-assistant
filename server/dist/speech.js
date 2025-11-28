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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.base64ToWavBuffer = base64ToWavBuffer;
exports.createWavHeader = createWavHeader;
exports.sendBufferedAudio = sendBufferedAudio;
const fs = __importStar(require("fs"));
// import openai from 'openai';
// import { createOpenAICompletion, openaiClient } from './openai';
const audio_1 = require("./audio");
// async function handleAudioData(
//   data: Buffer, 
//   audioBuffer: Buffer,
//   lastSendTime: number,
//   BUFFER_INTERVAL: number,
//   openAIWs: WebSocket,
//   currentFileWriter: wav.FileWriter,
//   triggerResponse: boolean
// ): Promise<AudioResponse> {
//   if (!currentFileWriter) {
//     return {
//       audioBuffer,
//       currentFileWriter,
//       lastSendTime,
//       responseBuffer: null
//     };
//   }
//   audioBuffer = Buffer.concat([audioBuffer, data]);
//   let responseBuffer: Buffer | null = null;
//   if (triggerResponse || Date.now() - lastSendTime >= BUFFER_INTERVAL) {
//     console.log(`Handling audio data: Buffer size ${audioBuffer.length} bytes, Time since last send: ${Date.now() - lastSendTime}ms`);
//     if (triggerResponse) {
//       console.log('Response triggered manually');
//     } else {
//       console.log('Buffer interval reached, processing audio');
//     }
//     responseBuffer = await processAudioWithOpenAI(currentFileWriter.path, openAIWs);
//     lastSendTime = Date.now();
//   }
//   return { audioBuffer, currentFileWriter, lastSendTime, responseBuffer };
// }
// Deprecated - using Gemini instead
// export async function processAudioWithOpenAI(audioFilePath: string): Promise<Buffer | null> {
//   try {
//     const fileBuffer = await readAudioFile(audioFilePath);
//     if (!fileBuffer) return null;
//     const response = await createOpenAICompletion(fileBuffer);
//     if (!response) return null;
//     if (!response.choices?.[0]?.message?.audio?.data) {
//       console.error('Invalid response format from OpenAI');
//       return null;
//     }
//     return base64ToWavBuffer(response.choices[0].message.audio.data);
//   } catch (error) {
//     console.error('Error processing audio with OpenAI:', error);
//     return null;
//   }
// }
function readAudioFile(audioFilePath) {
    return __awaiter(this, void 0, void 0, function* () {
        if (!fs.existsSync(audioFilePath)) {
            console.error(`Audio file does not exist: ${audioFilePath}`);
            return null;
        }
        const fileBuffer = fs.readFileSync(audioFilePath);
        console.log(`Successfully read audio file: ${audioFilePath}`);
        return fileBuffer;
    });
}
function base64ToWavBuffer(base64String, sampleRate) {
    try {
        const buffer = Buffer.from(base64String, 'base64');
        const wavHeader = createWavHeader(buffer.length, sampleRate);
        return Buffer.concat([wavHeader, buffer]);
    }
    catch (error) {
        console.error('Error converting base64 to WAV buffer:', error);
        return null;
    }
}
function base64ToWavBuffer44100(base64String) {
    try {
        const buffer = Buffer.from(base64String, 'base64');
        const wavHeader = createWavHeader(buffer.length, audio_1.SampleRate.RATE_44100);
        return Buffer.concat([wavHeader, buffer]);
    }
    catch (error) {
        console.error('Error converting base64 to WAV buffer:', error);
        return null;
    }
}
function base64ToWavBuffer24000(base64String) {
    try {
        const buffer = Buffer.from(base64String, 'base64');
        const wavHeader = createWavHeader(buffer.length, audio_1.SampleRate.RATE_24000);
        return Buffer.concat([wavHeader, buffer]);
    }
    catch (error) {
        console.error('Error converting base64 to WAV buffer:', error);
        return null;
    }
}
function base64ToWavBuffer22050(base64String) {
    try {
        const buffer = Buffer.from(base64String, 'base64');
        const wavHeader = createWavHeader(buffer.length, audio_1.SampleRate.RATE_22050);
        return Buffer.concat([wavHeader, buffer]);
    }
    catch (error) {
        console.error('Error converting base64 to WAV buffer:', error);
        return null;
    }
}
function createWavHeader(dataLength, sampleRate = audio_1.SampleRate.RATE_24000) {
    const wavHeader = Buffer.alloc(44);
    const numChannels = 1; // Mono
    const bitsPerSample = 16;
    const byteRate = sampleRate * numChannels * bitsPerSample / 8;
    const blockAlign = numChannels * bitsPerSample / 8;
    // RIFF chunk descriptor
    wavHeader.write('RIFF', 0);
    wavHeader.writeUInt32LE(36 + dataLength, 4);
    wavHeader.write('WAVE', 8);
    // Format chunk
    wavHeader.write('fmt ', 12);
    wavHeader.writeUInt32LE(16, 16); // Subchunk1Size
    wavHeader.writeUInt16LE(1, 20); // AudioFormat (PCM)
    wavHeader.writeUInt16LE(numChannels, 22);
    wavHeader.writeUInt32LE(sampleRate, 24);
    wavHeader.writeUInt32LE(byteRate, 28);
    wavHeader.writeUInt16LE(blockAlign, 32);
    wavHeader.writeUInt16LE(bitsPerSample, 34);
    // Data chunk
    wavHeader.write('data', 36);
    wavHeader.writeUInt32LE(dataLength, 40);
    return wavHeader;
}
function sendBufferedAudio(audioBuffer, openAIWs) {
    return __awaiter(this, void 0, void 0, function* () {
        if (audioBuffer.length === 0)
            return;
        //   try {
        //     console.log(`Sending ${audioBuffer.length} bytes of buffered audio to OpenAI`);
        //     const event = await audioToItemCreateEvent(audioBuffer);
        //     openAIWs.send(JSON.stringify(event));
        //     sendResponseCreateEvent(openAIWs);
        //     console.log('Sent buffered audio data to OpenAI');
        //   } catch (error) {
        //     console.error('Error processing audio buffer:', error);
        //   }
    });
}
