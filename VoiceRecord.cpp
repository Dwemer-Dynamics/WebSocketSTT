#include <ixwebsocket/IXWebSocket.h>
#include <portaudio.h>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <fstream>
#include <sstream>
#include <windows.h>


#include "VoiceRecordControl.h"  // Your header
#include "json.hpp"

#include "RE/Skyrim.h"

#pragma comment(lib, "winmm.lib")

namespace logger = SKSE::log;  // Your logger

// Audio format constants
constexpr int SAMPLE_RATE = 16000;
constexpr int FRAMES_PER_BUFFER = 512;
constexpr int NUM_CHANNELS = 1;
constexpr PaSampleFormat SAMPLE_FORMAT = paInt16;

// Deepgram WebSocket URL and API key (replace with your actual key)
const std::string DEEPGRAM_WS_URL = "wss://api.deepgram.com/v1/listen";


// WebSocket instance and synchronization primitives
static ix::WebSocket websocket;
static std::mutex websocketMutex;
static std::condition_variable websocketCV;
static bool websocketOpen = false;


struct AudioData {
    std::atomic<bool> recording;
};

class Transcript {  // Or wherever you want to put this string
private:
    std::mutex transcriptMutex;  // Mutex to protect transcriptBuffer
    std::string transcriptBuffer;

public:
    

    void add(const std::string& message) {
        std::lock_guard<std::mutex> lock(transcriptMutex);  // Lock the mutex
        transcriptBuffer += message+" ";                        // Append to the buffer
        // ... other processing (e.g., check for complete utterance, etc.)
    }

    std::string GetAndClearTranscript() {
        std::lock_guard<std::mutex> lock(transcriptMutex);  // Lock the mutex
        std::string transcript = transcriptBuffer;          // Copy the buffer
        transcriptBuffer.clear();                           // Clear the buffer
        return transcript;                                  // Return the copied string
    }

    
};

static Transcript* message_to_send;


std::string ReadAPIKeyFromFile() {
    std::string iniPath ="Data/SKSE/Plugins/WebSocketSTT.ini";
    std::ifstream file(iniPath);
    if (!file.is_open()) {
        logger::error("Failed to open API key file: {}", iniPath);
        return "";
    }

    std::string line, apiKey;
    while (std::getline(file, line)) {
        if (line.find("API_KEY=") != std::string::npos) {
            apiKey = line.substr(line.find("=") + 1);
            break;
        }
    }

    file.close();
    if (apiKey.empty()) {
        logger::error("API key not found in file.");
        return "";
    }

    return apiKey;
}


bool OpenWebSocket() {
    ix::WebSocketHttpHeaders headers;
    std::string apiKey = ReadAPIKeyFromFile();

    if (apiKey.empty()) {
        logger::error("API key is empty.");
        return false;
    }
    headers["Sec-WebSocket-Protocol"] = "token, " + apiKey;
    
    std::string url = DEEPGRAM_WS_URL + "?encoding=linear16&sample_rate=" + std::to_string(SAMPLE_RATE) +
                      "&channels=" + std::to_string(NUM_CHANNELS)+"&model=nova-2&filler_words=true";

    websocket.setUrl(url);
    websocket.setExtraHeaders(headers);

    websocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            logger::info("Received message: {}", msg->str);
            // Process the transcript here.  This is where you get the text from Deepgram
            // msg->str will contain the JSON response.  You need to parse it to extract the transcript.
            // Example (using a JSON library like nlohmann::json):
            
            try {
                auto json = nlohmann::json::parse(msg->str);
                
                std::string type = json["type"];
                if (type == "Results") {
                    std::string transcript = json["channel"]["alternatives"][0]["transcript"];
                    bool isFinal = json["is_final"];
                    if (isFinal) {
                        logger::info("Transcript: {}", transcript);
                        message_to_send->add(transcript);
                        
                    }
                }
                } catch(const nlohmann::json::parse_error& e) {
                logger::error("JSON parse error: {}", e.what());
             }
            

        } else if (msg->type == ix::WebSocketMessageType::Open) {
            {
                std::lock_guard<std::mutex> lock(websocketMutex);
                websocketOpen = true;
            }
            logger::info("WebSocket opened");
            websocketCV.notify_one();
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            {
                std::lock_guard<std::mutex> lock(websocketMutex);
                //websocketOpen = false;
            }
            logger::info("WebSocket closed (code: {}, reason: {})", msg->closeInfo.code, msg->closeInfo.reason);
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            logger::error("WebSocket error: {}", msg->errorInfo.reason);
        }
    });

    websocket.start();
    return true;
}

static int recordCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
    AudioData* data = (AudioData*)userData;

    if (data->recording && websocketOpen) {
        const int16_t* buffer = (const int16_t*)inputBuffer;
        std::string audioData((const char*)buffer, framesPerBuffer * NUM_CHANNELS * sizeof(int16_t));
        websocket.sendBinary(audioData);
        return paContinue;
    }
    return paComplete;
}

void SendKeepAlive() {
    while (websocketOpen) {
        if (!VoiceRecordControl::getInstance().getRecording()) {
            nlohmann::json keepAliveMessage;
            keepAliveMessage["type"] = "KeepAlive";
            websocket.sendText(keepAliveMessage.dump());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void StopRecording() {
    nlohmann::json keepAliveMessage;
    keepAliveMessage["type"] = "Finalize";
    websocket.sendText(keepAliveMessage.dump());

    std::string eventType = "inputtext";
    

    int retries = 0;

    while (retries < 5) {
        std::string transcript = message_to_send->GetAndClearTranscript();
        if (transcript.empty()) {
            logger::info("NOT Sending empty user input: {}", transcript);
            retries++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

        } else {
            logger::info("Sending user input: {}", transcript);
            auto callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>();
            auto args = RE::MakeFunctionArguments(std::move(transcript), std::move(eventType));
            RE::BSScript::Internal::VirtualMachine::GetSingleton()->DispatchStaticCall("AIAgentFunctions",
                                                                                       "sendMessage", args, callback);
            retries = 6;
        }
    }
    
}


void StartRecording() {
    VoiceRecordControl::getInstance().setRecording(true);
    AudioData audioData;
    audioData.recording = true;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        logger::error("PortAudio error: {}", Pa_GetErrorText(err));
        return;
    }

    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
        logger::info("No default input device.");
        Pa_Terminate();
        return;
    }
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = SAMPLE_FORMAT;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaStream* stream;
    err = Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paNoFlag, recordCallback,
                        &audioData);
    if (err != paNoError) {
        logger::info("Failed to open audio stream: {}", Pa_GetErrorText(err));
        Pa_Terminate();
        return;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        logger::info("Failed to start audio stream: {}", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return;
    }

    while (VoiceRecordControl::getInstance().getRecording() && websocketOpen) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    audioData.recording = false;

    err = Pa_StopStream(stream);
    if (err != paNoError) {
        logger::info("Failed to stop audio stream: {}", Pa_GetErrorText(err));
    }
    Pa_CloseStream(stream);

    // Terminate PortAudio
    Pa_Terminate();
    StopRecording();
}


void InitializeWebSocket() {
    if (OpenWebSocket()) {
        std::unique_lock<std::mutex> lock(websocketMutex);
        websocketCV.wait(lock, [] { return websocketOpen; });
        logger::info("WebSocket ready");
        std::thread keepAliveThread(SendKeepAlive);
        keepAliveThread.detach();

        message_to_send = new Transcript();
    } else {
        logger::info("Could not open websocket, STT unavailable");
    }
}