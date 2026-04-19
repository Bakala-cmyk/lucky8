# Lucky8 UI 改造计划 — 吃豆人主题

## 目标

把当前静态文字 UI 全面重做成 **Pac-Man 街机风**，配上按语境变化的音效与背景音。交互需要"活"起来 —— IDLE 有可爱的默认动画，等待与展示都带即时反馈，最后吃豆人把文字吃掉收尾。

---

## 核心体验脚本

> 玩家拿起设备 → 屏幕上吃豆人在吃豆 →
> 摇一摇 → 吃豆人追着 "WiFi 鬼"一路吃 → 抓住 →
> 能量豆激活，四鬼变蓝到处晃 → 屏幕显示今日运势，逐字打字机 →
> 根据语境播一段专属小旋律 →
> 若干秒后吃豆人从左跑到右，把整段文字一个字一个字吃光 →
> 回到 IDLE。

---

## 分状态设计

### 1. IDLE — 闲逛吃豆

**画面**
- 底部一排豆子（`.`），间距 10 px，跨越全屏 240 px
- 黄色吃豆人在底排来回跑，嘴巴开合两帧切换（240 ms/帧 → 约 4 fps 嘴巴）
- 走到右边界时翻转方向
- 右上角一枚能量豆（大圆）每 500 ms 闪一次
- 顶部居中 `SHAKE ME!`，每 800 ms 闪烁显示 / 隐藏

**音效**
- 每 ~1.5 s 发一次 `waka` (短 400 Hz+200 Hz 双音)，低音量，当"呼吸"声
- 完全静音模式（长按 B 键切换？）后续加

---

### 2. CONNECTING — 追 WiFi 鬼

**画面**
- Pac-Man 从左侧出发往右走，嘴巴开合
- 前方一只 **红色 Blinky**，顶上写小字 `WiFi`
- 两者都在底排一起位移，Pac-Man 追得稍快
- 顶部黄字 `CONNECTING...` 带 3 个点跑马灯
- WiFi 连上瞬间：Pac-Man 撞到鬼 → 鬼消失，弹 `+200` 黄色小字 200 ms → 进入 LOADING
- WiFi 失败：Pac-Man 追到边缘鬼溜走 → 进入 ERROR

**音效**
- 追击中：经典"siren"警笛音，两音交替 (A3 ↔ E4)，200 ms/音
- 抓到：三音上行 jingle (C5 → E5 → G5)
- 失败：下行 wail

---

### 3. LOADING — 能量豆模式

**画面**
- 屏幕中央一枚大能量豆，每 150 ms 闪
- 四只鬼（红/粉/青/橙）全部变蓝色 scared 状态
- 四鬼在屏幕内随机游走（简单线性+碰边反弹，避免真 pathfinding）
- 顶部黄字 `ASKING THE COSMOS...` 带跑马灯
- 停留到 API 返回为止

**音效**
- 能量豆激活音（上升扫频 200 Hz → 800 Hz，400 ms）
- 之后循环一小段 "scared" BGM（短促四音 loop，100 ms/音）

---

### 4. DISPLAYING — 打字机 + 结尾吞字

分两个子阶段：

#### 4a. TYPING（0 ~ 所有字打完）
- 黑底，文字从左上开始逐字打出，25 ms/字（短句约 1 秒）
- 每打一个字发一次极短 `click` (800 Hz, 10 ms)
- 字都打完后，播放 mood jingle（见下）
- mood jingle 放完后进入保持阶段

#### 4b. HOLD（mood jingle 播完 ~ MSG_DISPLAY_SECONDS 到期）
- 右侧放一个符合 mood 的配角 sprite（例如 lucky → 樱桃；love → 粉鬼 Pinky）
- sprite 做轻微 2 px 上下浮动（呼吸感）

#### 4c. EAT（自动进入）
- 吃豆人从屏幕最右进场，向左移动
- 每经过一个字符宽度（14 px），"吃掉"下一个字符 —— 用黑色矩形盖掉那个字 + 播一次 `waka`
- 一直吃到左边界，屏幕空 → 回 IDLE
- 整个吃的过程约 1.5 ~ 2 秒（取决于文字长度）

**触发提前结束**：DISPLAYING 任何子阶段按 A 键 → 直接跳到 IDLE（跳过动画）

---

### 5. ERROR — GAME OVER

**画面**
- 经典死亡动画：吃豆人原地，嘴巴每帧开得更大 → 最后裂成几条线 → 消失（6 帧，80 ms/帧）
- 红色 `GAME OVER` 大字居中
- 下方小字：错误原因 + `Press A to retry`

**音效**
- 经典死亡下降音：C5 → B4 → A4 → G4 → F4 → E4 → D4 → C4，80 ms/音

---

## Mood 分支

让 LLM 在返回文本的同时附带情绪标签，走 JSON 模式：

**新 system prompt（摘要）**：
> Return strict JSON `{"mood": "<one of: lucky|warning|calm|bold|love>", "text": "<one motivational sentence under 20 words>"}`. No markdown.

`api_client.fetchFortune()` 改签名：

```cpp
struct Fortune {
    String text;
    enum class Mood { LUCKY, WARNING, CALM, BOLD, LOVE } mood;
};
bool ApiClient::fetchFortune(Fortune& out);
```

### Mood → DISPLAYING jingle + 配角 sprite

| mood     | jingle                                         | 右侧 sprite           |
|----------|-----------------------------------------------|----------------------|
| `lucky`  | 上行琶音 C5-E5-G5-C6（吃鬼音）                 | 蓝色 scared 鬼被吃   |
| `warning`| siren 两音交替 4 次                            | 红 Blinky 摆尾       |
| `calm`   | 柔和 F4-A4-C5 长音                             | Pac-Man 闭眼睡       |
| `bold`   | 能量豆激活扫频 + 短吃鬼音                       | 闪烁能量豆           |
| `love`   | 樱桃音 E5 短促 + 两个心跳低音                   | 樱桃 🍒             |

---

## 架构改动

### 新模块

#### `src/sprites.h` — 所有像素精灵
- 用 `const uint8_t PROGMEM` 存 16×16 单色位图（每字节 8 像素）
- 颜色运行时指定（吃豆人黄 / 鬼按身份配色）
- 列表：
  - `PACMAN_RIGHT_OPEN` / `PACMAN_RIGHT_CLOSED`（左翻转即得左向版）
  - `GHOST_BODY`（通用，颜色外部传）
  - `GHOST_SCARED`（蓝色固定）
  - `PELLET_SMALL` / `PELLET_POWER`
  - `CHERRY`
  - `DEATH_FRAME_0..5`

#### `src/sound_player.{h,cpp}` — 非阻塞音序
```cpp
struct Note { uint16_t freq; uint16_t durMs; };

class SoundPlayer {
public:
    void begin();
    void play(const Note* seq, size_t len, bool loop=false);
    void stop();
    void update();   // call every loop tick; advances note when current ends
    bool playing() const;
private:
    const Note* _seq = nullptr;
    size_t _len = 0;
    size_t _idx = 0;
    uint32_t _noteStartMs = 0;
    bool _loop = false;
};
```
- 底层用 `M5.Beep.tone(freq, dur)` 或直接 `ledcWriteTone` 驱动 GPIO2 蜂鸣器
- 预定义音序都放在 `sound_player.cpp` 的 `constexpr Note` 数组里

#### `src/animations.{h,cpp}` — 每状态的动画器
```cpp
class Animation {
public:
    virtual void enter() = 0;
    virtual void update(uint32_t nowMs) = 0;  // 30 fps 调用
    virtual bool done() const { return false; }
    virtual ~Animation() = default;
};

class IdleAnim;        // pac-man 来回跑吃豆
class ConnectingAnim;  // pac-man 追 WiFi 鬼
class LoadingAnim;     // 能量豆 + 四只 scared 鬼
class DisplayingAnim;  // 三子阶段：TYPING / HOLD / EAT
class ErrorAnim;       // 死亡动画
```
每个动画器持有自己的时间轴与内部子状态，`update()` 里做纯绘制（不 delay）。

### 改动的现有模块

#### `display_manager.*`
- 退化为底层画笔封装：`drawSprite(x, y, sprite, color)` / `drawText(...)` / `clearRect(...)` / `fillRect(...)`
- `showIdle()` 等高层方法移到 `animations.cpp`

#### `api_client.*`
- 请求 body 加 `response_format: {"type": "json_object"}`
- system prompt 改成要求返回 JSON
- 解析 `choices[0].message.content` 里的 JSON，再二次 parse 出 `{mood, text}`
- Mood 字符串映射到 enum

#### `state_machine.h`
- 不变，或在 DISPLAYING 内部由 `DisplayingAnim` 自己跑子阶段

#### `main.cpp`
- `loop()` 结构变：
  ```cpp
  anim->update(millis());     // 绘制当前状态动画
  sound.update();              // 推进音序
  ```
- 不再每状态调用 `display.showXxx()`；状态切换时 `anim = new XxxAnim()` 并 `enter()`
- 删除 `delay(20)` / `delay(100)`，改 30 fps 固定节拍（帧间隔 33 ms）

#### `platformio.ini`
- 暂不需动

---

## 性能与资源预算

- 屏幕 240×135 = 32,400 像素，16-bit color 用整屏重绘约 65 KB/帧。30 fps 需要 ~1.9 MB/s SPI，M5 的 TFT 用 40 MHz SPI 刚够。优化：**只重绘 dirty 区域**（擦旧 sprite 位置 + 画新位置）
- PROGMEM 精灵总计约 1 KB，忽略不计
- 音序表几百字节，忽略不计
- RAM 当前占 14.6% (47 KB)，动画 + 音效新增估 5 KB，仍有大量余量

---

## 实施阶段

### Phase A — 基础设施（不改用户可见效果）
1. 写 `sound_player`，在 `setup()` 里播一段测试音确认蜂鸣器工作
2. 写 `sprites.h`，写一个 `drawSprite` 原语，在 IDLE 屏左上角画一个静止 Pac-Man 确认渲染
3. 写 `Animation` 抽象与 `main.cpp` 的新 loop 框架（暂时动画只画当前 sprite）

### Phase B — 逐状态铺开
4. `IdleAnim`：吃豆人来回跑吃豆 + 闪烁 `SHAKE ME!` + 低音 waka 呼吸
5. `ConnectingAnim`：追 WiFi 鬼 + siren
6. `LoadingAnim`：能量豆 + scared 鬼群 + scared BGM
7. `DisplayingAnim`：TYPING 打字机 + mood jingle + HOLD 配角呼吸 + EAT 吞字
8. `ErrorAnim`：死亡帧 + 下降音

### Phase C — 语境适配
9. `api_client` 改 JSON response_format + mood 解析
10. `DisplayingAnim` 按 mood 挑 jingle + 配角 sprite
11. 5 种 mood 都手工触发一遍（临时硬编码测）验证视听效果

### Phase D — 打磨
12. 调整各动画节奏（帧率、持续时长），实机看手感
13. 性能优化（dirty rect 重绘，必要时降帧率）
14. 可选：B 键静音模式、电量指示

---

## 风险 / 待定

- **蜂鸣器音质粗糙**：单声道方波，复杂旋律表现力有限。降低期望，走"像 Pac-Man"而非"还原 Pac-Man"。
- **屏幕刷新 flicker**：整屏重绘可能肉眼可见闪烁。必要时切到 dirty rect 或 sprite buffer。
- **JSON 稳定性**：LLM 偶尔返回非严格 JSON。`api_client` 需要 fallback：解析失败就 mood = `calm`、text 取原始字符串。
- **EAT 阶段定位**：需要知道每个字符在屏幕上的 x 坐标。打字机阶段记录每个字符的 bounding box，EAT 时复用。

---

## 最小可交付里程碑

Phase A + Phase B.4（IdleAnim）+ 一个固定的 mood（忽略 LLM mood）就已经是一个可玩的 demo。先冲这个节点拿反馈，再做全套。
