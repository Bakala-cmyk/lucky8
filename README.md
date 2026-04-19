# Lucky8 — Shake Life Coach

基于 **M5Stick Plus** 的摇一摇求签硬件：摇晃设备触发一次 LLM 调用，屏幕上显示一句英文励志/运势短句。

---

## Features

- **摇晃触发** — 50 Hz 轮询 MPU6886 加速度计，`|a| - 1g > 1.8g` 持续 300 ms 才判定为"摇晃"，避免误触；触发后 5 s 冷却，防止连发。
- **按钮兜底** — BtnA 可代替摇晃手动触发，方便调试和演示。
- **LLM 求签** — HTTPS POST 到 OpenAI 兼容接口，`max_tokens=60` 控费；system prompt 要求 ≤ 20 词的诗意/励志短句，无引号/标签。
- **完整状态机** — `IDLE → CONNECTING → LOADING → DISPLAYING → ERROR`，每个状态独立画面，带进入时间戳做超时判断。
- **LCD 自适应换行** — 横屏 240×135，按单词级 word-wrap 在屏幕内自动换行。
- **错误恢复** — WiFi 失败 / HTTP 非 200 / JSON 解析失败都落到 ERROR 屏，按 A/B 键回 IDLE。
- **显示超时 + 功耗** — 消息显示 10 s 后（或 BtnA 提前）自动关闭 WiFi 回空闲，避免常连耗电。
- **离线可构建** — `lib/` 下 vendor 了 `M5StickCPlus` 和 `ArduinoJson` 源码，`pio run` 不需要联网拉依赖。
- **凭据隔离** — `src/config.h` 被 gitignore，仓库只提交 `config.h.example` 模板。

---

## 硬件

- **M5Stick Plus**（M5Stack 出品，带 1.14" ST7789 LCD + MPU6886 IMU + 两按键）
- USB-C 线（烧录 + 供电）
- 一个能上网的 WiFi
- 一个 OpenAI 兼容的 API Key

---

## 项目结构

```
lucky8/
├── platformio.ini           # 构建配置（espressif32 + m5stick-c）
├── src/
│   ├── main.cpp             # setup() + loop() 主循环，组装状态机
│   ├── state_machine.h      # AppState enum + 状态切换
│   ├── shake_detector.*     # IMU 轮询 + 摇晃算法
│   ├── api_client.*         # WiFi + HTTPS POST + JSON 解析
│   ├── display_manager.*    # LCD 各状态画面 + word-wrap
│   ├── config.h.example     # 凭据模板（提交到仓库）
│   └── config.h             # 真实凭据（gitignored）
├── lib/
│   ├── M5StickCPlus/        # vendored，M5 官方库
│   └── ArduinoJson/         # vendored，v7.4.3
├── test/README.md           # 分阶段手动测试清单
├── plan.md                  # 整体设计方案
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
#define OPENAI_API_HOST  "api.openai.com"     // 或自定义网关域名
#define OPENAI_API_PATH  "/v1/chat/completions"
#define OPENAI_MODEL     "gpt-4o-mini"        // 按你的网关支持的模型改
```

### 2. 编译 + 烧录

需要装 [PlatformIO](https://platformio.org/install) CLI 或 VS Code 插件。

```bash
pio run                # 仅编译
pio run -t upload      # 编译 + 烧录
pio device monitor     # 看串口日志
```

### 3. 使用

- **IDLE** 屏出现 `~ Shake me! ~` → 摇一摇，或按 A 键
- 依次看到 `Connecting WiFi...` → `Asking the cosmos...` → 励志短句
- 10 s 后自动回 IDLE；或按 A 键提前返回
- 出错时屏幕变红，按 A 键重试

---

## 状态机

```
IDLE ──(摇晃 / BtnA)──► CONNECTING
                             │
                   WiFi OK ──┤── WiFi 失败 ──► ERROR
                             ▼
                          LOADING
                             │
                   API OK  ──┤── API 失败  ──► ERROR
                             ▼
                         DISPLAYING
                             │
                   超时 / BtnA ──► IDLE
```

---

## 调参

`src/config.h` 里这几个常量决定手感：

| 常量 | 默认 | 作用 |
|------|------|------|
| `SHAKE_THRESHOLD` | `1.8f` | 超出 1g 重力的加速度余量（单位 g）。越大越难触发 |
| `SHAKE_DURATION_MS` | `300` | 持续摇晃多少毫秒才算数。越大越抗误触 |
| `SHAKE_COOLDOWN_MS` | `5000` | 触发后多久内不再响应，防止连发 |
| `MSG_DISPLAY_SECONDS` | `10` | 消息在屏幕上停留秒数 |

建议 Phase 1 烧录后串口打印加速度原始值，按自己摇晃习惯调 `SHAKE_THRESHOLD`。

---

## 本版本做了什么

首版可运行固件：

- 按 `plan.md` 落地了 5 个模块 + 状态机骨架（main / state_machine / shake_detector / api_client / display_manager）
- `platformio.ini` 配成 `m5stick-c` @ 240 MHz + `huge_app.csv` 分区
- 把 `M5StickCPlus` 和 `ArduinoJson` 两个库 vendor 到 `lib/`，`pio run` 离线可跑
- 验证了 `pio run` 本地编译成功（RAM 14.6% / Flash 31.5%）
- 验证了 API 网关连通性与模型选型（默认用 `deepseek-chat` 或 `gpt-4o-mini`，视 Key 的网关支持）
- `test/README.md` 写好了 Phase 1–6 的人工验证清单

**尚未完成**（计划里的后续）：

- Phase 6 中文字体支持（`U8g2_for_TFT_eSPI` + wqy12）
- 实机联调摇晃手感调参
- 端到端延迟测量（目标 < 8 s）

---

## License

未定。硬件/库依赖遵循各自原 License（`lib/*/LICENSE*`）。
