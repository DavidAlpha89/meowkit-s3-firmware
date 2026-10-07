-- voicetest / main.lua — M1: Xiaozhi-v3 WebSocket handshake test.
--
-- Needs /apps/voicetest/config.lua (copy config.example.lua, fill in token).
-- Press A to connect: the device opens the WS with the auth headers, sends the
-- client hello, and shows the server's replies. A server "hello" = handshake OK.
-- short B = back, hold B = exit.

local cfg     = nil
local status  = "idle"
local lines   = {}
local started = false
local last_a  = false

local function log(s)
  lines[#lines + 1] = s
  while #lines > 8 do table.remove(lines, 1) end
end

local function load_cfg()
  local raw = mk.fs.read("/apps/voicetest/config.lua")
  if not raw then return nil, "no config.lua (copy config.example.lua)" end
  local f = load(raw)
  if not f then return nil, "config.lua parse error" end
  local ok, t = pcall(f)
  if not ok or type(t) ~= "table" then return nil, "config.lua must return a table" end
  if not (t.url and t.device_id and t.token) then return nil, "config missing url/device_id/token" end
  return t
end

local function draw()
  mk.display.clear(0x000000)
  mk.display.text(10, 6, "Voice Test (M1)", 0xBBE700, 2)
  local net = mk.wifi.connected() and mk.wifi.ip() or "wifi down"
  mk.display.text(10, 36, net .. "   " .. status, 0xFFFFFF, 1)
  for i, l in ipairs(lines) do
    mk.display.text(10, 56 + (i - 1) * 16, l:sub(1, 50), 0xCCCCCC, 1)
  end
  mk.display.text(10, 222, "A: connect   short B: back", 0x888888, 1)
end

function on_open()
  local c, err = load_cfg()
  cfg = c
  status = cfg and "ready - press A" or err
  draw()
end

function on_running(dt)
  -- A (edge) to connect
  local a = mk.input.down("a")
  if a and not last_a and cfg and not started then
    if mk.wifi.connected() then
      local ok, err = mk.voice.start(cfg.url, cfg.device_id, cfg.token)
      status  = ok and "connecting..." or ("err: " .. tostring(err))
      started = ok and true or false
    else
      status = "wifi down"
    end
    draw()
  end
  last_a = a

  -- drain server events
  local ev = mk.voice.poll()
  while ev do
    local t = ev.type
    if t == "connected" then
      status = "connected"; log("WS up; hello sent")
    elseif t == "hello" then
      log("server hello OK")
    elseif t == "stt" then
      log("stt: " .. (ev.text or ""))
    elseif t == "llm" then
      log("llm: " .. (ev.text or ""))
    elseif t == "disconnected" then
      status = "disconnected"; started = false; log("disconnected")
    elseif t == "error" then
      log("error: " .. (ev.text or ""))
    else
      log("evt: " .. t)
    end
    draw()
    ev = mk.voice.poll()
  end
end

function on_close()
  mk.voice.stop()
end
