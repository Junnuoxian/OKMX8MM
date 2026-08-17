# OKMX8MM Demo2 80% Completion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create a self-contained `OKMX8MM-Demo2` delivery folder that follows the STM32F407 reference workflow and is ready for M4/A53 software integration before real hardware validation.

**Architecture:** M4 keeps only acquisition responsibilities and emits a stable batch format. A53 consumes the batch, stores it for SD-card deployment, prepares MQTT cloud messages, prepares CAN frames for the truck, and exposes heartbeat/configuration/OTA-readiness hooks. The original projects remain as references; Demo2 is the learner-facing delivery folder.

**Tech Stack:** C, CMake, PowerShell, POSIX shell, NXP i.MX8MM M4 SDK template, Linux A53 userspace, JSONL test outputs, Git.

## Global Constraints

- Treat `D:\Codex_AI\YY_Demo\China-STM32F407-ATK-EXPLORER` as the primary functional reference.
- Keep M4 limited to analog/digital acquisition and batch framing.
- Keep A53 responsible for SD storage, MQTT upload preparation, CAN transmission preparation, heartbeat, configuration, and OTA readiness.
- Do not claim real SD, cloud, CAN bus, external acquisition board, or M4-A53 communication has been hardware-tested.
- Keep all learner-facing deliverables inside `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2`.
- Manuals and tutorials must not use the Chinese word `地址`.

---

### Task 1: Create the self-contained Demo2 delivery tree

**Files:**
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\README.md`
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\docs\`
- Copy: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-M4-demo1\` into `OKMX8MM-Demo2\m4\`
- Copy: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-A53-demo\` into `OKMX8MM-Demo2\a53\`

**Interfaces:**
- Produces one entry folder with independent `m4`, `a53`, and `docs` sections.
- The copied A53 and M4 source trees retain their existing build/test commands.

- [ ] Step 1: Verify the source repositories are clean and identify ignored generated directories.
- [ ] Step 2: Copy source trees without copying build, runtime, package, or private configuration output.
- [ ] Step 3: Add the Demo2 root README with the data-flow diagram and newcomer entry points.
- [ ] Step 4: Run a file inventory and confirm every learner-facing document is under Demo2.

### Task 2: Add the STM32-to-OKMX8MM functional migration map

**Files:**
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\docs\stm32-to-okmx8mm.md`
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\docs\newcomer-runbook.txt`

**Interfaces:**
- Documents the mapping:
  - STM32 `adc` -> M4 acquisition input
  - STM32 `adc-storage` -> A53 SD writer
  - STM32 `mqtt` -> A53 MQTT publisher
  - STM32 `can` -> A53 CAN sender
  - STM32 `heart-beat` -> A53 heartbeat
  - STM32 `ota` -> A53 OTA readiness and later updater
- Uses field names `ai0` through `ai9`, `di_bits`, `speed_pulse_delta`, and `speed_period_us`.

- [ ] Step 1: Extract the reference module list from the STM32 project.
- [ ] Step 2: Write the OKMX8MM responsibility split and data flow.
- [ ] Step 3: Write a direct newcomer procedure from PC simulation to board smoke test.
- [ ] Step 4: Check learner-facing documents for the forbidden Chinese word.

### Task 3: Add A53 heartbeat and OTA-readiness outputs

**Files:**
- Modify: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53\src\`
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53\config\ota.env.example`
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53\scripts\check-ota-readiness.sh`
- Test: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53\tests\`

**Interfaces:**
- Each successful batch updates the existing status record and writes a heartbeat JSONL record.
- OTA readiness checks only validate version/package metadata and never install or replace a running program.
- Existing storage, MQTT outbox, and CAN trace behavior remains unchanged.

- [ ] Step 1: Add failing tests for heartbeat record creation and OTA metadata validation.
- [ ] Step 2: Run the focused tests and confirm they fail for the missing behavior.
- [ ] Step 3: Implement minimal heartbeat output using the existing status/configuration path.
- [ ] Step 4: Implement the non-destructive OTA readiness script.
- [ ] Step 5: Run focused tests and the existing Windows test suite.

### Task 4: Add a real-input adapter boundary for M4-to-A53 integration

**Files:**
- Modify: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53\src\m4_file_source.c`
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53\scripts\read-m4-serial.sh`
- Modify: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\m4\demo1\src\`
- Test: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53\tests\`

**Interfaces:**
- Keep `a53_m4_batch_t` stable.
- Accept one complete CSV batch per line from a serial/RPMsg adapter.
- Reject malformed lines and sequence gaps without writing inconsistent downstream records.

- [ ] Step 1: Add failing parser tests for valid, malformed, and skipped-sequence batches.
- [ ] Step 2: Run the focused parser tests and confirm the expected failures.
- [ ] Step 3: Implement the adapter boundary without changing downstream storage/MQTT/CAN consumers.
- [ ] Step 4: Add the shell usage example for a UART or RPMsg character device.
- [ ] Step 5: Run parser, Windows integration, and deliverable checks.

### Task 5: Package and verify Demo2

**Files:**
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\scripts\package-demo2.ps1`
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\scripts\check-demo2.ps1`
- Create: `D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\docs\acceptance-checklist.txt`
- Update: `D:\Codex_AI\YY_Demo\OKMX8MM\README.md`

**Interfaces:**
- The package contains source, docs, examples, and scripts but excludes generated build/runtime output and private credentials.
- The self-check reports source presence, required documents, test results, and forbidden-word scan.

- [ ] Step 1: Add the packaging script with an explicit exclusion list.
- [ ] Step 2: Add the self-check script and acceptance checklist.
- [ ] Step 3: Run all Windows tests and the Demo2 self-check.
- [ ] Step 4: Generate `OKMX8MM-Demo2.zip`.
- [ ] Step 5: Inspect Git status, commit the completed Demo2 deliverable, and push it to the existing remote.
