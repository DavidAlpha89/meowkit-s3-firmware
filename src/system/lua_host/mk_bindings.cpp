/**
 * @file mk_bindings.cpp
 * @brief The `mk.*` Lua API — implementation.
 */
#include "mk_bindings.hpp"

#include <Arduino.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <cstring>
#include "../../bsp/devices.h"
#include "../voice/voice_client.hpp"
#include "mk_ws.hpp"
#include <ArduinoJson.h>
#include <esp_system.h>
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

namespace {

// One running script at a time → a process-global device context is fine.
DEVICES* g_dev  = nullptr;
bool     g_exit = false;

// ── helpers ──────────────────────────────────────────────────────────

// LovyanGFX interprets a uint32_t color argument as RGB888 and converts to the
// panel's native format, so scripts can pass plain 0xRRGGBB integers.
inline uint32_t argColor(lua_State* L, int idx, uint32_t def)
{
    return (uint32_t)luaL_optinteger(L, idx, (lua_Integer)def);
}

Button_Class* buttonByName(const char* n)
{
    if (!n || !g_dev) return nullptr;
    if (!strcmp(n, "a"))     return &g_dev->button.A;
    if (!strcmp(n, "b"))     return &g_dev->button.B;
    if (!strcmp(n, "up"))    return &g_dev->button.Up;
    if (!strcmp(n, "down"))  return &g_dev->button.Down;
    if (!strcmp(n, "left"))  return &g_dev->button.Left;
    if (!strcmp(n, "right")) return &g_dev->button.Right;
    return nullptr;
}

// Only absolute SD paths, never any ".." traversal.
bool safePath(const char* p, String& out)
{
    if (!p || p[0] != '/') return false;
    if (strstr(p, "..") != nullptr) return false;
    out = String(p);
    return true;
}

const char* baseName(const char* path)
{
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

// ── mk.display ───────────────────────────────────────────────────────

int l_disp_clear(lua_State* L)
{
    g_dev->Lcd.fillScreen(argColor(L, 1, 0x000000));
    return 0;
}

int l_disp_text(lua_State* L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    const char* s = luaL_checkstring(L, 3);
    uint32_t c = argColor(L, 4, 0xFFFFFF);
    int size = (int)luaL_optinteger(L, 5, 1);
    g_dev->Lcd.setTextColor(c);
    g_dev->Lcd.setTextSize(size < 1 ? 1 : size);
    g_dev->Lcd.drawString(s, x, y);
    return 0;
}

int l_disp_pixel(lua_State* L)
{
    g_dev->Lcd.drawPixel((int)luaL_checkinteger(L, 1),
                         (int)luaL_checkinteger(L, 2),
                         argColor(L, 3, 0xFFFFFF));
    return 0;
}

int l_disp_line(lua_State* L)
{
    g_dev->Lcd.drawLine((int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
                        (int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4),
                        argColor(L, 5, 0xFFFFFF));
    return 0;
}

int l_disp_rect(lua_State* L)
{
    int x = (int)luaL_checkinteger(L, 1), y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3), h = (int)luaL_checkinteger(L, 4);
    uint32_t c = argColor(L, 5, 0xFFFFFF);
    bool filled = lua_isnoneornil(L, 6) ? true : lua_toboolean(L, 6);
    if (filled) g_dev->Lcd.fillRect(x, y, w, h, c);
    else        g_dev->Lcd.drawRect(x, y, w, h, c);
    return 0;
}

int l_disp_circle(lua_State* L)
{
    int x = (int)luaL_checkinteger(L, 1), y = (int)luaL_checkinteger(L, 2);
    int r = (int)luaL_checkinteger(L, 3);
    uint32_t c = argColor(L, 4, 0xFFFFFF);
    bool filled = lua_isnoneornil(L, 5) ? true : lua_toboolean(L, 5);
    if (filled) g_dev->Lcd.fillCircle(x, y, r, c);
    else        g_dev->Lcd.drawCircle(x, y, r, c);
    return 0;
}

int l_disp_width(lua_State* L)  { lua_pushinteger(L, g_dev->Lcd.width());  return 1; }
int l_disp_height(lua_State* L) { lua_pushinteger(L, g_dev->Lcd.height()); return 1; }

// ── mk.input ─────────────────────────────────────────────────────────

int l_input_down(lua_State* L)
{
    Button_Class* b = buttonByName(luaL_checkstring(L, 1));
    lua_pushboolean(L, b && b->state() == Button_Class::PRESSED);
    return 1;
}

// ── mk.fs (sandboxed to SD) ──────────────────────────────────────────

int l_fs_read(lua_State* L)
{
    String sp;
    if (!safePath(luaL_checkstring(L, 1), sp)) { lua_pushnil(L); return 1; }
    File f = SD_MMC.open(sp.c_str(), FILE_READ);
    if (!f || f.isDirectory()) { if (f) f.close(); lua_pushnil(L); return 1; }
    size_t n = f.size();
    luaL_Buffer b;
    char* buf = luaL_buffinitsize(L, &b, n);
    size_t rd = f.read((uint8_t*)buf, n);
    f.close();
    luaL_pushresultsize(&b, rd);
    return 1;
}

int l_fs_write(lua_State* L)
{
    size_t n = 0;
    String sp;
    const char* data = luaL_checklstring(L, 2, &n);
    if (!safePath(luaL_checkstring(L, 1), sp)) { lua_pushboolean(L, 0); return 1; }
    File f = SD_MMC.open(sp.c_str(), FILE_WRITE);
    if (!f) { lua_pushboolean(L, 0); return 1; }
    size_t w = f.write((const uint8_t*)data, n);
    f.close();
    lua_pushboolean(L, w == n);
    return 1;
}

int l_fs_list(lua_State* L)
{
    String sp;
    lua_newtable(L);
    if (!safePath(luaL_checkstring(L, 1), sp)) return 1;
    File d = SD_MMC.open(sp.c_str());
    if (!d || !d.isDirectory()) { if (d) d.close(); return 1; }
    int i = 1;
    for (File e = d.openNextFile(); e; e = d.openNextFile()) {
        lua_pushstring(L, baseName(e.name()));
        lua_rawseti(L, -2, i++);
        e.close();
    }
    d.close();
    return 1;
}

int l_fs_exists(lua_State* L)
{
    String sp;
    lua_pushboolean(L, safePath(luaL_checkstring(L, 1), sp) && SD_MMC.exists(sp.c_str()));
    return 1;
}

// ── mk.time ──────────────────────────────────────────────────────────

int l_time_ms(lua_State* L) { lua_pushinteger(L, (lua_Integer)millis()); return 1; }

int l_time_delay(lua_State* L)
{
    long ms = (long)luaL_checkinteger(L, 1);
    if (ms < 0)    ms = 0;
    if (ms > 1000) ms = 1000; // cap so a script can't trip the watchdog
    delay(ms);
    return 0;
}

// ── mk.sys ───────────────────────────────────────────────────────────

int l_sys_log(lua_State* L)   { Serial.printf("[lua] %s\n", luaL_checkstring(L, 1)); return 0; }
int l_sys_exit(lua_State* L)  { (void)L; g_exit = true; return 0; }
int l_sys_heap(lua_State* L)  { lua_pushinteger(L, (lua_Integer)ESP.getFreeHeap());  return 1; }
int l_sys_psram(lua_State* L) { lua_pushinteger(L, (lua_Integer)ESP.getFreePsram()); return 1; }

// ── mk.color ─────────────────────────────────────────────────────────

int l_color(lua_State* L)
{
    long r = (long)luaL_checkinteger(L, 1) & 0xFF;
    long g = (long)luaL_checkinteger(L, 2) & 0xFF;
    long b = (long)luaL_checkinteger(L, 3) & 0xFF;
    lua_pushinteger(L, (lua_Integer)((r << 16) | (g << 8) | b));
    return 1;
}

// ── mk.wifi ──────────────────────────────────────────────────────────

int l_wifi_connected(lua_State* L) { lua_pushboolean(L, g_dev->wifi.isConnected());        return 1; }
int l_wifi_ip(lua_State* L)        { lua_pushstring(L, g_dev->wifi.getIP().c_str());        return 1; }
int l_wifi_ssid(lua_State* L)      { lua_pushstring(L, g_dev->wifi.getSSID().c_str());      return 1; }
int l_wifi_rssi(lua_State* L)      { lua_pushinteger(L, (lua_Integer)WiFi.RSSI());          return 1; }

int l_wifi_connect(lua_State* L)
{
    const char* ssid = luaL_checkstring(L, 1);
    const char* pass = luaL_optstring(L, 2, "");
    // Non-blocking: starts the attempt; poll mk.wifi.connected() for the result.
    bool started = g_dev->wifi.connect(ssid, pass, (int)luaL_optinteger(L, 3, 15000));
    lua_pushboolean(L, started);
    return 1;
}

// ── mk.http ──────────────────────────────────────────────────────────

// Apply an optional {name=value, ...} header table at stack index `idx`.
void applyHeaders(lua_State* L, int idx, HTTPClient& http)
{
    if (!lua_istable(L, idx)) return;
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING && lua_isstring(L, -1)) {
            http.addHeader(lua_tostring(L, -2), lua_tostring(L, -1));
        }
        lua_pop(L, 1); // pop value, keep key for lua_next
    }
}

// Shared GET/POST. Blocking (fine for the cooperative model on a button press).
// Returns (status:int, body:string) on success, or (nil, err:string).
int httpRequest(lua_State* L, bool isPost)
{
    const char* url = luaL_checkstring(L, 1);
    if (!g_dev->wifi.isConnected()) {
        lua_pushnil(L); lua_pushstring(L, "wifi not connected"); return 2;
    }

    const bool https = (strncmp(url, "https", 5) == 0);
    WiFiClientSecure sclient;
    WiFiClient       client;
    HTTPClient       http;
    http.setConnectTimeout(8000);
    http.setTimeout(8000);

    bool ok;
    if (https) {
        sclient.setInsecure();           // dev default: accept any/self-signed cert
        ok = http.begin(sclient, url);
    } else {
        ok = http.begin(client, url);
    }
    if (!ok) { lua_pushnil(L); lua_pushstring(L, "http.begin failed (bad url?)"); return 2; }

    int code;
    if (isPost) {
        size_t blen = 0;
        const char* body  = luaL_optlstring(L, 2, "", &blen);
        const char* ctype = luaL_optstring(L, 3, "application/json");
        http.addHeader("Content-Type", ctype);
        applyHeaders(L, 4, http);
        code = http.POST((uint8_t*)body, blen);
    } else {
        applyHeaders(L, 2, http);
        code = http.GET();
    }

    if (code <= 0) {
        String err = HTTPClient::errorToString(code);
        http.end();
        lua_pushnil(L); lua_pushstring(L, err.c_str()); return 2;
    }

    String payload = http.getString();
    http.end();
    lua_pushinteger(L, code);
    lua_pushlstring(L, payload.c_str(), payload.length());
    return 2;
}

int l_http_get(lua_State* L)  { return httpRequest(L, false); }
int l_http_post(lua_State* L) { return httpRequest(L, true);  }

// ── mk.audio (speaker: ES8311 DAC → NS4150B amp) ─────────────────────
// Sine tones are the worst case for the 1 W speaker, so tone output is clamped
// under the firmware's own SPK_VOLUME_MAX ceiling.
constexpr int kToneVolMax  = 50;
constexpr int kAmpDrainMs  = 60;   // let I2S DMA empty before cutting the amp

void audio_ensure()
{
    if (!g_dev->speaker.isEnabled()) g_dev->speaker.begin();
}

// Power the NS4150B only around playback, mute-gated to avoid pops, with the
// same settle delays as the (audible) system_sound.cpp amp() sequence.
void audio_amp(bool on)
{
    if (on) {
        g_dev->speaker.setMute(true);
        g_dev->io_exp.digitalWrite(HAL_IOEXP_PA_EN, HIGH);
        delay(12);                       // NS4150B turn-on settle
        g_dev->speaker.setMute(false);
    } else {
        g_dev->speaker.setMute(true);
        delay(4);
        g_dev->io_exp.digitalWrite(HAL_IOEXP_PA_EN, LOW);
    }
}

int l_audio_tone(lua_State* L)
{
    uint32_t f  = (uint32_t)luaL_checkinteger(L, 1);
    uint32_t ms = (uint32_t)luaL_checkinteger(L, 2);
    int vol     = (int)luaL_optinteger(L, 3, 30);
    if (ms > 5000) ms = 5000;              // cap duration
    if (vol < 0) vol = 0;
    if (vol > kToneVolMax) vol = kToneVolMax;
    audio_ensure();
    Serial.printf("[mk.audio] tone f=%u ms=%u vol=%d spk_enabled=%d\n",
                  (unsigned)f, (unsigned)ms, vol, (int)g_dev->speaker.isEnabled());
    audio_amp(true);
    g_dev->speaker.tone(f, ms, vol);       // blocking (writes into I2S DMA)
    delay(kAmpDrainMs);                    // drain DMA before the amp cuts
    audio_amp(false);
    return 0;
}

int l_audio_beep(lua_State* L)
{
    (void)L;
    audio_ensure();
    audio_amp(true);
    g_dev->speaker.tone(880, 120, kToneVolMax);
    delay(kAmpDrainMs);
    audio_amp(false);
    return 0;
}

int l_audio_volume(lua_State* L)
{
    int v = (int)luaL_checkinteger(L, 1);
    if (v < 0) v = 0;
    if (v > SPK_VOLUME_MAX) v = SPK_VOLUME_MAX;  // firmware's enforced ceiling
    audio_ensure();
    lua_pushboolean(L, g_dev->speaker.setVolume(v));
    return 1;
}

// ── mk.voice (Xiaozhi-v3 WebSocket) ──────────────────────────────────

int l_voice_start(lua_State* L)
{
    String url = luaL_checkstring(L, 1);
    const char* device_id = luaL_checkstring(L, 2);
    const char* token     = luaL_checkstring(L, 3);

    if (!g_dev->wifi.isConnected()) {
        lua_pushboolean(L, 0); lua_pushstring(L, "wifi not connected"); return 2;
    }

    // Parse ws://host[:port]/path (TLS/wss not needed for the local backend).
    int scheme = url.indexOf("://");
    if (scheme >= 0) url = url.substring(scheme + 3);
    int slash = url.indexOf('/');
    String hostport = (slash >= 0) ? url.substring(0, slash) : url;
    String path     = (slash >= 0) ? url.substring(slash)    : String("/");
    uint16_t port = 80;
    String host = hostport;
    int colon = hostport.indexOf(':');
    if (colon >= 0) {
        host = hostport.substring(0, colon);
        port = (uint16_t)hostport.substring(colon + 1).toInt();
    }
    if (host.length() == 0 || port == 0) {
        lua_pushboolean(L, 0); lua_pushstring(L, "bad url"); return 2;
    }

    g_voice.start(host, port, path, String(device_id), String(token));
    lua_pushboolean(L, 1);
    return 1;
}

int l_voice_poll(lua_State* L)
{
    g_voice.loop();
    VoiceEvent ev;
    if (!g_voice.popEvent(ev)) { lua_pushnil(L); return 1; }
    lua_newtable(L);
    lua_pushstring(L, ev.type.c_str()); lua_setfield(L, -2, "type");
    if (ev.text.length()) { lua_pushstring(L, ev.text.c_str()); lua_setfield(L, -2, "text"); }
    return 1;
}

int l_voice_stop(lua_State* L) { (void)L; g_voice.stop(); return 0; }

void registerSub(lua_State* L, const char* name, const luaL_Reg* funcs)
{
    lua_newtable(L);
    luaL_setfuncs(L, funcs, 0);
    lua_setfield(L, -2, name); // mk[name] = subtable  (mk is at -2)
}

// ── mk.ws (generic WebSocket transport) ──────────────────────────────

int l_ws_connect(lua_State* L)
{
    const char* host = luaL_checkstring(L, 1);
    int         port = (int)luaL_checkinteger(L, 2);
    const char* path = luaL_optstring(L, 3, "/");
    const char* sub  = nullptr;
    const char* hdrs = nullptr;
    if (lua_istable(L, 4)) {
        lua_getfield(L, 4, "subprotocol");
        if (lua_isstring(L, -1)) sub = lua_tostring(L, -1);
        lua_getfield(L, 4, "headers");
        if (lua_isstring(L, -1)) hdrs = lua_tostring(L, -1);
        lua_pop(L, 2);
    }
    lua_pushboolean(L, g_ws.start(host, (uint16_t)port, path,
                                  sub ? sub : "", hdrs ? hdrs : ""));
    return 1;
}

int l_ws_poll(lua_State* L)
{
    g_ws.loop();                       // pump the socket once per call
    WsEvent ev;
    if (!g_ws.popEvent(ev)) { lua_pushnil(L); return 1; }
    lua_createtable(L, 0, 3);
    lua_pushstring(L, ev.type.c_str());  lua_setfield(L, -2, "type");
    if (ev.text.length()) { lua_pushlstring(L, ev.text.c_str(), ev.text.length()); lua_setfield(L, -2, "text"); }
    if (ev.data.length()) { lua_pushlstring(L, ev.data.c_str(), ev.data.length()); lua_setfield(L, -2, "data"); }
    return 1;
}

int l_ws_send(lua_State* L)
{
    size_t len; const char* s = luaL_checklstring(L, 1, &len);
    lua_pushboolean(L, g_ws.sendText(String(s, len)));
    return 1;
}

int l_ws_send_bin(lua_State* L)
{
    size_t len; const char* s = luaL_checklstring(L, 1, &len);
    lua_pushboolean(L, g_ws.sendBinary((const uint8_t*)s, len));
    return 1;
}

int l_ws_connected(lua_State* L) { lua_pushboolean(L, g_ws.connected()); return 1; }
int l_ws_close(lua_State* L)     { g_ws.stop(); return 0; }

// ── mk.crypto (mbedtls: gadget handshake needs sha256/HMAC/base64) ───

int l_sha256_hex(lua_State* L)
{
    size_t len; const char* s = luaL_checklstring(L, 1, &len);
    unsigned char out[32];
    mbedtls_sha256((const unsigned char*)s, len, out, 0);
    static const char* hex = "0123456789abcdef";
    char str[65];
    for (int i = 0; i < 32; i++) { str[i*2] = hex[out[i] >> 4]; str[i*2+1] = hex[out[i] & 0xF]; }
    str[64] = '\0';
    lua_pushlstring(L, str, 64);
    return 1;
}

int l_hmac_sha256(lua_State* L)
{
    size_t klen, mlen;
    const char* key = luaL_checklstring(L, 1, &klen);
    const char* msg = luaL_checklstring(L, 2, &mlen);
    unsigned char out[32];
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    int rc = mbedtls_md_hmac(info, (const unsigned char*)key, klen,
                             (const unsigned char*)msg, mlen, out);
    if (rc != 0) { lua_pushnil(L); lua_pushinteger(L, rc); return 2; }
    lua_pushlstring(L, (const char*)out, 32);
    return 1;
}

int l_b64_encode(lua_State* L)
{
    size_t len; const char* s = luaL_checklstring(L, 1, &len);
    size_t olen = 0;
    mbedtls_base64_encode(nullptr, 0, &olen, (const unsigned char*)s, len);
    std::string out(olen, '\0');
    if (mbedtls_base64_encode((unsigned char*)&out[0], out.size(), &olen,
                              (const unsigned char*)s, len) != 0) {
        lua_pushnil(L); return 1;
    }
    out.resize(olen);
    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int l_b64_decode(lua_State* L)
{
    size_t len; const char* s = luaL_checklstring(L, 1, &len);
    std::string out(len, '\0');
    size_t olen = 0;
    if (mbedtls_base64_decode((unsigned char*)&out[0], out.size(), &olen,
                              (const unsigned char*)s, len) != 0) {
        lua_pushnil(L); return 1;
    }
    lua_pushlstring(L, out.data(), olen);
    return 1;
}

int l_random(lua_State* L)
{
    int n = (int)luaL_checkinteger(L, 1);
    if (n < 0) n = 0;
    if (n > 512) n = 512;
    std::string out(n, '\0');
    esp_fill_random(&out[0], n);
    lua_pushlstring(L, out.data(), n);
    return 1;
}

// ── mk.json (ArduinoJson bridge; binary-safe strings) ────────────────

static void pushJsonValue(lua_State* L, JsonVariantConst v)
{
    if (v.isNull())      { lua_pushnil(L); return; }
    if (v.is<bool>())    { lua_pushboolean(L, v.as<bool>()); return; }
    if (v.is<int64_t>()) { lua_pushinteger(L, (lua_Integer)v.as<int64_t>()); return; }
    if (v.is<float>() || v.is<double>()) { lua_pushnumber(L, v.as<double>()); return; }
    if (v.is<const char*>()) {
        String s = v.as<String>();
        lua_pushlstring(L, s.c_str(), s.length());
        return;
    }
    if (v.is<JsonArrayConst>()) {
        JsonArrayConst arr = v.as<JsonArrayConst>();
        lua_createtable(L, arr.size(), 0);
        int i = 1;
        for (JsonVariantConst e : arr) { pushJsonValue(L, e); lua_rawseti(L, -2, i++); }
        return;
    }
    if (v.is<JsonObjectConst>()) {
        JsonObjectConst obj = v.as<JsonObjectConst>();
        lua_createtable(L, 0, obj.size());
        for (auto kv : obj) {
            lua_pushlstring(L, kv.key().c_str(), kv.key().size());
            pushJsonValue(L, kv.value());
            lua_settable(L, -3);
        }
        return;
    }
    lua_pushnil(L);
}

int l_json_decode(lua_State* L)
{
    size_t len; const char* s = luaL_checklstring(L, 1, &len);
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, s, len);
    if (err) { lua_pushnil(L); lua_pushstring(L, err.c_str()); return 2; }
    pushJsonValue(L, doc.as<JsonVariantConst>());
    return 1;
}

static bool luaToJson(lua_State* L, int idx, JsonVariant out, int depth)
{
    if (depth > 32) return false;
    switch (lua_type(L, idx)) {
    case LUA_TNIL: case LUA_TNONE:     out.clear(); return true;
    case LUA_TBOOLEAN:                out.set(lua_toboolean(L, idx) != 0); return true;
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx)) out.set((int64_t)lua_tointeger(L, idx));
        else                       out.set((double)lua_tonumber(L, idx));
        return true;
    case LUA_TSTRING: {
        size_t len; const char* s = lua_tolstring(L, idx, &len);
        out.set(String(s, len));
        return true;
    }
    case LUA_TTABLE: {
        // Array iff keys are exactly 1..#t with no hash part.
        int n = (int)lua_rawlen(L, idx);
        bool isArray = n > 0;
        for (int i = 1; i <= n && isArray; i++) {
            lua_rawgeti(L, idx, i);
            isArray = lua_type(L, -1) != LUA_TNIL;
            lua_pop(L, 1);
        }
        if (isArray) {
            JsonArray arr = out.to<JsonArray>();
            for (int i = 1; i <= n; i++) {
                lua_rawgeti(L, idx, i);
                if (!luaToJson(L, lua_gettop(L), arr.add<JsonVariant>(), depth + 1)) { lua_pop(L, 1); return false; }
                lua_pop(L, 1);
            }
            return true;
        }
        JsonObject obj = out.to<JsonObject>();
        lua_pushnil(L);                       // first key
        while (lua_next(L, idx) != 0) {
            if (lua_type(L, -2) != LUA_TSTRING) { lua_pop(L, 2); return false; }
            size_t klen; const char* k = lua_tolstring(L, -2, &klen);
            if (!luaToJson(L, lua_gettop(L), obj[k], depth + 1)) { lua_pop(L, 2); return false; }
            lua_pop(L, 1);                    // drop value, keep key for next iteration
        }
        return true;
    }
    default:
        return false;                         // functions/userdata never serialize
    }
}

int l_json_encode(lua_State* L)
{
    luaL_checkany(L, 1);
    JsonDocument doc;
    if (!luaToJson(L, 1, doc.to<JsonVariant>(), 0)) {
        lua_pushnil(L); lua_pushstring(L, "value not JSON-serializable"); return 2;
    }
    std::string out;
    serializeJson(doc, out);
    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

// ── mk.audio PCM streaming (gadget TTS: hold the amp across a stream) ──

bool     s_audio_streaming = false;
uint32_t s_audio_rate      = 16000;

int l_audio_stream_start(lua_State* L)
{
    s_audio_rate = (uint32_t)luaL_optinteger(L, 1, 16000);
    audio_ensure();
    audio_amp(true);
    s_audio_streaming = true;
    return 0;
}

int l_audio_stream_write(lua_State* L)
{
    size_t len; const char* s = luaL_checklstring(L, 1, &len);
    if (!s_audio_streaming) { lua_pushboolean(L, false); return 1; }
    if (len > 256 * 1024) len = 256 * 1024;      // sanity cap per chunk
    if (len >= 2)
        g_dev->speaker.playPcm((const int16_t*)s, len / 2, 1, s_audio_rate);
    lua_pushboolean(L, true);
    return 1;
}

int l_audio_stream_stop(lua_State* L)
{
    if (!s_audio_streaming) return 0;
    delay(kAmpDrainMs);                          // drain DMA before the amp cuts
    audio_amp(false);
    s_audio_streaming = false;
    return 0;
}

} // namespace

namespace mk_bindings {

void install(lua_State* L, DEVICES* device)
{
    g_dev  = device;
    g_exit = false;

    lua_newtable(L); // mk

    static const luaL_Reg disp[] = {
        { "clear", l_disp_clear }, { "text", l_disp_text }, { "pixel", l_disp_pixel },
        { "line", l_disp_line },   { "rect", l_disp_rect }, { "circle", l_disp_circle },
        { "width", l_disp_width }, { "height", l_disp_height }, { nullptr, nullptr }
    };
    static const luaL_Reg input[] = {
        { "down", l_input_down }, { nullptr, nullptr }
    };
    static const luaL_Reg fs[] = {
        { "read", l_fs_read }, { "write", l_fs_write }, { "list", l_fs_list },
        { "exists", l_fs_exists }, { nullptr, nullptr }
    };
    static const luaL_Reg tmr[] = {
        { "ms", l_time_ms }, { "delay", l_time_delay }, { nullptr, nullptr }
    };
    static const luaL_Reg sys[] = {
        { "log", l_sys_log }, { "exit", l_sys_exit }, { "heap", l_sys_heap },
        { "psram", l_sys_psram }, { nullptr, nullptr }
    };
    static const luaL_Reg wifi[] = {
        { "connected", l_wifi_connected }, { "ip", l_wifi_ip }, { "ssid", l_wifi_ssid },
        { "rssi", l_wifi_rssi }, { "connect", l_wifi_connect }, { nullptr, nullptr }
    };
    static const luaL_Reg http[] = {
        { "get", l_http_get }, { "post", l_http_post }, { nullptr, nullptr }
    };
    static const luaL_Reg audio[] = {
        { "tone", l_audio_tone }, { "beep", l_audio_beep }, { "volume", l_audio_volume },
        { "stream_start", l_audio_stream_start }, { "stream_write", l_audio_stream_write },
        { "stream_stop", l_audio_stream_stop },
        { nullptr, nullptr }
    };
    static const luaL_Reg voice[] = {
        { "start", l_voice_start }, { "poll", l_voice_poll }, { "stop", l_voice_stop },
        { nullptr, nullptr }
    };
    static const luaL_Reg ws[] = {
        { "connect", l_ws_connect }, { "poll", l_ws_poll }, { "send", l_ws_send },
        { "send_bin", l_ws_send_bin }, { "connected", l_ws_connected }, { "close", l_ws_close },
        { nullptr, nullptr }
    };
    static const luaL_Reg crypto[] = {
        { "sha256_hex", l_sha256_hex }, { "hmac_sha256", l_hmac_sha256 },
        { "b64_encode", l_b64_encode }, { "b64_decode", l_b64_decode },
        { "random", l_random },
        { nullptr, nullptr }
    };
    static const luaL_Reg json[] = {
        { "decode", l_json_decode }, { "encode", l_json_encode },
        { nullptr, nullptr }
    };

    registerSub(L, "display", disp);
    registerSub(L, "input",   input);
    registerSub(L, "fs",      fs);
    registerSub(L, "time",    tmr);
    registerSub(L, "sys",     sys);
    registerSub(L, "wifi",    wifi);
    registerSub(L, "http",    http);
    registerSub(L, "audio",   audio);
    registerSub(L, "voice",   voice);
    registerSub(L, "ws",      ws);
    registerSub(L, "crypto",  crypto);
    registerSub(L, "json",    json);

    lua_pushcfunction(L, l_color);
    lua_setfield(L, -2, "color");

    lua_setglobal(L, "mk"); // _G.mk = mk
}

bool exitRequested() { return g_exit; }
void clearExit()     { g_exit = false; }

} // namespace mk_bindings
