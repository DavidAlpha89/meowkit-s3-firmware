/**
 * @file voice_client.cpp
 * @brief Xiaozhi-v3 WebSocket client — implementation (M1 handshake).
 */
#include "voice_client.hpp"

#include <WebSocketsClient.h>
#include <ArduinoJson.h>

namespace {

WebSocketsClient _ws;
VoiceClient*     g_self = nullptr;

// Client hello — audio_params must match the server's _valid_client_hello
// EXACTLY (Opus / 16 kHz / mono / 60 ms for the device→server upload path).
const char* kHelloJson =
    "{\"type\":\"hello\",\"version\":3,\"transport\":\"websocket\","
    "\"audio_params\":{\"format\":\"opus\",\"sample_rate\":16000,"
    "\"channels\":1,\"frame_duration\":60}}";

void onWsEvent(WStype_t type, uint8_t* payload, size_t length)
{
    if (!g_self) return;
    switch (type) {
    case WStype_CONNECTED:
        _ws.sendTXT(kHelloJson);
        g_self->pushEvent("connected");
        break;

    case WStype_TEXT: {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, (const char*)payload, length);
        if (err) { g_self->pushEvent("error", "bad json from server"); break; }
        String t = (const char*)(doc["type"] | "");
        if (t == "stt" || t == "llm") {
            g_self->pushEvent(t, (const char*)(doc["text"] | ""));
        } else if (t == "tts") {
            g_self->pushEvent(String("tts_") + (const char*)(doc["state"] | ""));
        } else {
            g_self->pushEvent(t.length() ? t : String("text"));
        }
        break;
    }

    case WStype_BIN:
        // M2: V3 Opus audio frames (TTS down). Ignored during M1.
        break;

    case WStype_DISCONNECTED:
        g_self->pushEvent("disconnected");
        break;

    case WStype_ERROR:
        g_self->pushEvent("error", "websocket error");
        break;

    default:
        break;
    }
}

} // namespace

VoiceClient g_voice;

void VoiceClient::start(const String& host, uint16_t port, const String& path,
                        const String& device_id, const String& token)
{
    g_self = this;
    _events.clear();

    // Persist the header block (library reads it on connect).
    _headers = "Authorization: Bearer " + token +
               "\r\nDevice-Id: " + device_id +
               "\r\nProtocol-Version: 3";
    _ws.setExtraHeaders(_headers.c_str());
    _ws.onEvent(onWsEvent);
    _ws.setReconnectInterval(5000);
    _ws.begin(host.c_str(), port, path.c_str());
    _running = true;
}

void VoiceClient::loop()
{
    if (_running) _ws.loop();
}

void VoiceClient::stop()
{
    _running = false;
    _ws.disconnect();
    _events.clear();
}

bool VoiceClient::popEvent(VoiceEvent& out)
{
    if (_events.empty()) return false;
    out = _events.front();
    _events.pop_front();
    return true;
}

void VoiceClient::pushEvent(const String& type, const String& text)
{
    if (_events.size() >= kMaxEvents) _events.pop_front();
    _events.push_back(VoiceEvent{type, text});
}
