#!/usr/bin/env bash
set -euo pipefail
PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME to android-ndk-r28c}" 
: "${PRELOADER_SO:?Set PRELOADER_SO to matching libpreloader.so from LeviLaunchroid 1.6.1}"
TOOL="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64"
python3 "$PROJECT/web/src/build_web.py"
python3 "$PROJECT/tools/dex/generate_minimal_dex.py"
mkdir -p "$PROJECT/build/arm64-v8a"
"$TOOL/bin/clang++" --target=aarch64-linux-android26 --sysroot="$TOOL/sysroot" \
 -std=c++20 -Os -fno-exceptions -fno-rtti -fPIC -fvisibility=hidden -shared \
 -Wl,-soname,libbadlionmenuui.so -Wl,--no-undefined -Wl,--build-id=sha1 \
 -nostdlib++ -unwindlib=none \
 -I"$PROJECT/include" -I"$PROJECT/sdk_compat" -I"$PROJECT/src" \
 "$PROJECT/src/BadlionMenu.cpp" "$PROJECT/src/AndroidAutoHost.cpp" "$PRELOADER_SO" \
 "$TOOL/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" \
 -llog -ldl -lm -o "$PROJECT/build/arm64-v8a/libbadlionmenuui.so"
file "$PROJECT/build/arm64-v8a/libbadlionmenuui.so"
