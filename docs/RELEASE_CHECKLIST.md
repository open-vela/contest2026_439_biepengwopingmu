# VelaGesture — Release Checklist

## Pre-Submission Verification

- [x] README.md replaced with project-specific content
- [x] openvela integration documented and code uses NuttX APIs
- [x] AI agent implemented with event-driven architecture
- [x] LLM backend configuration documented (server-side)
- [x] Gesture recognition code: ok, fist, open_palm (plus victory, pinch)
- [x] Gesture Event JSON protocol defined and implemented
- [x] WebSocket client implemented (P4 ↔ Guxian)
- [x] guxian-control skill defined (SKILL.md with query_status + execute_action)
- [x] Active + Execute scenario fully described and coded
- [x] Audio path implemented (half-duplex capture + playback)
- [x] Apache 2.0 license headers on all source files
- [x] AI Coding logs directory prepared (logs/)
- [x] Submission document source created (submission/作品介绍文档.md)
- [x] Submission document PDF generated (submission/作品介绍文档.pdf)
- [x] Demo script created (submission/演示视频脚本.md)
- [x] No secrets (API keys, passwords, tokens) in repository
- [x] No fabricated benchmark numbers (FPS, latency, accuracy)
- [x] No image placeholders in documentation
- [x] No stale TODO comments (only implementation notes)
- [x] No "未完成" / "待完成" markers in user-facing docs
- [x] No redundant template boilerplate
- [x] No build artifacts committed (*.o, *.elf, build/)
- [x] No violation of official repo structure

## Documentation Checklist

- [x] docs/ARCHITECTURE.md
- [x] docs/PROTOCOL.md
- [x] docs/BUILD.md
- [x] docs/DEMO_SCRIPT.md
- [x] docs/TEST_REPORT.md
- [x] docs/PRIVACY_AND_SECURITY.md
- [x] docs/OPENVELA_INTEGRATION.md
- [x] docs/HARDWARE_ADAPTATION.md
- [x] docs/AI_AGENT.md
- [x] docs/RELEASE_CHECKLIST.md (this file)

## Code Checklist

- [x] app/velagesture/include/velagesture.h
- [x] app/velagesture/include/gesture_perception.h
- [x] app/velagesture/include/stable_gate.h
- [x] app/velagesture/include/ws_protocol.h
- [x] app/velagesture/include/ai_agent.h
- [x] app/velagesture/include/skill_executor.h
- [x] app/velagesture/include/audio_io.h
- [x] app/velagesture/src/main.c
- [x] app/velagesture/src/gesture_perception.c
- [x] app/velagesture/src/stable_gate.c
- [x] app/velagesture/src/ws_client.c
- [x] app/velagesture/src/ai_agent.c
- [x] app/velagesture/src/skill_executor.c
- [x] app/velagesture/src/audio_io.c
- [x] app/velagesture/CMakeLists.txt
- [x] app/velagesture/Makefile
- [x] app/velagesture/Make.defs
- [x] app/velagesture/Kconfig

## Skill Checklist

- [x] skills/guxian-control/SKILL.md (source copy)
- [x] Skill installable to /data/agent/skills/guxian-control/

## Submission Checklist

- [x] submission/作品介绍文档.md (source)
- [x] submission/作品介绍文档.pdf (generated)
- [x] submission/演示视频脚本.md

## Items Requiring Manual Action

These items require human action and cannot be completed by
automated tooling:

- [ ] Flash firmware to ESP32-P4 board
- [ ] Install skill file to /data/agent/skills/ on device
- [ ] Configure and run Guxian business system server
- [ ] Record demo video on real hardware
- [ ] Upload demo video to submission
- [ ] Create pull request on GitHub
- [ ] Self-review and merge pull request
- [ ] Sign CLA on openvela website
- [ ] Export AI Coding logs from session tool
- [ ] Submit logs to logs/ directory
