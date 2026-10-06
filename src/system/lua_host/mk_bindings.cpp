/**
 * @file mk_bindings.cpp
 * @brief The `mk.*` Lua API — implementation.
 */
#include "mk_bindings.hpp"

#include <Arduino.h>
#include <SD_MMC.h>
#include <cstring>
#include "../../bsp/devices.h"

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

void registerSub(lua_State* L, const char* name, const luaL_Reg* funcs)
{
    lua_newtable(L);
    luaL_setfuncs(L, funcs, 0);
    lua_setfield(L, -2, name); // mk[name] = subtable  (mk is at -2)
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

    registerSub(L, "display", disp);
    registerSub(L, "input",   input);
    registerSub(L, "fs",      fs);
    registerSub(L, "time",    tmr);
    registerSub(L, "sys",     sys);

    lua_pushcfunction(L, l_color);
    lua_setfield(L, -2, "color");

    lua_setglobal(L, "mk"); // _G.mk = mk
}

bool exitRequested() { return g_exit; }
void clearExit()     { g_exit = false; }

} // namespace mk_bindings
