# Manual Test Checklist

## Phase 1 — Hardware smoke test
- [ ] Flash minimal sketch: `M5.Lcd.print("Hello")` → screen lights up with text
- [ ] Serial Monitor shows raw accelerometer values ~(0, 0, 1) at rest
- [ ] Serial Monitor prints "SHAKE!" when device is shaken
- [ ] Tune `SHAKE_THRESHOLD` in config.h until false positives are rare

## Phase 2 — WiFi + HTTPS
- [ ] Serial prints IP address after boot
- [ ] HTTPS GET to httpbin.org/get returns 200 (setInsecure works)
- [ ] POST to OpenAI returns HTTP 200 with raw JSON in Serial

## Phase 3 — JSON parsing
- [ ] `choices[0].message.content` prints correctly to Serial
- [ ] `ESP.getFreeHeap()` before/after API call shows no major leak

## Phase 4 — Display integration
- [ ] Fortune text renders on LCD with word-wrap
- [ ] Long sentences wrap correctly without overlapping screen edges

## Phase 5 — Full end-to-end
- [ ] BtnA manually triggers full cycle: connecting → loading → displaying
- [ ] Shaking triggers the same cycle
- [ ] Message auto-clears after MSG_DISPLAY_SECONDS
- [ ] BtnA during DISPLAYING returns to idle immediately
- [ ] WiFi off → ERROR screen → BtnA → back to idle
- [ ] Wrong API key → ERROR screen → BtnA → back to idle
- [ ] 3 consecutive shake cycles work without reset

## Phase 6 — Chinese support
- [x] Add vendored U8g2_for_TFT_eSPI + WQY 16px GB2312 Chinese font
- [x] System prompt changed to request Simplified Chinese output
- [ ] API Chinese response renders on LCD
- [ ] Chinese / English mixed response wraps correctly
- [ ] Typewriter, scroll, and eat-text animations handle UTF-8 glyphs correctly
