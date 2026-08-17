#!/bin/sh
set -eu

CONFIG_FILE="${1:-config/ota.env.example}"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "missing config: $CONFIG_FILE" >&2
    exit 1
fi

. "$CONFIG_FILE"

if [ -z "${DEMO2_DEVICE_MODEL:-}" ]; then
    echo "missing DEMO2_DEVICE_MODEL" >&2
    exit 1
fi

if [ -z "${DEMO2_CURRENT_VERSION:-}" ]; then
    echo "missing DEMO2_CURRENT_VERSION" >&2
    exit 1
fi

if [ -z "${DEMO2_PACKAGE_FILE:-}" ]; then
    echo "missing DEMO2_PACKAGE_FILE" >&2
    exit 1
fi

if [ -z "${DEMO2_EXPECTED_SHA256:-}" ]; then
    echo "missing DEMO2_EXPECTED_SHA256" >&2
    exit 1
fi

echo "ota readiness ok"
echo "model=$DEMO2_DEVICE_MODEL"
echo "version=$DEMO2_CURRENT_VERSION"
echo "package=$DEMO2_PACKAGE_FILE"

