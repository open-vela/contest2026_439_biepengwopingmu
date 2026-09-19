# VelaGesture

**基于 openvela 的多模态智能手势交互终端**

2026 首届 openvela AI 硬件开发者大赛 参赛作品

队伍编号：439 | 队伍名称：biepengwopingmu | 赛道：AI 硬件产品创新

---

## 1. 项目简介

VelaGesture 是一个面向工业控制和数字孪生场景的多模态智能手势交互终端。它运行在 ESP32-P4 Function-EV-Board 上，基于 openvela 操作系统，通过摄像头识别手势、麦克风采集语音，将用户的物理交互转化为结构化事件，并由 AI Agent 驱动 Skill 完成任务执行。

**重要：本项目不是从零开发。** 它是在已有的「多模态智能手势交互终端」原型基础上，向 ESP32-P4 / openvela 嵌入式平台的迁移与系统升级。原有原型已具备手势识别、稳定门控、WebSocket 通信和古贤业务系统联动能力，本次参赛将这些能力迁移到嵌入式硬件平台，并新增了 ai_agent 和 Skill 执行能力。

## 2. 项目来源与迁移关系

### 原平台

原有手势交互终端原型运行在通用计算平台上，已实现：
- 摄像头手势检测（ESP-DL / CNN 模型）
- 手势分类（OK、Fist、Open Palm、Victory、Pinch）
- 稳定门控（连续帧计数过滤）
- Gesture Event JSON 协议
- WebSocket 双向通信
- 古贤业务系统联动
- TaskManager 任务状态管理

### 本次迁移

迁移到 ESP32-P4 + openvela 平台后：
- **保持不变：** 手势分类模型基础、稳定门控算法、Gesture Event 协议、WebSocket 协议、古贤业务系统服务端
- **新增：** ai_agent 事件驱动架构、guxian-control Skill、主动告警 + 执行场景、语音交互、NuttX 嵌入式适配
- **裁剪/重构：** 推理后端适配 P4 硬件加速、音频改为半双工、UI 简化为串口/LVGL

## 3. 核心功能

### 手势识别
摄像头实时捕获画面，推理后端（ESP-DL / TF-Lite Micro）识别手势类型。支持 OK、Fist、Open Palm、Victory、Pinch 五种手势。

### 稳定门控
连续 5 帧检测到相同手势才触发事件，避免瞬时误触。连续 3 帧不同手势才切换识别结果。

### Gesture Event
统一的 JSON 事件格式：
```json
{"type":"gesture","gesture":"ok","confidence":0.96,"stable":true,"timestamp":1726734600000}
```

### WebSocket
P4 设备与古贤业务系统之间的双向 JSON 通信。支持 gesture、task、status、error 四种设备→服务端消息，以及 alert、task_result、ack 三种服务端→设备消息。

### 语音
半双工音频采集（16kHz/16bit/单声道），PCM 数据通过 WebSocket 发送到服务端进行 ASR。不做 AEC、不做 barge-in。

### AI Agent
事件驱动的决策引擎，接收手势、告警、语音意图三种事件，通过规则策略决定执行动作。运行在专用线程中，32 槽位事件队列。

### guxian-control Skill
自定义 Skill，提供 `query_status`（查询系统状态）和 `execute_action`（执行操作）两个能力。安装在 `/data/agent/skills/guxian-control/`。

### 主动 + 执行场景
古贤系统检测异常 → WebSocket 推送告警 → Agent 提醒用户 → 用户做 OK 手势确认 → Skill 执行确认操作 → 返回结果 → 设备显示完成状态。

## 4. 系统架构

```
Camera / Mic / Touch
  |
  v
端侧感知 (Gesture Perception + Audio Capture)
  |
  v
事件生成 (Stable Gate → Gesture Event / Speech Intent)
  |
  v
Multimodal Context
  |
  v
AI Agent (事件队列 + 策略引擎)
  |
  v
guxian-control Skill (query_status / execute_action)
  |
  v
TaskManager
  |
  v
Guxian WebSocket (JSON / TLS)
  |
  v
古贤业务系统 (数字孪生 / 工业控制)
  |
  v
状态反馈 (Display / Audio)
```

## 5. Hardware

| 组件 | 规格 |
|------|------|
| 开发板 | ESP32-P4 Function-EV-Board V1.6 |
| 主芯片 | ESP32-P4, chip revision v3.2 |
| CPU | 双核 RISC-V, 400 MHz |
| PSRAM | 32 MB |
| 显示 | 1024x600 MIPI-DSI |
| 摄像头 | MIPI-CSI |
| 音频 | I2S 编解码器 (Mic + Speaker) |
| 连接 | Wi-Fi 6, BLE 5 |

## 6. OpenVela Integration

openvela 是基于 Apache NuttX 的开源 IoT 操作系统。VelaGesture 使用 openvela 提供：

- **RTOS 基础：** POSIX 线程、I/O、信号、时钟
- **设备驱动：** 摄像头 (`/dev/video0`)、音频 (`/dev/audio/*`)、显示 (`/dev/fb0`)
- **网络栈：** libwebsockets (WSS)
- **应用框架：** Kconfig + Makefile 构建系统
- **AI Agent：** `packages/ai_agent` 框架
- **NSH Shell：** 命令行交互

## 7. Build

```bash
# 1. 拉取工程
mkdir velaworkspace && cd velaworkspace
repo init -u https://github.com/open-vela/contest2026_439_biepengwopingmu \
  -b dev-ai-contest-2026 -m contest2026_439_biepengwopingmu.xml
repo sync -c -j8

# 2. 启用 VelaGesture
./build.sh vendor/espressif/boards/esp32p4/ev/configs/vela menuconfig
# → Application Configuration → Demos → [*] VelaGesture

# 3. 编译
./build.sh vendor/espressif/boards/esp32p4/ev/configs/vela -j8

# 4. 烧录
esptool.py --chip esp32p4 --port /dev/ttyUSB0 \
  write_flash 0x0 out/esp32p4-ev/vela/nuttx.bin
```

## 8. Run

```bash
# 连接串口
screen /dev/ttyUSB0 115200

# 启动应用
nsh> velagesture

# 预期输出
==========================================
  VelaGesture v1.0.0
  Multimodal Gesture Interaction Terminal
  Platform: ESP32-P4 + openvela
==========================================
[INIT] Application context ready
[INIT] Skill executor ready
[INIT] WebSocket client connected
[INIT] AI agent started
[INIT] Gesture perception started
[RUN] VelaGesture is running.
```

## 9. WebSocket Protocol

设备 → 服务端：
```json
{"type":"gesture","gesture":"ok","confidence":0.96,"stable":true,"timestamp":1726734600000}
{"type":"task","task_id":1,"skill":"guxian-control","action":"execute_action","payload":{...}}
{"type":"status","device_id":"p4-001","uptime_s":3600}
{"type":"error","code":-1,"message":"Camera init failed"}
```

服务端 → 设备：
```json
{"type":"alert","alert_id":"a-001","message":"Water level high","severity":2}
{"type":"task_result","task_id":1,"status":"success","result":{...}}
{"type":"ack","ref_type":"alert","ref_id":"a-001","status":"received"}
```

详见 `docs/PROTOCOL.md`。

## 10. Skill

`guxian-control` 是 VelaGesture 的自定义 Agent Skill。

安装位置：`/data/agent/skills/guxian-control/SKILL.md`

能力：
- `query_status` — 查询古贤系统设备/场景状态
- `execute_action` — 执行操作（确认告警、定位设备、切换场景等）

详见 `skills/guxian-control/SKILL.md`。

## 11. Active + Execute

完整主动执行场景：

1. 古贤系统检测到异常（水位超限）
2. WebSocket 推送告警到 P4 设备
3. AI Agent 存储告警并提示用户
4. 用户做 OK 手势确认
5. Agent 调用 guxian-control Skill
6. 古贤系统执行操作并返回结果
7. 设备显示任务完成状态

这个场景满足比赛对"主动 + 执行"的要求。

## 12. Security / Privacy

- **摄像头数据：** 原始帧在设备本地处理，不传输原始图像
- **网络传输：** 仅传输结构化事件（JSON）和必要音频（PCM），使用 TLS 加密
- **凭证管理：** 不在代码中存储 API Key、密码或 Token
- **仓库安全：** `.gitignore` 排除编译产物和敏感文件

## 13. AI Coding

本项目使用 AI Coding 工具辅助开发，涵盖以下环节：

- **需求拆解：** 将比赛要求分解为可执行的开发任务
- **方案设计：** 设计系统架构、模块划分、协议格式
- **编码：** 生成 C 源代码、构建系统文件、Skill 定义
- **调试：** 代码审查、静态分析、依赖检查
- **文档：** 生成 README、架构文档、协议文档、测试报告

AI 工具帮助在短时间内完成从架构设计到代码实现的全流程，特别是在嵌入式代码的 NuttX API 使用、构建系统配置和文档生成方面提供了显著效率提升。

完整日志见 `logs/` 目录。

## 14. License

```
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

---

## 目录结构

```
.
├── README.md                           # 本文件
├── app/
│   ├── velagesture/                    # 主应用
│   │   ├── include/                    # 头文件
│   │   ├── src/                        # 源文件
│   │   ├── CMakeLists.txt
│   │   ├── Makefile
│   │   ├── Make.defs
│   │   ├── Kconfig
│   │   └── README.md
│   └── hello_app/                      # 官方样例（保留）
├── board/
│   └── contest_board/                  # 官方样例（保留）
├── skills/
│   └── guxian-control/
│       └── SKILL.md                    # Skill 定义
├── docs/
│   ├── ARCHITECTURE.md                 # 系统架构
│   ├── PROTOCOL.md                     # WebSocket 协议
│   ├── BUILD.md                        # 编译指南
│   ├── DEMO_SCRIPT.md                  # 演示脚本
│   ├── TEST_REPORT.md                  # 测试报告
│   ├── PRIVACY_AND_SECURITY.md         # 隐私安全
│   ├── OPENVELA_INTEGRATION.md         # openvela 集成
│   ├── HARDWARE_ADAPTATION.md          # 硬件适配
│   ├── AI_AGENT.md                     # AI Agent 说明
│   └── RELEASE_CHECKLIST.md            # 发布检查清单
├── logs/                               # AI Coding 日志
├── submission/
│   ├── 作品介绍文档.md                  # 作品介绍源文件
│   ├── 作品介绍文档.pdf                 # 作品介绍 PDF
│   ├── 演示视频脚本.md                  # 视频脚本
│   └── 提交清单.md                      # 提交清单
├── contest2026_439_biepengwopingmu.xml # 团队 manifest
└── openvela.xml                        # openvela manifest
```
