# Установка BikeComp через TestFlight

TestFlight — официальный способ Apple для бета-тестирования iOS-приложений.
Сборка выполняется на macOS (локально или в GitHub Actions), затем IPA загружается
в App Store Connect и рассылается тестировщикам.

## Что понадобится

| Требование | Зачем |
| --- | --- |
| Apple Developer Program (99 USD/год) | Подпись приложения и доступ к TestFlight |
| App Store Connect | Управление сборками и тестировщиками |
| iPhone с iOS 13+ | Минимальная версия проекта |
| Приложение **TestFlight** из App Store | Установка бета-сборок |
| Apple ID тестировщика | Приглашение в группу тестирования |

Bundle ID приложения: `app.bikecomp.mobile`.

---

## 1. Подготовка в Apple Developer

### 1.1. Создайте App ID

1. Откройте [Apple Developer → Identifiers](https://developer.apple.com/account/resources/identifiers/list).
2. Нажмите **+** → **App IDs** → **App**.
3. Укажите:
   - **Description:** BikeComp Mobile
   - **Bundle ID:** `app.bikecomp.mobile` (Explicit)
4. Включите capability **Bluetooth** (если доступна в списке).
5. Сохраните.

### 1.2. Создайте приложение в App Store Connect

1. Откройте [App Store Connect → Apps](https://appstoreconnect.apple.com/apps).
2. **+** → **New App**.
3. Заполните:
   - **Platform:** iOS
   - **Name:** BikeComp
   - **Bundle ID:** `app.bikecomp.mobile`
   - **SKU:** `bikecomp-mobile` (любой уникальный идентификатор)
4. Сохраните.

### 1.3. Создайте Distribution Certificate

1. На Mac откройте **Keychain Access** → **Certificate Assistant** →
   **Request a Certificate From a Certificate Authority**.
2. Сохраните `.certSigningRequest`.
3. В [Certificates](https://developer.apple.com/account/resources/certificates/list)
   создайте **Apple Distribution** certificate.
4. Скачайте `.cer` и дважды кликните для установки в Keychain.
5. Экспортируйте сертификат + приватный ключ в `.p12` с паролем.

### 1.4. Создайте App Store provisioning profile

1. [Profiles](https://developer.apple.com/account/resources/profiles/list) → **+**.
2. Тип: **App Store Connect** (или **App Store**).
3. App ID: `app.bikecomp.mobile`.
4. Certificate: ваш Distribution certificate.
5. Скачайте `.mobileprovision`.

### 1.5. Создайте API-ключ App Store Connect (для CI)

1. [Users and Access → Keys](https://appstoreconnect.apple.com/access/api).
2. **+** → имя `github-actions-testflight`.
3. Роль: **App Manager** или **Developer**.
4. Скачайте `.p8` (один раз). Запишите **Key ID** и **Issuer ID**.

---

## 2. Секреты GitHub Actions

В репозитории: **Settings → Environments → ios-testflight → Environment secrets**.

| Secret | Значение |
| --- | --- |
| `IOS_CERTIFICATE_P12` | Файл `.p12` в Base64: `base64 -i cert.p12 \| pbcopy` |
| `IOS_CERTIFICATE_PASSWORD` | Пароль от `.p12` |
| `IOS_PROVISIONING_PROFILE` | Файл `.mobileprovision` в Base64 |
| `APPSTORE_ISSUER_ID` | Issuer ID из App Store Connect |
| `APPSTORE_API_KEY_ID` | Key ID API-ключа |
| `APPSTORE_API_PRIVATE_KEY` | Содержимое файла `.p8` (включая `BEGIN/END`) |

Без настроенного environment `ios-testflight` workflow не запустится с подписью.
Для проверки компиляции без Apple-аккаунта используйте job **Mobile iOS** в CI
(собирает unsigned `.app`).

---

## 3. Сборка и загрузка в TestFlight

### Вариант A: GitHub Actions (рекомендуется)

Workflow: `.github/workflows/ios-testflight.yml`.

Перед первым запуском создайте GitHub Environment `ios-testflight` и добавьте в него
секреты из таблицы ниже (Settings → Environments → ios-testflight).

**Автоматически при теге:**

```bash
git tag v0.1.0
git push origin v0.1.0
```

**Вручную:**

1. GitHub → **Actions** → **iOS TestFlight** → **Run workflow**.
2. Дождитесь зелёного статуса. IPA сохраняется в Artifacts (400 дней) и
   автоматически загружается в TestFlight.

### Вариант B: Локально на Mac

```bash
# Откройте ios/Runner.xcworkspace в Xcode и выберите Team + Automatic Signing
# либо настройте Manual signing с Distribution profile.

./tool/build-ios.sh
```

Загрузите IPA одним из способов:

- приложение **Transporter** (Mac App Store);
- Xcode → **Window → Organizer** → **Distribute App**;
- `xcrun altool --upload-app -f build/ios/ipa/*.ipa ...`

---

## 4. Добавление тестировщиков

1. App Store Connect → ваше приложение → **TestFlight**.
2. Дождитесь статуса сборки **Ready to Test** (обычно 5–30 минут после загрузки).
3. При первой загрузке заполните **Export Compliance**:
   - приложение использует стандартное шифрование (HTTPS/TLS) → **No** для
     custom encryption.
4. **Internal Testing** — до 100 пользователей с ролью в вашей команде Developer.
5. **External Testing** — до 10 000 внешних тестировщиков; требуется короткий
   Beta App Review (обычно 24–48 ч).

Добавьте email тестировщика в группу. Apple отправит приглашение.

---

## 5. Установка на iPhone

1. Установите **TestFlight** из App Store (если ещё не установлен).
2. Откройте письмо-приглашение на iPhone или перейдите по ссылке из письма.
3. Нажмите **View in TestFlight** / **Принять** → **Установить**.
4. При первом запуске BikeComp разрешите доступ к **Bluetooth** — без этого
   поиск велокомпьютера не работает.
5. Держите iPhone рядом с XIAO nRF52840 и выполните pairing по инструкции
   в приложении.

### Обновление бета-версии

Когда вы загрузите новую сборку в TestFlight, приложение **TestFlight**
предложит обновление. Внутренние тестировщики получают сборки сразу после
обработки Apple; внешние — после одобрения Beta Review.

### Типичные проблемы

| Симптом | Решение |
| --- | --- |
| «Unable to Install» | Проверьте, что UDID устройства в Development profile (для ad-hoc) или используйте App Store profile + TestFlight |
| Сборка «Expired» | TestFlight-билды действуют 90 дней; загрузите новую |
| Нет приглашения | Проверьте спам; email должен совпадать с Apple ID |
| Bluetooth не работает | Settings → BikeComp → Bluetooth → Allow; перезапустите приложение |
| CI: `no signing certificate` | Проверьте `IOS_CERTIFICATE_P12` и пароль |

---

## 6. Проверка без TestFlight (разработка)

На Mac с подключённым iPhone:

```bash
cd mobile-app
open ios/Runner.xcworkspace
# В Xcode выберите устройство и Run (⌘R)

# Или из терминала:
../tool/flutterw run --dart-define=BIKECOMP_FAKE_BLE=true
```

`BIKECOMP_FAKE_BLE=true` включает эмуляцию BLE без физического велокомпьютера.
