/**
 * @file mk_ws.hpp
 * @brief Generic WebSocket client for Lua apps (mk.ws).
 *
 * Unlike voice_client (which hard-codes the Xiaozhi-v3 handshake), this is a
 * plain transport: arbitrary path, subprotocol, extra headers, text and binary
 * frames. The Hermes gadget protocol (hermes-gadget.v1) is the primary user;
 * any JSON-over-WebSocket backend works.
 *
 * "Lua orchestrates, C++ pumps": the socket runs on the main loop via poll()
 * (driven by mk.ws.poll from the app runner), and events surface as a drained
 * queue. Binary frames arrive as raw Lua strings (arbitrary bytes).
 */
#pragma once

#include <Arduino.h>
#include <deque>

struct WsEvent {
    String type;   ///< "open","text","bin","close","error"
    String text;   ///< text payload (type=="text") or error message
    String data;   ///< raw binary payload (type=="bin")
};

class WsClient {
public:
    bool _connected = false;   ///< set from the static WS callback

    /// Begin an async connect. opts may be null/empty for defaults.
    /// headers: raw extra-header block "X: v\r\nY: v" (persisted by the lib).
    bool start(const String& host, uint16_t port, const String& path,
               const String& subprotocol, const String& headers);

    /// Pump the socket. Call frequently (mk.ws.poll does this).
    void loop();

    void stop();

    bool connected() const { return _connected; }

    bool sendText(const String& s);
    bool sendBinary(const uint8_t* data, size_t len);

    /// Pop the oldest queued event; false if none.
    bool popEvent(WsEvent& out);
    void pushEvent(const String& type, const String& text = String(),
                   const String& data = String());

private:
    bool _running   = false;
    std::deque<WsEvent> _events;
    static constexpr size_t kMaxEvents = 64;
    static constexpr size_t kMaxBinBytes = 256 * 1024;  ///< raw bytes held for bin events
    size_t _binBytes = 0;
};

extern WsClient g_ws;
