#!/usr/bin/env bash
# Установка зависимостей сборки. Вызывается грейдером один раз.
set -euo pipefail

if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update -y || apt-get update -y || true
    sudo apt-get install -y g++ python3-pip pkg-config libgtest-dev \
        || apt-get install -y g++ python3-pip pkg-config libgtest-dev || true
fi

# meson + ninja через pip (надёжнее, чем системные пакеты).
python3 -m pip install --user --upgrade meson ninja || pip3 install --user --upgrade meson ninja

echo "setup.sh: done"
