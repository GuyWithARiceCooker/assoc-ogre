#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP="$ROOT/app/src/main"
BUILD="$ROOT/build"
DIST="$ROOT/dist"
SDK="${ANDROID_HOME:-/usr/lib/android-sdk}"
BUILD_TOOLS="$SDK/build-tools/debian"
ANDROID_JAR="$SDK/platforms/android-23/android.jar"
AAPT="$BUILD_TOOLS/aapt"
DX="$BUILD_TOOLS/dx"
ZIPALIGN="$BUILD_TOOLS/zipalign"
APKSIGNER="$BUILD_TOOLS/apksigner"
KEYSTORE="$BUILD/debug.keystore"

rm -rf "$BUILD"
mkdir -p "$BUILD/gen" "$BUILD/classes" "$DIST"

"$AAPT" package -f -m \
  -J "$BUILD/gen" \
  -S "$APP/res" \
  -M "$APP/AndroidManifest.xml" \
  -I "$ANDROID_JAR" \
  -F "$BUILD/projector-smoke-unsigned.apk" \
  -A "$APP/assets"

javac -source 8 -target 8 \
  -bootclasspath "$ANDROID_JAR" \
  -classpath "$ANDROID_JAR" \
  -d "$BUILD/classes" \
  $(find "$BUILD/gen" "$APP/java" -name '*.java' | sort)

"$DX" --dex --output="$BUILD/classes.dex" "$BUILD/classes"
(cd "$BUILD" && "$AAPT" add -f "$BUILD/projector-smoke-unsigned.apk" classes.dex >/dev/null)

"$ZIPALIGN" -f 4 "$BUILD/projector-smoke-unsigned.apk" "$BUILD/projector-smoke-aligned.apk"

if [[ ! -f "$KEYSTORE" ]]; then
  keytool -genkeypair -v \
    -keystore "$KEYSTORE" \
    -storepass android \
    -keypass android \
    -alias androiddebugkey \
    -keyalg RSA \
    -keysize 2048 \
    -validity 10000 \
    -dname "CN=Android Debug,O=assoc-ogre,C=HU" >/dev/null
fi

"$APKSIGNER" sign \
  --ks "$KEYSTORE" \
  --ks-pass pass:android \
  --key-pass pass:android \
  --out "$DIST/projector-smoke-debug.apk" \
  "$BUILD/projector-smoke-aligned.apk"

"$APKSIGNER" verify --verbose "$DIST/projector-smoke-debug.apk"
ls -lh "$DIST/projector-smoke-debug.apk"
