/**
 * @file voice_client.hpp
 * @brief Xiaozhi-v3 WebSocket client for MeowKit (voice chat transport).
 *
 * M1 scope: connect to the backend's /xiaozhi WebSocket with the required auth
 * headers, send the client hello, and surface server messages (hello/stt/llm/
 * tts) to Lua as a drained event queue. Audio capture/Opus/streaming is M2.
 *
 * "Lua orchestrates, C++ streams": the WebSocket runs here; Lua drives it via
 * mk.voice.start()/poll()/stop(). For M1 the socket is pumped from poll()
 * (handshake traffic is light); M2 will move audio onto a dedicated task.
 */
#pragma once

#include <Arduino.h>
#include <deque>

struct VoiceEvent {
    String type;   ///< "connected","hello","stt","llm","tts_start","tts_stop","emotion","disconnected","error"
    String text;   ///< payload text where applicable (stt/llm transcript, error message)
};

class VoiceClient {
public:
    /// Begin an async connect to ws://host:port/path with Xiaozhi auth headers.
    void start(const String& host, uint16_t port, const String& path,
               const String& device_id, const String& token);

    /// Pump the WebSocket. Call frequently (mk.voice.poll does this).
    void loop();

    /// Disconnect and clear state.
    void stop();

    bool running() const { return _running; }

    /// Pop the oldest queued event; false if none.
    bool popEvent(VoiceEvent& out);

    /// Enqueue an event (used by the WS callback).
    void pushEvent(const String& type, const String& text = String());

private:
    String                 _headers;
    bool                   _running = false;
    std::deque<VoiceEvent> _events;
    static constexpr size_t kMaxEvents = 32;
};

extern VoiceClient g_voice;
