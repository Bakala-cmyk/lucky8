# M5Stick Plus "Shake Life Coach" — 开发计划

## Context

在空目录 `D:\code\lucky8` 中从零搭建一个嵌入式固件项目。用户摇动 M5Stick Plus 设备，IMU（MPU6886 加速度计）检测到摇晃后触发一次 OpenAI GPT API 调用，生成一句励志/运势英文短句，显示在设备屏幕上。技术栈：PlatformIO + Arduino framework + C++，英文优先（后期可叠加中文字体）。

---

## 项目结构

```
D:\code\lucky8\
├── platformio.ini              # 编译/上传配置
├── src/
│   ├── main.cpp                # setup() + loop()，状态机主循环
│   ├── config.h                # WiFi/API KEY/阈值（gitignore）
│   ├── config.h.example        # 模板，提交到版本库
│   ├── shake_detector.h/.cpp   # IMU 轮询 + 摇晃算法
│   ├── api_client.h/.cpp       # WiFi 连接 + HTTPS POST to OpenAI
│   ├── display_manager.h/.cpp  # LCD 封装：空闲/加载/消息/错误画面
│   └── state_machine.h/.cpp    # AppState enum + 状态转换
├── .gitignore                  # 排除 config.h / .pio/ / .vscode/
└── test/
    └── README.md               # 手动测试清单
```

---

## 关键依赖（platformio.ini）

```ini
[env:M5StickCPlus]
platform  = espressif32
board     = m5stick-c
framework = arduino

board_build.f_cpu      = 240000000L
board_build.partitions = huge_app.csv   ; ~3 MB app 分区

upload_speed   = 1500000
monitor_speed  = 115200

lib_deps =
    https://github.com/m5stack/M5StickC-Plus.git   ; LCD / IMU / 按钮
    bblanchon/ArduinoJson @ ^7.0.0                 ; JSON 解析

build_flags = -DCORE_DEBUG_LEVEL=1
```

---

## config.h 关键参数

```cpp
// WiFi
#define WIFI_SSID     "..."
#define WIFI_PASSWORD "..."

// OpenAI
#define OPENAI_API_KEY   "sk-..."
#define OPENAI_API_HOST  "api.openai.com"
#define OPENAI_API_PATH  "/v1/chat/completions"
#define OPENAI_MODEL     "gpt-4o-mini"   // 最便宜、最快

// 摇晃检测
#define SHAKE_THRESHOLD    1.8f   // 超出 1g 重力的加速度余量（单位 g）
#define SHAKE_DURATION_MS  300    // 持续摇晃毫秒数才触发
#define SHAKE_COOLDOWN_MS  5000   // 触发后冷却时间，防止连发

// 显示
#define MSG_DISPLAY_SECONDS  10
```

---

## 状态机设计

```
IDLE ──(摇晃/BtnA)──► CONNECTING
                           │
              WiFi OK ─────┤──── WiFi 失败 ──► ERROR
                           ▼
                        LOADING
                           │
              API OK  ─────┤──── API 失败  ──► ERROR
                           ▼
                       DISPLAYING
                           │
              超时/BtnA ───┘
                           ▼
                          IDLE
```

状态文件：`src/state_machine.h` 定义 `enum class AppState { IDLE, CONNECTING, LOADING, DISPLAYING, ERROR }`，存储进入时间戳供超时判断。

---

## 摇晃检测算法（shake_detector.cpp）

1. 每 20 ms（50 Hz）在 `loop()` 中调用 `ShakeDetector::update()`
2. 读取 MPU6886 三轴加速度 `(ax, ay, az)`，单位 g
3. 计算 `excess = |sqrt(ax²+ay²+az²) - 1.0|`
4. `excess > SHAKE_THRESHOLD` 持续 ≥ `SHAKE_DURATION_MS` → 触发
5. 触发后设置 `_lastTriggerMs`，冷却期内不重复触发
6. 返回 `true` 代表本次 loop 确认了一次摇晃事件

---

## API 调用流程（api_client.cpp）

### WiFi 连接
- `WiFi.begin(SSID, PWD)`，15 秒超时，失败返回 false
- 仅在 CONNECTING 状态时连接，DISPLAYING 结束后 `WiFi.disconnect(true)` 节省功耗

### HTTPS POST to OpenAI
```
POST https://api.openai.com/v1/chat/completions
Headers:
  Content-Type: application/json
  Authorization: Bearer <OPENAI_API_KEY>

Body:
{
  "model": "gpt-4o-mini",
  "max_tokens": 60,
  "messages": [
    {"role": "system", "content": "You are a fortune-teller life coach. Reply with ONE short motivational sentence under 20 words. Be vivid and inspiring."},
    {"role": "user", "content": "<random prompt from pool>"}
  ]
}
```

### 提示词池（随机选一条）
```
"Give me today's fortune."
"Inspire me for today."
"What's my life advice for today?"
"Give me a powerful motto for this moment."
```

### 响应解析（ArduinoJson 7）
提取路径：`respDoc["choices"][0]["message"]["content"].as<String>()`

### 注意事项
- `WiFiClientSecure::setInsecure()` 跳过证书校验（个人设备可接受）
- `http.useHTTP10(true)` 避免 chunked transfer，允许流式解析
- `max_tokens=60` 控制费用，约 0.000015 USD/次
- 使用 `ESP.getFreeHeap()` 在调试阶段监控堆内存

---

## 显示管理（display_manager.cpp）

使用 `M5.Lcd`（内置 TFT_eSPI，ST7789，135×240）：

| 状态        | 画面内容                                     |
|------------|---------------------------------------------|
| IDLE       | 黑底 + 中央小字 `"Shake me!"` + 图标        |
| CONNECTING | 黄色 `"Connecting..."` + 滚动点动画          |
| LOADING    | 蓝色 `"Thinking..."` + 进度动画              |
| DISPLAYING | 白字消息，`setTextWrap(true)`，自动换行      |
| ERROR      | 红色错误简短说明                             |

屏幕方向：`setRotation(3)`（横屏 240×135）  
字体：`setTextFont(2)`（内置 16px，每行约 30 字符，共 8 行）

---

## main.cpp 核心逻辑骨架

```cpp
void setup() {
    M5.begin();
    M5.Imu.Init();
    display.begin();
    display.showIdle();
}

void loop() {
    M5.update();
    switch (sm.current()) {
    case IDLE:
        if (shaker.update() || M5.BtnA.wasPressed())
            → CONNECTING, display.showConnecting();
        delay(20);
        break;
    case CONNECTING:
        api.connectWiFi() ? → LOADING : → ERROR;
        break;
    case LOADING:
        msg = api.fetchFortune();
        msg.length() > 0 ? → DISPLAYING : → ERROR;
        break;
    case DISPLAYING:
        if (timeout || BtnA) { WiFi.disconnect(); → IDLE; }
        break;
    case ERROR:
        if (BtnA || BtnB) → IDLE;
        break;
    }
}
```

---

## 关键文件清单

| 文件 | 核心职责 |
|------|---------|
| `platformio.ini` | 板型、分区表、库依赖 |
| `src/config.h` | 所有凭据和调参常量（不提交） |
| `src/main.cpp` | 状态机主循环，组装所有模块 |
| `src/shake_detector.cpp` | IMU 50Hz 轮询 + 阈值算法 |
| `src/api_client.cpp` | WiFiClientSecure + HTTPClient + ArduinoJson |
| `src/display_manager.cpp` | LCD 封装，各状态画面渲染 |

---

## 分阶段验证步骤

### Phase 1 — 硬件烟雾测试
1. 最小 sketch：`M5.Lcd.print("Hello")` → 确认屏幕亮起
2. Serial Monitor 打印加速度计原始值，静止时应约 `(0, 0, 1)`
3. 实现 ShakeDetector，Serial 打印 "SHAKE!"，手动调整 `SHAKE_THRESHOLD`

### Phase 2 — WiFi + HTTPS
4. 连接 WiFi，Serial 打印 IP
5. HTTPS GET `httpbin.org/get`，验证 TLS + `setInsecure()` 正常
6. POST 到 OpenAI，Serial 打印原始 JSON 响应

### Phase 3 — JSON 解析
7. ArduinoJson 解析，打印 `choices[0].message.content`
8. 用 `ESP.getFreeHeap()` 前后对比，确认无内存泄漏

### Phase 4 — 显示集成
9. 把解析结果渲染到 LCD（英文），测试长句自动换行
10. 完整状态机联调，BtnA 手动触发代替摇晃

### Phase 5 — 完整联调
11. 端到端摇晃→显示，连续触发 3 次
12. 测试错误路径：WiFi 断开、错误 API Key、超时
13. 计时从摇晃确认到消息上屏的延迟，目标 < 8 秒

### Phase 6（可选）— 中文支持
14. 添加 `U8g2_for_TFT_eSPI` + `u8g2_font_wqy12_t_chinese2` 字体
15. 修改 system prompt 要求返回中文，测试中文渲染和自动换行

---

## 成本估算

- gpt-4o-mini：约 $0.000015/次调用（60 tokens output）
- 每天摇 50 次 ≈ $0.00075/天，极低
