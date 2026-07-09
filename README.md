# Lucky8 — Pac-Man Shake Oracle

基于 **M5Stick Plus** 的摇一摇求签硬件 —— 街机吃豆人主题皮肤 + 按语境分类的音效。摇一下，Pac-Man 追上 "WiFi 鬼"，四只蓝鬼在 LOADING 飘来飘去，LLM 返回的运势逐字打出、播一段与 mood 对应的小旋律，最后吃豆人从右往左把文字一行行吃光。

> 分支对照：`main` 是静态 UI 的最小可运行版；`ui` 分支（本 README 所在）把整个显示层重写成吃豆人主题。

---

## 与 `main` 分支的差异

| 维度 | `main` | `ui`（当前分支） |
|------|--------|-----------------|
| 显示层 | 静态文字 + 单色状态屏 | 30 fps 动画，`TFT_eSprite` 双缓冲无闪烁 |
| IDLE | "Shake me!" 静态文字 | Pac-Man 来回吃 14 颗豆，吃完一排 → 豆子重生 + 换随机颜色；`~ SHAKE ME! ~` ⇄ `~ lucky8! ~` 交替闪烁 |
| CONNECTING | "Connecting WiFi..." 黄字 | Pac-Man 循环追红色 Blinky；WiFi 成功后进入 **CATCHUP** 加速撞击 → 吃掉鬼 + `+100` 上飘 |
| LOADING | "Asking the cosmos..." 蓝字 | 中央脉冲能量豆 + 四只蓝色 scared 鬼随机游走 |
| DISPLAYING | 一次性 print 全文 | 打字机逐字 + 按 mood 播 jingle + 配角呼吸 → **吃豆人从右往左一行行完整吃掉** |
| ERROR | 红字错误提示 | 经典 Pac-Man 六帧死亡动画 + `GAME OVER` + 下行死亡音 |
| 音效 | 无 | 非阻塞音序器 + 11 段预置旋律（siren / 吃鬼 / 能量豆 / scared BGM / 打字咔哒 / 吃字 waka / 死亡下行 / 5 种 mood jingle） |
| API | 纯文本 | JSON response_format 返回 `{mood, text}`，5 种 mood 各自驱动专属音 + 画 |
| 运势多样性 | 纯靠 LLM | `temperature=1.2` + 15 条 prompt 池 + 每次追加 `[seed:<esp_random>]` 随机 nonce，防任何服务端缓存 |
| 网络阻塞 | 主循环被 `WiFi.begin()` / `HTTPClient` 阻塞，动画卡住 | WiFi/API 跑在 FreeRTOS 核心 0 的独立 task，核心 1 动画不间断 |

---

## 5 种 Mood 映射

LLM 在返回文本时附带 `mood`，驱动 DISPLAYING 阶段的 jingle + 右侧配角：

| mood      | jingle                            | 配角 sprite                  |
|-----------|-----------------------------------|------------------------------|
| `lucky`   | 上行琶音 C5→E5→G5→C6（吃鬼音）    | 蓝色 scared 鬼被吃           |
| `warning` | 两音交替警笛 A3↔E4 × 4            | 红 Blinky，眼睛左右扫        |
| `calm`    | 柔和 F4→A4→C5 长音                | 睡着的吃豆人 + 上飘的 `z`    |
| `bold`    | 扫频上行 + 短吃鬼音                | 闪烁能量豆                   |
| `love`    | 樱桃叮咚 E5 + 两个低音心跳          | 樱桃 🍒                     |

---

## Features

- **吃豆人街机主题** — 全 5 个状态都有主题专属动画 + 音效
- **按语境变化的背景音** — LLM 同时返回 mood，驱动不同 jingle + 配角 sprite
- **逐字打字机 + 逐行吞字** — 文字逐字出现，结束后 Pac-Man 一行一行从右往左吃光
- **中文 UTF-8 展示** — API 文本使用 U8g2 + 文泉驿 16px GB2312 字体渲染，支持中文/英文混排、换行、滚动和吞字
- **摇晃触发** — 50 Hz 轮询 MPU6886，`|a|-1g > 1.8g` 持续 300 ms 触发，5 s 冷却
- **按钮兜底** — BtnA 代替摇晃；DISPLAYING 中按 A 直接跳到吞字阶段
- **非阻塞网络** — WiFi 连接 + HTTPS 请求在核心 0 的 FreeRTOS task 执行，核心 1 动画不卡
- **双缓冲渲染** — 240×135 sprite 整屏先画再一次性 push，无闪烁
- **运势永远新鲜** — 随机 nonce + 高温采样 + 扩展 prompt 池，从设备侧根除重复
- **离线可构建** — `lib/` 下 vendor `M5StickCPlus` + `ArduinoJson` + `U8g2_for_TFT_eSPI`，`pio run` 不走网络
- **凭据隔离** — `src/config.h` gitignore，只提交 `config.h.example`

---

## 硬件

- **M5Stick Plus**（1.14" ST7789 LCD + MPU6886 IMU + 两按键 + 蜂鸣器）
- USB-C 线（烧录 + 供电）
- 一个能上网的 WiFi
- 一个 OpenAI 兼容的 API Key（项目默认 `deepseek-chat`，因为 `gpt-4o-mini` 在部分网关不可用）

---

## 项目结构

```
lucky8/
├── platformio.ini           # espressif32 + m5stick-c @ 240 MHz + huge_app
├── src/
│   ├── main.cpp             # 状态机主循环，核心 0 worker task 调度
│   ├── state_machine.h      # AppState enum
│   ├── shake_detector.*     # IMU 轮询 + 摇晃算法
│   ├── api_client.*         # HTTPS + JSON response_format + mood 解析
│   ├── display_manager.*    # TFT_eSprite 渲染器 + Pac-Man 原语 + UTF-8 中文文字
│   ├── animations.*         # IdleAnim / ConnectingAnim / LoadingAnim /
│   │                          DisplayingAnim(TYPING→HOLD→EAT) / ErrorAnim
│   ├── sound_player.*       # 非阻塞音序器 + 11 段预置旋律
│   ├── config.h.example     # 凭据模板
│   └── config.h             # 真实凭据（gitignored）
├── lib/
│   ├── M5StickCPlus/        # vendored
│   ├── ArduinoJson/         # vendored
│   └── U8g2_for_TFT_eSPI/   # vendored, UTF-8 Chinese text rendering
├── test/README.md           # 分阶段手动测试清单
├── plan.md                  # 初版总体设计
├── ui_plan.md               # 吃豆人 UI 改造详细计划
└── .gitignore
```

---

## 快速开始

### 1. 填凭据

```bash
cp src/config.h.example src/config.h
```

编辑 `src/config.h`：

```cpp
#define WIFI_SSID        "你的WiFi"
#define WIFI_PASSWORD    "你的密码"
#define OPENAI_API_KEY   "sk-..."
#define OPENAI_API_HOST  "api.openai.com"    // 或自定义网关域名
#define OPENAI_API_PATH  "/v1/chat/completions"
#define OPENAI_MODEL     "deepseek-chat"     // 按你的网关支持的模型改
```

### 2. 编译 + 烧录

需要 [PlatformIO](https://platformio.org/install) CLI 或 VS Code 插件。

```bash
pio run                # 仅编译
pio run -t upload      # 编译 + 烧录
pio device monitor     # 看串口日志（可看到 [API] mood=X text=...）
```

### 3. 玩法

- **LANGUAGE**：A 选中文，B 选 English；后续 UI 提示和 API 返回内容都会跟随该语言
- **MODE**：A 选求签，B 选 Truth
- **IDLE**：Pac-Man 在底排吃豆，标题交替闪 → **摇一摇** 或按 **A**
- **CONNECTING**：Pac-Man 追红鬼，连上时快速撞击 + `+100`
- **LOADING**：能量豆模式，四蓝鬼游走
- **DISPLAYING**：逐字打字 → mood jingle + 配角 → Pac-Man 从右往左逐行吃字
- **ERROR**：死亡动画 + `GAME OVER`，按 A/B 回 IDLE

DISPLAYING 中按 **A** 会直接跳到吞字阶段，不用等 10 秒。

---

## 状态机

```
LANGUAGE_SELECT ──(A 中文 / B English)──► MODE_SELECT
                                             │
                              A 求签 / B Truth
                                             ▼
IDLE ──(摇晃 / BtnA)──► CONNECTING
                              │
                    WiFi OK ──┤── WiFi 失败 ──► ERROR
                              ▼
                           LOADING
                              │
                    API OK  ──┤── API 失败  ──► ERROR
                              ▼
                        DISPLAYING
                      ┌──  TYPING ──┐
                      │     ↓       │
                      │   HOLD      │◄── BtnA 直接跳到 EAT
                      │     ↓       │
                      │    EAT      │
                      └─────┬───────┘
                            ▼
                           IDLE
```

---

## 调参

`src/config.h`：

| 常量 | 默认 | 作用 |
|------|------|------|
| `SHAKE_THRESHOLD` | `1.8f` | 超出 1g 重力的余量（g）。越大越难触发 |
| `SHAKE_DURATION_MS` | `300` | 持续摇晃多少毫秒才算数 |
| `SHAKE_COOLDOWN_MS` | `5000` | 触发后冷却时间 |
| `MSG_DISPLAY_SECONDS` | `10` | DISPLAYING 的 HOLD 阶段停留秒数（到点自动开吃） |

动画节奏（硬编码在 `src/animations.cpp`）：

| 项目 | 值 |
|------|----|
| 打字速度 | 28 ms / 字符 |
| 逐行吃字速度 | ~180 px/s（约 1.3 s/行） |
| IDLE Pac-Man 速度 | 1.3 px/frame（~39 px/s） |
| CONNECTING CHASE 速度 | Pac 1.8 / 鬼 1.3 px/frame |
| CONNECTING CATCHUP 速度 | Pac 3.6 / 鬼 0.6 px/frame |
| 死亡动画 | 80 ms × 6 帧 |

---

## 资源占用

编译后（ESP32 Arduino，`huge_app.csv` 分区）：

- **RAM**：~48 KB / 320 KB（14.7%，含 65 KB `TFT_eSprite` 框缓冲由 heap 动态分配）
- **Flash**：~1 MB / 3 MB（32.7%）
- **帧率**：稳定 30 fps（帧预算 33 ms，实测每帧绘制 + `pushSprite` 约 13–18 ms）

---

## License

未定。硬件/库依赖遵循各自原 License（`lib/*/LICENSE*`）。
