-- gadget / main.lua — Hermes Gadget client (protocol v1).
--
-- Speaks hermes-gadget.v1 over mk.ws: TOFU key pair, HMAC auth, pairing code,
-- conversation lines, TTS playback through the speaker, yes/no prompts,
-- display cards, dashboard, and a `beep` device action. Mic uplink is a later
-- milestone (no capture binding yet), so `mic` is not advertised.
--
-- Needs /apps/gadget/config.lua (copy config.example.lua).
-- Buttons: A answers prompt=yes · Left answers prompt=no · Up toggles dashboard.
-- (B stays the runner's back/exit key, so it deliberately does NOT answer.)

local cfg, cfg_err
local key, device_id
local state = "boot"          -- boot|wifi|connecting|handshake|pairing|ready|offline
local session, hb_s, paired = nil, 20, false
local last_rx = 0
local last_try = 0
local lines = {}              -- {text, color}
local card = nil              -- {title, body, until}
local notice = nil            -- {text, until}
local status_line = ""
local prompt_q = nil          -- {id, title, text, shown_at}
local dash = nil              -- last dashboard snapshot
local show_dash = false
local audio_stream = false
local audio_rate = 16000
local W, H, COLS, ROWS

local PROTO_SUB = "hermes-gadget.v1"
local GRAY, WHITE, YELLOW, GREEN, CYAN, RED = 0x888888, 0xFFFFFF, 0xFFE066, 0x7CE07C, 0x66D9EF, 0xFF6666

local function now() return mk.time.ms() end

local function log(s, color)
  lines[#lines + 1] = { s, color or WHITE }
  while #lines > 9 do table.remove(lines, 1) end
end

local function wrap(text, cols)
  local out = {}
  for para in (text .. "\n"):gmatch("([^\n]*)\n") do
    if para == "" then out[#out + 1] = "" else
      local line = ""
      for word in para:gmatch("%S+") do
        if #line == 0 then line = word
        elseif #line + 1 + #word <= cols then line = line .. " " .. word
        else out[#out + 1] = line; line = word end
      end
      out[#out + 1] = line
    end
  end
  return out
end

local function send(t)
  local s = mk.json.encode(t)
  if s then mk.ws.send(s) end
end

-- ── handshake ──────────────────────────────────────────────────────────

local function hello()
  send({
    type = "hello", proto = 1, device_id = device_id,
    name = cfg.name or "MeowKit", board = "meowkit-s3", firmware = "phase5-0.1",
    caps = {
      display = { width = W, height = H, color = true, charset = "latin1",
                  text_cols = COLS, text_rows = ROWS },
      speaker = { rate = 16000, format = "pcm16" },
    },
    actions = {
      { name = "beep", description = "Play a short beep on the MeowKit speaker.",
        params = { type = "object", properties = {} } },
    },
  })
  state = "handshake"
end

local function on_challenge(m)
  if m.enrolled then
    local mac = mk.crypto.hmac_sha256(key, "hermes-gadget/v1|" .. device_id .. "|" .. m.nonce)
    if mac then send({ type = "auth", mac = mk.crypto.b64_encode(mac) }) end
  else
    send({ type = "auth", key = mk.crypto.b64_encode(key) })
  end
end

-- ── dispatch ───────────────────────────────────────────────────────────

local function on_text(ev)
  local m, err = mk.json.decode(ev.text)
  if not m then return end
  local t = m.type

  if t == "challenge" then on_challenge(m)
  elseif t == "welcome" then
    session, paired = m.session, m.paired == true
    hb_s = m.heartbeat_s or 20
    state = paired and "ready" or "pairing"
    if not paired then log("waiting for pairing...", YELLOW) end
  elseif t == "pairing" then
    state = "pairing"; card = nil
    log("code " .. (m.code or "?"), YELLOW)
  elseif t == "paired" then
    state = "ready"; paired = true; log("paired ✓", GREEN)
  elseif t == "unpaired" then
    state = "pairing"; paired = false; log("unpaired", RED)
  elseif t == "ping" then
    send({ type = "pong", ts = m.ts })
  elseif t == "notice" then
    notice = { text = m.text or "", expires = now() + (m.ttl_s or 8) * 1000 }
  elseif t == "turn.start" then
    status_line = "thinking..."
  elseif t == "status" then
    status_line = m.text or ""
  elseif t == "transcript" then
    log("» " .. (m.text or ""), CYAN)
  elseif t == "reply" then
    if not m.interim then log("« " .. (m.text or ""), WHITE) end
  elseif t == "turn.end" then
    status_line = ""
  elseif t == "prompt" then
    prompt_q = { id = m.id, title = m.title or "?", text = m.text or "", shown_at = now() }
  elseif t == "prompt.close" then
    if prompt_q and prompt_q.id == m.id then prompt_q = nil end
  elseif t == "display" then
    card = { title = m.title or "", body = m.body or "",
             expires = (m.ttl_s and m.ttl_s > 0) and (now() + m.ttl_s * 1000) or nil }
  elseif t == "dashboard" then
    dash = m
  elseif t == "audio.start" then
    audio_rate = m.rate or 16000
    mk.audio.stream_start(audio_rate)
    audio_stream = true
  elseif t == "audio.end" or t == "audio.abort" then
    if audio_stream then mk.audio.stream_stop(); audio_stream = false end
  elseif t == "action" then
    if m.name == "beep" then
      mk.audio.beep()
      send({ type = "action.result", id = m.id, ok = true, result = {} })
    else
      send({ type = "action.result", id = m.id, ok = false, error = "unsupported: " .. tostring(m.name) })
    end
  elseif t == "error" then
    log("err: " .. tostring(m.message or m.code), RED)
  end
end

local function on_bin(ev)
  if not audio_stream or #ev.data < 5 then return end
  if string.byte(ev.data, 1) == 1 then          -- channel 1 = audio
    mk.audio.stream_write(string.sub(ev.data, 5))
  end
end

-- ── connection management ──────────────────────────────────────────────

local function try_connect()
  if not mk.wifi.connected() then state = "wifi"; return end
  last_try = now()
  local ok = mk.ws.connect(cfg.host, cfg.port, cfg.path or "/gadget",
                           { subprotocol = PROTO_SUB })
  state = ok and "connecting" or "offline"
end

local function drop_ws()
  if audio_stream then mk.audio.stream_stop(); audio_stream = false end
  mk.ws.close()
end

-- ── UI ─────────────────────────────────────────────────────────────────

local function draw()
  mk.display.clear(0x000000)
  local head = (cfg.name or "MeowKit") .. " · " .. state
  local hc = ({ ready = GREEN, pairing = YELLOW, offline = RED, wifi = RED })[state] or GRAY
  mk.display.text(6, 4, head, hc, 1)

  if prompt_q then
    mk.display.text(6, 24, prompt_q.title, YELLOW, 1)
    for i, l in ipairs(wrap(prompt_q.text, COLS - 2)) do
      if i <= 5 then mk.display.text(6, 40 + (i - 1) * 16, l, WHITE, 1) end
    end
    mk.display.text(6, H - 18, "A: yes    LEFT: no", GRAY, 1)
    return
  end

  if show_dash and dash then
    mk.display.text(6, 24, "dashboard", YELLOW, 1)
    local y = 42
    for _, s in ipairs(dash.sessions or {}) do
      if y < H - 24 then
        mk.display.text(6, y, s.label .. " [" .. (s.state or "?") .. "]",
                        s.state == "waiting" and YELLOW or (s.state == "busy" and GREEN or GRAY), 1)
        y = y + 16
      end
    end
    for _, tl in ipairs(dash.tiles or {}) do
      if y < H - 24 then
        mk.display.text(6, y, tl.label .. ": " .. (tl.value or ""),
                        tl.state == "bad" and RED or (tl.state == "warn" and YELLOW or GRAY), 1)
        y = y + 16
      end
    end
    mk.display.text(6, H - 18, "UP: chat", GRAY, 1)
    return
  end

  local y = 24
  if card then
    mk.display.text(6, y, card.title, YELLOW, 1); y = y + 18
    for i, l in ipairs(wrap(card.body, COLS - 2)) do
      if y < H - 34 then mk.display.text(6, y, l, WHITE, 1); y = y + 16 end
    end
  else
    for _, ln in ipairs(lines) do
      mk.display.text(6, y, ln[1]:sub(1, COLS), ln[2], 1); y = y + 16
    end
  end

  if notice and now() < notice.expires then
    mk.display.text(6, H - 34, notice.text:sub(1, COLS), YELLOW, 1)
  elseif status_line ~= "" then
    mk.display.text(6, H - 34, status_line:sub(1, COLS), GRAY, 1)
  end
  mk.display.text(6, H - 18, "UP: dash", GRAY, 1)
end

-- ── lifecycle ──────────────────────────────────────────────────────────

local last_up, last_a, last_left = false, false, false

function on_open()
  W, H = mk.display.width(), mk.display.height()
  COLS = math.floor((W - 12) / 6)
  ROWS = math.floor((H - 40) / 16)

  local raw = mk.fs.read("/apps/gadget/config.lua")
  if raw then
    local f = load(raw)
    if f then local ok, t = pcall(f); if ok and type(t) == "table" then cfg = t end end
  end
  if not cfg then cfg_err = "no config.lua (copy config.example.lua)"; return end

  key = mk.fs.read("/apps/gadget/key.bin")
  if not key or #key ~= 32 then
    key = mk.crypto.random(32)
    if not mk.fs.write("/apps/gadget/key.bin", key) then
      cfg_err = "cannot save key.bin"; return
    end
  end
  device_id = "hg-" .. mk.crypto.sha256_hex(key):sub(1, 16)
  log("id " .. device_id, GRAY)
  try_connect()
end

function on_running(dt)
  if not cfg then
    mk.display.clear(0x000000)
    mk.display.text(6, 10, "Gadget setup", YELLOW, 2)
    for i, l in ipairs(wrap(cfg_err or "?", COLS)) do
      mk.display.text(6, 44 + (i - 1) * 16, l, WHITE, 1)
    end
    return
  end

  -- pump the socket, drain events
  local guard = 0
  while guard < 64 do
    guard = guard + 1
    local ev = mk.ws.poll()
    if not ev then break end
    last_rx = now()
    if ev.type == "open" then hello()
    elseif ev.type == "text" then on_text(ev)
    elseif ev.type == "bin" then on_bin(ev)
    elseif ev.type == "close" or ev.type == "error" then
      if audio_stream then mk.audio.stream_stop(); audio_stream = false end
      state = "offline"; log("link down", RED)
      break
    end
  end

  -- reconnect + dead-link watchdog
  local t = now()
  if not mk.ws.connected() and t - last_try > 5000 then try_connect() end
  if mk.ws.connected() and t - last_rx > (hb_s * 3) * 1000 then
    log("heartbeat timeout", RED); drop_ws(); state = "offline"; last_try = t
  end
  if card and card.expires and t > card.expires then card = nil end

  -- buttons (edge-detected)
  local up    = mk.input.down("up")
  local a     = mk.input.down("a")
  local left  = mk.input.down("left")
  if up and not last_up then show_dash = not show_dash; draw() end
  if prompt_q and t - prompt_q.shown_at > 600 then
    if a and not last_a then send({ type = "prompt.reply", id = prompt_q.id, answer = "yes" }); prompt_q = nil; draw() end
    if left and not last_left then send({ type = "prompt.reply", id = prompt_q.id, answer = "no" }); prompt_q = nil; draw() end
  end
  last_up, last_a, last_left = up, a, left

  draw()
end

function on_close()
  drop_ws()
end
