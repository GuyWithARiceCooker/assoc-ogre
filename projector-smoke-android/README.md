# Projector Smoke Android APK

Minimal Android WebView wrapper for the mobile projector/fog demo.

## Build

```bash
./projector-smoke-android/build-apk.sh
```

Output:

```text
projector-smoke-android/dist/projector-smoke-debug.apk
```

The APK is debug-signed and loads `app/src/main/assets/index.html` locally, so it does not need network access for the demo itself.
