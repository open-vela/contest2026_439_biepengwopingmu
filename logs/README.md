# logs/ — AI Coding 日志目录

存放开发中与 AI 工具的对话日志，和作品代码一并提交。

## 当前状态

目录结构已建立。日志文件需要使用官方工具从 AI Coding 会话中导出。

## 导出步骤

### 方法一：使用 contest-snapshot（推荐）

在完整的 openvela workspace 中执行：

```bash
# 查看可用会话
contest-snapshot --list

# 导出当天会话
contest-snapshot --today --confirm

# 如需补采历史会话
contest-snapshot --backfill
```

### 方法二：手动导出

如果 contest-snapshot 不可用，可以从 Claude Code 会话中手动导出对话日志。

## 目录结构

```text
logs/
├── manifest.json                    # 会话清单
└── 21CHAPPiE/                       # GitHub 用户名
    └── 2026-09-19/                  # 日期 YYYY-MM-DD
        └── claude-code__<sid>.jsonl # 会话文件
```

- `<sid>`：session id
- 每个 `.jsonl` 每行一个事件

## 验证

导出后运行官方验证脚本：

```bash
python3 ../.claude/skills/contest-log-collector/tools/validate-log.py logs/
```

## 重要

- 不要手工编造日志内容
- 不要修改已有日志正文
- 不要把 API Key、密码等敏感信息写入日志
