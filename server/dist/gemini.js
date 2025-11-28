"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __await = (this && this.__await) || function (v) { return this instanceof __await ? (this.v = v, this) : new __await(v); }
var __asyncValues = (this && this.__asyncValues) || function (o) {
    if (!Symbol.asyncIterator) throw new TypeError("Symbol.asyncIterator is not defined.");
    var m = o[Symbol.asyncIterator], i;
    return m ? m.call(o) : (o = typeof __values === "function" ? __values(o) : o[Symbol.iterator](), i = {}, verb("next"), verb("throw"), verb("return"), i[Symbol.asyncIterator] = function () { return this; }, i);
    function verb(n) { i[n] = o[n] && function (v) { return new Promise(function (resolve, reject) { v = o[n](v), settle(resolve, reject, v.done, v.value); }); }; }
    function settle(resolve, reject, d, v) { Promise.resolve(v).then(function(v) { resolve({ value: v, done: d }); }, reject); }
};
var __asyncGenerator = (this && this.__asyncGenerator) || function (thisArg, _arguments, generator) {
    if (!Symbol.asyncIterator) throw new TypeError("Symbol.asyncIterator is not defined.");
    var g = generator.apply(thisArg, _arguments || []), i, q = [];
    return i = Object.create((typeof AsyncIterator === "function" ? AsyncIterator : Object).prototype), verb("next"), verb("throw"), verb("return", awaitReturn), i[Symbol.asyncIterator] = function () { return this; }, i;
    function awaitReturn(f) { return function (v) { return Promise.resolve(v).then(f, reject); }; }
    function verb(n, f) { if (g[n]) { i[n] = function (v) { return new Promise(function (a, b) { q.push([n, v, a, b]) > 1 || resume(n, v); }); }; if (f) i[n] = f(i[n]); } }
    function resume(n, v) { try { step(g[n](v)); } catch (e) { settle(q[0][3], e); } }
    function step(r) { r.value instanceof __await ? Promise.resolve(r.value.v).then(fulfill, reject) : settle(q[0][2], r); }
    function fulfill(value) { resume("next", value); }
    function reject(value) { resume("throw", value); }
    function settle(f, v) { if (f(v), q.shift(), q.length) resume(q[0][0], q[0][1]); }
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.createGeminiCompletion = createGeminiCompletion;
exports.createGeminiCompletionStream = createGeminiCompletionStream;
const generative_ai_1 = require("@google/generative-ai");
const audio_1 = require("./audio");
const speech_1 = require("./speech");
const axios_1 = __importDefault(require("axios"));
const GEMINI_API_KEY = 'AIzaSyCgnCZ3NJTl7qCVD8ZMGIyY6DhnlHCTjQE';
const genAI = new generative_ai_1.GoogleGenerativeAI(GEMINI_API_KEY);
/**
 * Convert text to speech using Google Text-to-Speech API
 */
function textToSpeech(text) {
    return __awaiter(this, void 0, void 0, function* () {
        try {
            const response = yield axios_1.default.post(`https://texttospeech.googleapis.com/v1/text:synthesize?key=${GEMINI_API_KEY}`, {
                input: { text },
                voice: {
                    languageCode: 'en-US',
                    name: 'en-US-Neural2-C',
                    ssmlGender: 'FEMALE'
                },
                audioConfig: {
                    audioEncoding: 'LINEAR16',
                    sampleRateHertz: 16000
                }
            });
            const audioContent = response.data.audioContent;
            if (!audioContent) {
                console.error('No audio content in TTS response');
                return null;
            }
            // Convert base64 audio to buffer with WAV header
            const audioBuffer = Buffer.from(audioContent, 'base64');
            const wavHeader = (0, speech_1.createWavHeader)(audioBuffer.length, audio_1.SampleRate.RATE_16000);
            return Buffer.concat([wavHeader, audioBuffer]);
        }
        catch (error) {
            console.error('Error in text-to-speech conversion:', error);
            return null;
        }
    });
}
/**
 * Process audio buffer with Gemini (transcribe + generate response)
 * Returns audio buffer as response
 */
function createGeminiCompletion(fileBuffer) {
    return __awaiter(this, void 0, void 0, function* () {
        try {
            // Use Gemini Flash model for audio processing
            const model = genAI.getGenerativeModel({ model: 'gemini-2.0-flash-exp' });
            // Convert audio buffer to base64
            const base64Audio = fileBuffer.toString('base64');
            // Create the content with audio
            const result = yield model.generateContent([
                {
                    inlineData: {
                        data: base64Audio,
                        mimeType: 'audio/wav'
                    }
                },
                'You are a helpful AI assistant. Please listen to the audio input, transcribe it, and provide a relevant and concise response. If the audio is unclear or there\'s no speech detected, kindly ask for clarification.'
            ]);
            const response = yield result.response;
            const text = response.text();
            console.log('Gemini response text:', text);
            // Convert text response to speech
            const audioBuffer = yield textToSpeech(text);
            console.log('Generated audio buffer:', audioBuffer === null || audioBuffer === void 0 ? void 0 : audioBuffer.length, 'bytes');
            return audioBuffer;
        }
        catch (error) {
            console.error('Error creating Gemini completion:', error);
            return null;
        }
    });
}
/**
 * Process audio buffer with Gemini in streaming mode
 * Streams audio chunks directly to clients
 */
function createGeminiCompletionStream(fileBuffer) {
    return __asyncGenerator(this, arguments, function* createGeminiCompletionStream_1() {
        var _a, e_1, _b, _c;
        try {
            const model = genAI.getGenerativeModel({ model: 'gemini-2.0-flash-exp' });
            const base64Audio = fileBuffer.toString('base64');
            const result = yield __await(model.generateContentStream([
                {
                    inlineData: {
                        data: base64Audio,
                        mimeType: 'audio/wav'
                    }
                },
                'You are a helpful AI assistant. Please listen to the audio input, transcribe it, and provide a relevant and concise response. If the audio is unclear or there\'s no speech detected, kindly ask for clarification.'
            ]));
            let fullText = '';
            try {
                // Stream text chunks
                for (var _d = true, _e = __asyncValues(result.stream), _f; _f = yield __await(_e.next()), _a = _f.done, !_a; _d = true) {
                    _c = _f.value;
                    _d = false;
                    const chunk = _c;
                    const chunkText = chunk.text();
                    fullText += chunkText;
                    console.log('Received text chunk from Gemini:', chunkText);
                }
            }
            catch (e_1_1) { e_1 = { error: e_1_1 }; }
            finally {
                try {
                    if (!_d && !_a && (_b = _e.return)) yield __await(_b.call(_e));
                }
                finally { if (e_1) throw e_1.error; }
            }
            console.log('Complete Gemini response:', fullText);
            // Convert complete text to speech
            const audioBuffer = yield __await(textToSpeech(fullText));
            if (audioBuffer) {
                // Yield audio in chunks
                const CHUNK_SIZE = 2048;
                for (let i = 0; i < audioBuffer.length; i += CHUNK_SIZE) {
                    yield yield __await(audioBuffer.slice(i, i + CHUNK_SIZE));
                }
            }
        }
        catch (error) {
            console.error('Error creating Gemini completion stream:', error);
        }
    });
}
