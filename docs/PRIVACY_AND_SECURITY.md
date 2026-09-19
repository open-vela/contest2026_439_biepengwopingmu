# VelaGesture — Privacy and Security

## Data Handling Principles

### 1. Camera Data — Local Processing

Raw camera frames are processed on-device.  The frame buffer is
allocated in PSRAM, used for gesture inference, and overwritten on
the next capture cycle.  Frames are never transmitted over the
network in their raw form.

Only structured gesture events (gesture name, confidence,
timestamp) are sent to the server.

### 2. Audio Data — Server-Side ASR

When audio capture is enabled, PCM frames are sent to the server
for speech recognition.  The audio data is:
- Transmitted over a TLS-encrypted WebSocket connection.
- Processed in real-time and not stored on the server beyond the
  ASR processing window.
- Never persisted on the device beyond the capture buffer.

### 3. Network Communication

All WebSocket communication uses TLS (WSS).  The device does not
send:
- Raw camera frames
- Unencrypted credentials
- Device-identifying information beyond what is necessary for the
  protocol (task IDs, device status)

### 4. Credential Management

- No API keys, passwords, or access tokens are stored in the
  source code.
- WebSocket server address and port are configured via Kconfig
  and can be changed at build time.
- TLS certificates (if used) are expected to be provisioned
  separately and stored in the device's secure storage.
- The `.gitignore` excludes common secret file patterns.

### 5. Skill Data

Skill definitions (SKILL.md) are read-only configuration files.
They do not contain credentials.  Skill execution results are
transient and not persisted beyond the current session.

## Commit Checklist

Before committing to the repository, verify:
- [ ] No API keys in any file
- [ ] No passwords in any file
- [ ] No access tokens in any file
- [ ] No private keys or certificates
- [ ] No hardcoded IP addresses (except localhost examples)
- [ ] No personal chat logs or conversations
- `.gitignore` is configured to exclude:
  - `build/`, `out/` (build artifacts)
  - `*.pem`, `*.key` (credentials)
  - `.env` (environment files)
