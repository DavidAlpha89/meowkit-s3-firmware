/**
 * @file mk_ws.cpp
 * @brief Generic WebSocket client for Lua apps (mk.ws) — implementation.
 */
#include "mk_ws.hpp"

#include <WebSocketsClient.h>

namespace {

WebSocketsClient _ws;
WsClient*        _self = nullptr;

void onWsEvent(WStype_t type, uint8_t* payload, size_t length)
{
    if (!_self) return;
    switch (type) {
    case WStype_CONNECTED:
        _self->_connected = true;
        _self->pushEvent("open");
        break;

    case WStype_TEXT:
        _self->pushEvent("text", String((const char*)payload, length));
        break;

    case WStype_BIN:
        _self->pushEvent("bin", String(), String((const char*)payload, length));
        break;

    case WStype_DISCONNECTED:
        _self->_connected = false;
        _self->pushEvent("close");
        break;

    case WStype_ERROR:
        _self->pushEvent("error", "websocket error");
        break;

    default:
        break;
    }
}

} // namespace

WsClient g_ws;

bool WsClient::start(const String& host, uint16_t port, const String& path,
                     const String& subprotocol, const String& headers)
{
    _self = this;
    _events.clear();
    _binBytes   = 0;
    _connected  = false;

    if (headers.length())     _ws.setExtraHeaders(headers.c_str());
    _ws.onEvent(onWsEvent);
    _ws.setReconnectInterval(0);   // reconnect policy belongs to the Lua app
    // WebSockets 2.7.x: the subprotocol is begin()'s 4th argument (default
    // "arduino" — must be overridden or the gadget server sees the wrong one).
    _ws.begin(host.c_str(), port, path.c_str(),
              subprotocol.length() ? subprotocol.c_str() : "arduino");
    _running = true;
    return true;
}

void WsClient::loop()
{
    if (_running) _ws.loop();
}

void WsClient::stop()
{
    _ws.disconnect();
    _running   = false;
    _connected = false;
    _events.clear();
    _binBytes = 0;
}

bool WsClient::sendText(const String& s)
{
    if (!_running) return false;
    return _ws.sendTXT((uint8_t*)s.c_str(), s.length()) > 0;
}

bool WsClient::sendBinary(const uint8_t* data, size_t len)
{
    if (!_running) return false;
    return _ws.sendBIN((uint8_t*)data, len) > 0;
}

void WsClient::pushEvent(const String& type, const String& text, const String& data)
{
    // Bound queue depth and total binary bytes so a chatty server can never
    // OOM the Lua VM; oldest events are sacrificed first.
    while (_events.size() >= kMaxEvents ||
           (_binBytes + data.length() > kMaxBinBytes && !_events.empty())) {
        _binBytes -= _events.front().data.length();
        _events.pop_front();
    }
    _events.push_back(WsEvent{ type, text, data });
    _binBytes += data.length();
}

bool WsClient::popEvent(WsEvent& out)
{
    if (_events.empty()) return false;
    out = _events.front();
    _binBytes -= out.data.length();
    _events.pop_front();
    return true;
}
