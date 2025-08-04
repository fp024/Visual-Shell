#!/bin/sh

# 빌드 디렉토리 생성
mkdir -p build

# 컴파일
gcc -g -W -Wall -o build/visual-shell src/main.c -lncursesw -lmenuw -lpanelw

if [ $? -eq 0 ]; then
    echo "[v] Build successful: build/visual-shell"
    echo "Run with: ./build/visual-shell"
else
    echo "[x] Build failed"
    exit 1
fi