#!/usr/bin/env sh
set -eu

usage() {
    cat <<'EOF'
Usage:
  MQTT_HOST=host sh scripts/publish-mqtt-outbox.sh [--file outbox.jsonl] [--dry-run] [--clear-on-success]
  sh scripts/publish-mqtt-outbox.sh --env config/mqtt.env [--dry-run]

Required:
  MQTT_HOST       MQTT server name or IP

Optional:
  MQTT_PORT       Default: 1883
  MQTT_USER       MQTT user name
  MQTT_PASSWORD   MQTT password
EOF
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEMO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
OUTBOX_FILE="$DEMO_ROOT/runtime-data/a53-mqtt-outbox.jsonl"
ENV_FILE=
DRY_RUN=0
CLEAR_ON_SUCCESS=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --file)
            [ "$#" -ge 2 ] || { usage; exit 2; }
            OUTBOX_FILE=$2
            shift 2
            ;;
        --env)
            [ "$#" -ge 2 ] || { usage; exit 2; }
            ENV_FILE=$2
            shift 2
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --clear-on-success)
            CLEAR_ON_SUCCESS=1
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            usage
            exit 2
            ;;
    esac
done

if [ -n "$ENV_FILE" ]; then
    [ -f "$ENV_FILE" ] || { printf 'Env file not found: %s\n' "$ENV_FILE" >&2; exit 1; }
    set -a
    # shellcheck disable=SC1090
    . "$ENV_FILE"
    set +a
fi

[ -n "${MQTT_HOST:-}" ] || { printf 'MQTT_HOST is required\n' >&2; exit 2; }
command -v jq >/dev/null 2>&1 || { printf 'jq is required\n' >&2; exit 1; }
[ "$DRY_RUN" -eq 1 ] || command -v mosquitto_pub >/dev/null 2>&1 || {
    printf 'mosquitto_pub is required\n' >&2
    exit 1
}
[ -f "$OUTBOX_FILE" ] || { printf 'Outbox file not found: %s\n' "$OUTBOX_FILE" >&2; exit 1; }

MQTT_PORT=${MQTT_PORT:-1883}
if [ -n "${MQTT_USER:-}" ] || [ -n "${MQTT_PASSWORD:-}" ]; then
    [ -n "${MQTT_USER:-}" ] && [ -n "${MQTT_PASSWORD:-}" ] || {
        printf 'MQTT_USER and MQTT_PASSWORD must be set together\n' >&2
        exit 2
    }
fi

count=0
while IFS= read -r line || [ -n "$line" ]; do
    [ -n "$line" ] || continue
    topic=$(printf '%s' "$line" | jq -er '.topic')
    payload=$(printf '%s' "$line" | jq -ec '.payload')

    if [ "$DRY_RUN" -eq 1 ]; then
        printf 'DRY RUN topic=%s payload=%s\n' "$topic" "$payload"
    elif [ -n "${MQTT_USER:-}" ]; then
        mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -u "$MQTT_USER" -P "$MQTT_PASSWORD" -t "$topic" -m "$payload"
    else
        mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" -t "$topic" -m "$payload"
    fi
    count=$((count + 1))
done < "$OUTBOX_FILE"

if [ "$CLEAR_ON_SUCCESS" -eq 1 ] && [ "$DRY_RUN" -eq 0 ]; then
    : > "$OUTBOX_FILE"
fi

printf 'Processed %s MQTT messages\n' "$count"
