#!/usr/bin/env sh
set -eu

pass() {
    printf '[OK] %s\n' "$1"
}

warn() {
    printf '[WARN] %s\n' "$1"
}

fail() {
    printf '[FAIL] %s\n' "$1"
    exit 1
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEMO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

printf 'OKMX8MM A53 demo board check\n'
printf 'arch: %s\n' "$(uname -m)"

command -v cmake >/dev/null 2>&1 && pass 'cmake found' || warn 'cmake not found'
command -v cc >/dev/null 2>&1 && pass 'C compiler found' || warn 'C compiler not found'

mkdir -p "$DEMO_ROOT/runtime-data" || fail 'cannot create runtime-data'
if [ -w "$DEMO_ROOT/runtime-data" ]; then
    pass 'runtime-data is writable'
else
    fail 'runtime-data is not writable'
fi

if command -v ip >/dev/null 2>&1; then
    if ip link show can0 >/dev/null 2>&1; then
        pass 'can0 found'
    else
        warn 'can0 not found'
    fi
else
    warn 'ip command not found'
fi

if [ -d /mnt/sdcard ] && [ -w /mnt/sdcard ]; then
    pass '/mnt/sdcard is writable'
else
    warn '/mnt/sdcard is not ready'
fi

command -v jq >/dev/null 2>&1 && pass 'jq found' || warn 'jq not found'
command -v mosquitto_pub >/dev/null 2>&1 && pass 'mosquitto_pub found' || warn 'mosquitto_pub not found'

warn 'MQTT publish requires MQTT_HOST and publish-mqtt-outbox.sh'
warn 'CAN is still trace file mode in this demo'
printf 'check finished\n'
