#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${ANDROID_JAR:?Set ANDROID_JAR to a real Android SDK platforms/android-35/android.jar or compatible API>=28}"
: "${D8:?Set D8 to Android SDK build-tools/<version>/d8 executable}"
OUT="$ROOT/build/java"
mkdir -p "$OUT/classes" "$OUT/dex"
javac -source 8 -target 8 -classpath "$ANDROID_JAR" -d "$OUT/classes" \
 "$ROOT/android/app/src/main/java/org/therealbatu/badlion/BadlionMenuBridge.java"
jar --create --file "$OUT/badlion-host-classes.jar" -C "$OUT/classes" .
"$D8" --min-api 26 --lib "$ANDROID_JAR" --output "$OUT/dex" "$OUT/badlion-host-classes.jar"
test -s "$OUT/dex/classes.dex"
echo "DEX created: $OUT/dex/classes.dex"
