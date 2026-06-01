#!/usr/bin/env bash
set -e

if command -v sudo >/dev/null 2>&1; then SUDO=sudo; else SUDO=; fi

$SUDO apt-get update -qq
$SUDO apt-get install -y --no-install-recommends g++ make

g++ --version >/dev/null 2>&1 || { echo "ERROR: g++ not available after setup" >&2; exit 1; }
