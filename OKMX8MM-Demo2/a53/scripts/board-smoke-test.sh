#!/usr/bin/env sh
set -eu

pass() {
    printf '[OK] %s\n' "$1"
}

fail() {
    printf '[FAIL] %s\n' "$1" >&2
    exit 1
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEMO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
APP="$DEMO_ROOT/build-linux/okmx8mm-a53-demo"
RUNTIME_DIR="$DEMO_ROOT/runtime-data"
STORAGE_FILE="$RUNTIME_DIR/a53-storage.jsonl"
MQTT_FILE="$RUNTIME_DIR/a53-mqtt-outbox.jsonl"
CAN_FILE="$RUNTIME_DIR/a53-can-trace.log"
STATUS_FILE="$RUNTIME_DIR/a53-status.json"

cd "$DEMO_ROOT"

sh scripts/check-board-env.sh
sh scripts/build-linux.sh

[ -x "$APP" ] || fail 'build-linux/okmx8mm-a53-demo not found'

mkdir -p "$RUNTIME_DIR"
rm -f "$STORAGE_FILE" "$STORAGE_FILE.cursor" "$MQTT_FILE" "$CAN_FILE" "$STATUS_FILE"

"$APP" --cycles 3 \
    --storage "$STORAGE_FILE" \
    --mqtt-outbox "$MQTT_FILE" \
    --can-trace "$CAN_FILE" \
    --status "$STATUS_FILE"

"$APP" --check-storage "$STORAGE_FILE"

[ -s "$STORAGE_FILE" ] || fail 'storage file is empty'
[ -s "$MQTT_FILE" ] || fail 'MQTT outbox file is empty'
[ -s "$CAN_FILE" ] || fail 'CAN trace file is empty'
[ -s "$STATUS_FILE" ] || fail 'status file is empty'

grep '"ok":true' "$STATUS_FILE" >/dev/null 2>&1 || fail 'status is not ok'
grep '"processed_batches":3' "$STATUS_FILE" >/dev/null 2>&1 || fail 'processed batch count is not 3'
grep 'frame=321#' "$CAN_FILE" >/dev/null 2>&1 || fail 'CAN frame not found'

cat "$STATUS_FILE"
pass 'board smoke test passed'
