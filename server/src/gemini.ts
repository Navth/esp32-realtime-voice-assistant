import { GoogleGenerativeAI } from '@google/generative-ai';
import fs from 'fs';
import { SampleRate } from './audio';
import { createWavHeader } from './speech';
import axios from 'axios';

const GEMINI_API_KEY = 'AIzaSyCgnCZ3NJTl7qCVD8ZMGIyY6DhnlHCTjQE';
const genAI = new GoogleGenerativeAI(GEMINI_API_KEY);

/**
 * Convert text to speech using Google Text-to-Speech API
 */
async function textToSpeech(text: string): Promise<Buffer | null> {
  try {
    const response = await axios.post(
      `https://texttospeech.googleapis.com/v1/text:synthesize?key=${GEMINI_API_KEY}`,
      {
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
      }
    );

    const audioContent = response.data.audioContent;
    if (!audioContent) {
      console.error('No audio content in TTS response');
      return null;
    }

    // Convert base64 audio to buffer with WAV header
    const audioBuffer = Buffer.from(audioContent, 'base64');
    const wavHeader = createWavHeader(audioBuffer.length, SampleRate.RATE_16000);
    return Buffer.concat([wavHeader, audioBuffer]);
  } catch (error) {
    console.error('Error in text-to-speech conversion:', error);
    return null;
  }
}

/**
 * Process audio buffer with Gemini (transcribe + generate response)
 * Returns audio buffer as response
 */
export async function createGeminiCompletion(fileBuffer: Buffer): Promise<Buffer | null> {
  try {
    // Use Gemini Flash model for audio processing
    const model = genAI.getGenerativeModel({ model: 'gemini-1.5-flash' });

    // Convert audio buffer to base64
    const base64Audio = fileBuffer.toString('base64');

    // Create the content with audio
    const result = await model.generateContent([
      {
        inlineData: {
          data: base64Audio,
          mimeType: 'audio/wav'
        }
      },
      'You are a helpful AI assistant. Please listen to the audio input, transcribe it, and provide a relevant and concise response. If the audio is unclear or there\'s no speech detected, kindly ask for clarification.'
    ]);

    const response = await result.response;
    const text = response.text();
    console.log('Gemini response text:', text);

    // Convert text response to speech
    const audioBuffer = await textToSpeech(text);
    console.log('Generated audio buffer:', audioBuffer?.length, 'bytes');

    return audioBuffer;
  } catch (error) {
    console.error('Error creating Gemini completion:', error);
    return null;
  }
}

/**
 * Process audio buffer with Gemini in streaming mode
 * Streams audio chunks directly to clients
 */
export async function* createGeminiCompletionStream(fileBuffer: Buffer): AsyncGenerator<Buffer, void, unknown> {
  try {
    const model = genAI.getGenerativeModel({ model: 'gemini-1.5-flash' });

    const base64Audio = fileBuffer.toString('base64');

    const result = await model.generateContentStream([
      {
        inlineData: {
          data: base64Audio,
          mimeType: 'audio/wav'
        }
      },
      'You are a helpful AI assistant. Please listen to the audio input, transcribe it, and provide a relevant and concise response. If the audio is unclear or there\'s no speech detected, kindly ask for clarification.'
    ]);

    let fullText = '';
    
    // Stream text chunks
    for await (const chunk of result.stream) {
      const chunkText = chunk.text();
      fullText += chunkText;
      console.log('Received text chunk from Gemini:', chunkText);
    }

    console.log('Complete Gemini response:', fullText);

    // Convert complete text to speech
    const audioBuffer = await textToSpeech(fullText);
    
    if (audioBuffer) {
      // Yield audio in chunks
      const CHUNK_SIZE = 2048;
      for (let i = 0; i < audioBuffer.length; i += CHUNK_SIZE) {
        yield audioBuffer.slice(i, i + CHUNK_SIZE);
      }
    }
  } catch (error) {
    console.error('Error creating Gemini completion stream:', error);
  }
}
