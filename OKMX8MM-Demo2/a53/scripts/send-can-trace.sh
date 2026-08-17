#!/usr/bin/env sh
set -eu

usage() {
    cat <<'EOF'
Usage:
  sh scripts/send-can-trace.sh [--file can-trace.log] [--iface can0] [--dry-run] [--clear-on-success]
  sh scripts/send-can-trace.sh --env config/can.env [--dry-run]

Optional:
  CAN_IFACE       Default: can0
EOF
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEMO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
TRACE_FILE="$DEMO_ROOT/runtime-data/a53-can-trace.log"
ENV_FILE=
CAN_IFACE=${CAN_IFACE:-can0}
DRY_RUN=0
CLEAR_ON_SUCCESS=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        --file)
            [ "$#" -ge 2 ] || { usage; exit 2; }
            TRACE_FILE=$2
            shift 2
            ;;
        --iface)
            [ "$#" -ge 2 ] || { usage; exit 2; }
            CAN_IFACE=$2
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

[ -f "$TRACE_FILE" ] || { printf 'CAN trace file not found: %s\n' "$TRACE_FILE" >&2; exit 1; }
[ "$DRY_RUN" -eq 1 ] || command -v cansend >/dev/null 2>&1 || {
    printf 'cansend is required\n' >&2
    exit 1
}

byte_hex() {
    value=$1
    printf '%02X' "$((value & 255))"
}

word_le_hex() {
    value=$1
    value=$((value & 65535))
    byte_hex "$value"
    byte_hex "$((value / 256))"
}

line_to_frame() {
    line=$1
    direct_frame=$(printf '%s\n' "$line" | sed -n 's/.*frame=\([0-9A-Fa-f][0-9A-Fa-f]*#[0-9A-Fa-f][0-9A-Fa-f]*\).*/\1/p')
    if [ -n "$direct_frame" ]; then
        printf '%s\n' "$direct_frame"
        return 0
    fi

    can_id=$(printf '%s\n' "$line" | sed -n 's/.*id=0x\([0-9A-Fa-f][0-9A-Fa-f]*\).*/\1/p')
    seq=$(printf '%s\n' "$line" | sed -n 's/.*seq=\([0-9][0-9]*\).*/\1/p')
    ai0=$(printf '%s\n' "$line" | sed -n 's/.*ai0=\(-*[0-9][0-9]*\).*/\1/p')
    di=$(printf '%s\n' "$line" | sed -n 's/.*di=0x\([0-9A-Fa-f][0-9A-Fa-f]*\).*/0x\1/p')
    speed=$(printf '%s\n' "$line" | sed -n 's/.*speed_pulse=\([0-9][0-9]*\).*/\1/p')

    [ -n "$can_id" ] && [ -n "$seq" ] && [ -n "$ai0" ] && [ -n "$di" ] && [ -n "$speed" ] || return 1

    payload="$(word_le_hex "$seq")$(word_le_hex "$ai0")$(byte_hex "$di")$(word_le_hex "$speed")00"
    printf '%s#%s\n' "$can_id" "$payload"
}

count=0
while IFS= read -r line || [ -n "$line" ]; do
    [ -n "$line" ] || continue
    frame=$(line_to_frame "$line") || {
        printf 'Invalid CAN trace line: %s\n' "$line" >&2
        exit 1
    }

    if [ "$DRY_RUN" -eq 1 ]; then
        printf 'DRY RUN cansend %s %s\n' "$CAN_IFACE" "$frame"
    else
        cansend "$CAN_IFACE" "$frame"
    fi
    count=$((count + 1))
done < "$TRACE_FILE"

if [ "$CLEAR_ON_SUCCESS" -eq 1 ] && [ "$DRY_RUN" -eq 0 ]; then
    : > "$TRACE_FILE"
fi

printf 'Processed %s CAN frames\n' "$count"
