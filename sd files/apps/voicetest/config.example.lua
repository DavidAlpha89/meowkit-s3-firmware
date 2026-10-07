-- voicetest config TEMPLATE.
-- Copy this to config.lua on the SD card and fill in your backend's values.
-- config.lua holds a secret token, so it stays on the card — never committed.
return {
  url       = "ws://192.168.50.200:9462/xiaozhi",  -- backend Xiaozhi-v3 WS (mesh-reachable)
  device_id = "your-device-id",                    -- lowercase; must match backend allowlist
  token     = "your-websocket-token",              -- backend PELICAN_WEBSOCKET_TOKEN (>=32 chars)
}
