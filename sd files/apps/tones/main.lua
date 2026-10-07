-- tones / main.lua — proves mk.audio (ES8311 DAC → NS4150B amp → speaker)
-- from Lua. Each key plays a short note. Controls: Up/Left/Down/Right/A = notes,
-- short B = back, hold B = exit. Volume is kept low (sine tones are hard on the
-- little 1 W speaker).

local notes = { up = 523, left = 587, down = 659, right = 698, a = 784 } -- ~C5..G5
local order = { "up", "left", "down", "right", "a" }
local prev  = {}
local last  = "-"

local function draw()
  mk.display.clear(0x000000)
  mk.display.text(10, 8,  "Tones (Lua audio)",      0xBBE700, 2)
  mk.display.text(10, 46, "Press keys to play:",     0xFFFFFF, 1)
  mk.display.text(10, 64, "Up  Left  Down  Right  A", 0xBBBBBB, 1)
  mk.display.text(10, 96, "last: " .. last,           0x00FF66, 1)
  mk.display.text(10, 222, "short B: back   hold B: exit", 0x888888, 1)
end

function on_open()
  mk.audio.volume(40)
  draw()
end

function on_running(dt)
  for _, k in ipairs(order) do
    local d = mk.input.down(k)
    if d and not prev[k] then
      mk.audio.tone(notes[k], 180, 40)   -- blocking ~180 ms
      last = k .. "  (" .. notes[k] .. " Hz)"
      draw()
    end
    prev[k] = d
  end
end
