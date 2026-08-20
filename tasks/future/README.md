# После v1.0

- [ ] v1.1 Nordic Secure BLE DFU.
- [x] Аварийный Adafruit BLEDfu (buttonless OTA через bootloader XIAO) — обход
  мёртвого USB; не заменяет Secure DFU.
- [ ] v1.1 Стандартный Cycling Speed and Cadence GATT **peripheral** (Strava/Zwift).
- [x] BLE CSC **central/client** для CYCPLUS C3 (каденс) и S3 (скорость) —
  протокол 1.2, Arduino dual-role; ANT+ намеренно не используется (ADR-014).
  Zephyr central не реализован.
- [x] v1.1 Android-версия Flutter-приложения (release APK в CI/Release workflow).
- [ ] v1.2 Журнал поездок/GPS во внешней QSPI Flash.
- [ ] v1.2 Второй Hall-датчик каденса.
- [ ] v2.0 IMU: наклон и автоматическая подсветка.

Эти задачи не входят в DoD v1.0 и не должны расширять текущий scope без отдельного
решения о версии и обновления протокола.
