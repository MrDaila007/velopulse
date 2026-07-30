# Локальный mobile toolchain

`bootstrap-mobile.sh` разворачивает в игнорируемую `.tooling/` проверенный набор:

- Flutter 3.44.7 / Dart 3.12.2;
- Eclipse Temurin JDK 17.0.20+8;
- Android Command-Line Tools `15859902`;
- Android Platforms 33/35/36, Build Tools 36.0.0, Platform Tools, NDK
  28.2.13676358 и CMake 3.22.1.

Архивы Flutter, JDK и Command-Line Tools закреплены URL и SHA-256. Нужны `curl`,
`tar`, `unzip` и доступ к официальным репозиториям Google/GitHub.

```bash
./tool/bootstrap-mobile.sh
./tool/flutterw doctor -v
```

`flutterw` всегда использует локальные JDK и Android SDK. Нестандартный SDK можно
передать только через `BIKECOMP_ANDROID_SDK_ROOT`. Команды запускаются из `mobile-app/` либо с
путём к проекту:

```bash
cd mobile-app
../tool/flutterw pub get
../tool/flutterw pub run build_runner build
../.tooling/flutter/bin/dart format --set-exit-if-changed lib test
../tool/flutterw analyze
../tool/flutterw test
../tool/flutterw build apk --debug
../tool/flutterw build apk --release
```
