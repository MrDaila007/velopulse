# 11. Периферия и расчёты: обзор open-source проектов и фичи для BikeComp

Дата: 2026-08-08  
Статус: исследование для roadmap v1.1+

Документ фиксирует, что делают открытые велокомпьютеры и сопутствующие проекты в части
**подключения периферии** (датчики скорости, каденса, пульса, мощности, тренажёров) и
**расчёта метрик** на их основе. В конце — приоритизированный список фич, которые можно
внедрить в наш проект (VeloPulse / BikeComp на nRF52840).

---

## 1. Текущее состояние BikeComp (baseline)

| Аспект | Реализовано сейчас |
| --- | --- |
| Источник скорости | Локальный цифровой датчик Холла на колесе (GPIO + ISR) |
| Расчёт скорости | `speed_x100 = C_mm × 360000 / dt_us`, сглаживание по N интервалам |
| Фильтрация импульсов | Debounce, лимит по max speed, stuck detection (`PulseFilter`) |
| Коррекция интервалов | `SpeedIntervalGuard` — оценка пропущенных импульсов, подавление bounce |
| Внешние датчики | **Нет** (ANT+, BLE Central, HR, power, cadence — вне v1.0 по ТЗ) |
| BLE-роль | Peripheral: собственный GATT-сервис конфигурации и телеметрии |
| CSC Profile | Запланирован в v1.1 ([ADR-013](07-decisions-and-risks.md), [tasks/future](../tasks/future/README.md)) |

Это важно при сопоставлении: большинство open-source проектов решают **другую** задачу —
подключение *внешних* коммерческих датчиков, тогда как наш v1.0 — автономный компьютер
с собственным Hall-датчиком.

---

## 2. Обзор open-source проектов

### 2.1. Сводная таблица

| Проект | Платформа | Периферия | Роль BLE/ANT | Ключевые расчёты |
| --- | --- | --- | --- | --- |
| [hishizuka/pizero_bikecomputer](https://github.com/hishizuka/pizero_bikecomputer) | Raspberry Pi Zero | ANT+ (HR, SPD, CDC, PWR, lights, remote), частично BLE, I²C, GPS | ANT USB dongle; BLE — Zwift Click | Интеграция потоков, приоритеты источников, NP, W′, grade, autostop |
| [vincent290587/stravaV10](https://github.com/vincent290587/stravaV10) | nRF52840 | ANT+ (HRM, BSC, FEC, power), BLE (Komoot LNS) | Central + Peripheral (S332) | GPS speed, baro power estimate, Strava segments |
| [nikitaignatov/velometa](https://github.com/nikitaignatov/velometa) | ESP32 (dual-core) | BLE commercial sensors, I²C lab sensors, GPS UART | BLE Central к Assioma/Kicker/HR | Зоны HR/Power, W′, NP, CdA, timeseries logging |
| [spasoye/nrf52840_zephyr_CSC_sensor](https://github.com/spasoye/nrf52840_zephyr_CSC_sensor) | nRF52840 + Zephyr | Reed switch → wheel revs | **BLE Peripheral** (CSC Service) | Передаёт cumulative revs + event time; скорость считает клиент |
| [AlexeiTrofimov/Bike-Computer](https://github.com/AlexeiTrofimov/Bike-Computer) | ESP32 | Reed switch | BLE Peripheral (custom notify) | `speed = f(Δt)` между импульсами, как у нас |
| [skorbuly/CSC_BLE_PWR_Bridge](https://github.com/skorbuly/CSC_BLE_PWR_Bridge) | Android | ANT+ → BLE rebroadcast | Bridge: ANT reader, BLE Peripheral | Проксирует готовые ANT+ расчёты в стандартные BLE-профили |
| [eMadman/ble-ant-bridge](https://github.com/eMadman/ble-ant-bridge) | nRF52840 SuperMini | BLE Central → ANT+ FE-C | Bridge | Парсинг Cycling Power, трансляция в ANT trainer |
| [Nilogax/SmartBridge](https://github.com/Nilogax/SmartBridge) | nRF52840 + Android | eBike BLE → Garmin ANT/BLE | Bridge | Маппинг rider/motor power, cadence, LEV battery |
| [Svenbosma/DIY-Garmin-Powermeter](https://github.com/Svenbosma/DIY-Garmin-Powermeter) | nRF52 | Strain gauge + gyro | **BLE Peripheral** (Cycling Power) | P = 2×F×v, каденс по гироскопу, калибровка по BLE CP Control Point |
| [gilesp1729/BenderP](https://github.com/gilesp1729/BenderP) | Arduino 33 BLE | Hall (speed) + IMU | BLE Peripheral (CPS) | Opposing-force power без strain gauge |

### 2.2. Детали по подключению и расчётам

#### pizero_bikecomputer — эталон «полного» head unit

**Подключение:**
- ANT+ через USB-донгл и библиотеку [openant](https://github.com/hishizuka/openant).
- Пэйринг сохраняется в `setting.conf`; при старте — автоподключение к известным device ID.
- MultiScan для поиска новых датчиков.
- BLE: Zwift Click V2 (удалённое управление), Fake Trainer.
- I²C: барометр, акселерометр, гироскоп, магнитометр, освещённость.

**Расчёты (`modules/sensor_core.py`):**
- **Приоритет скорости:** ANT+ speed/BSC → ANT+ power page 0x11 (wheel) → GPS (с фильтром по accelerometer `m_stat`).
- **Каденс:** отдельный CDC-датчик, combined SPD&CDC, или страницы power meter 0x10/0x12.
- **Мощность:** страницы ANT+ 0x12 > 0x11 > 0x10 (crank предпочтительнее wheel).
- **Свежесть данных:** `time_threshold` на каждый тип — устаревшие значения не попадают в UI.
- **Дистанция:** инкремент из cumulative distance датчика; fallback на GPS при позднем подключении ANT+.
- **Продвинутые метрики:** normalized power, accumulated work (кДж), W′ balance, grade (distance- и speed-based), gross average speed для бреветов, autostop по порогу скорости + accelerometer moving status.
- **Скользящие средние:** HR и power за 3 / 30 / 60 с.

**Полезные паттерны для нас:**
- Единый `SensorCore` с приоритетами источников и freshness timeout.
- Отдельные delta/timestamp per ANT+ page для power profile.
- Graceful degradation: GPS дополняет ANT+, если датчик подключился позже.

#### stravaV10 — ближайший аналог по железу (nRF52840)

**Подключение:**
- SoftDevice S332: одновременно BLE и ANT+.
- ANT+: HRM, BSC, FE-C (smart trainer), cycling power.
- BLE: Komoot turn-by-turn (LNS), UART debug.

**Расчёты:**
- GPS — основной источник скорости на улице.
- Оценка мощности по барометру и алгоритму наклона (без power meter).
- Пользовательские FTP и вес.
- BLE Cycling Power Vector (pedal dynamics) — как у ANT+.

**Полезные паттерны:**
- Dual-radio scheduling на nRF52840 (если когда-либо добавим ANT+ USB или второй чип).
- Baro-based power estimation как fallback без датчика мощности.

#### velometa — тренировочный фокус, BLE Central

**Подключение:**
- BLE Central к коммерческим датчикам (Assioma, Kicker, HR strap, speed).
- Dual-core ESP32: Core0 — датчики и SD, Core1 — LVGL UI.
- Архитектура: Driver → Sensor abstraction → Filter → Metrics → UI.

**Расчёты:**
- Зоны HR и Power (FTP-based).
- Normalized Power, Training Load, W′, W′bal, HR drift (decoupling), CdA.
- Непрерывный timeseries всех датчиков; маркеры start/pause/end без потери данных.

**Полезные паттерны:**
- Слой `Metrics` поверх сырых измерений — хорошо ложится на наш `lib/domain/`.
- Идея multi-sensor для одной метрики (2 power meters) — для v2+.
- Запись timeseries даже без явного «старта поездки».

#### nrf52840_zephyr_CSC_sensor — стандартный BLE CSC (периферия-датчик)

**Подключение:**
- Reed switch, GPIO interrupt, 20 ms debounce в ISR.
- BLE CSC Measurement notify ~1 Гц.

**Расчёт (на стороне head unit / приложения):**
```text
speed_m_s = (wheel_circumference_m × Δcumulative_revs) / Δevent_time_s
```
где `event_time` — 16-bit, единица 1/1024 с, rollover каждые ~64 с.

**Полезные паттерны:**
- Стандартный формат данных вместо proprietary notify.
- Rollover handling обязателен при приёме CSC/ANT+ BSC.

#### DIY power meters (Svenbosma, BenderP, stevejarvis/powermeter)

Это **датчики**, а не head unit, но формулы и BLE-профили релевантны для приёма данных:

| Метрика | Формула / метод |
| --- | --- |
| Мощность (strain) | `P = 2 × F × v`, `v = ω × r` (угловая скорость коленвала × радиус) |
| Каденс (gyro) | Интеграция угловой скорости за оборот, deadband от phantom revs |
| Каденс (CPS/BLE) | Cumulative crank revolutions + last crank event time |
| Калибровка | BLE Cycling Power Control Point (tare), UART `t`/`c`/`m`, сохранение в flash |
| Сглаживание | Усреднение за оборот коленвала, отброс min/max выборок |

---

## 3. Стандартные формулы приёма периферии (ANT+ / BLE)

Общая модель для **Speed/Cadence** (ANT+ BSC, BLE CSC Service `0x1816`):

```text
Δt_s = (event_time_new - event_time_old) / 1024     // с учётом rollover 16-bit
Δrevs = rev_count_new - rev_count_old               // с учётом rollover 16/32-bit

speed_m_s   = (wheel_circumference_m × Δwheel_revs) / Δt_s
cadence_rpm = (60 × Δcrank_revs) / Δt_s
distance_m += wheel_circumference_m × Δwheel_revs
```

**Rollover (из Loghorn/ant-plus, ramunasd gist, Zephyr CSC sample):**
- `event_time`: при `new < old` → `new += 64 × 1024`
- `rev_count` (16-bit): при `new < old` → `new += 65536`
- `wheel_rev_count` (32-bit ANT+): реже, но обрабатывать аналогично

**Heart Rate (BLE HRS `0x180D`, ANT+ HRM):**
- Мгновенное значение bpm из пакета; опционально RR-intervals для HRV.
- Freshness: если нет пакета > 2–3 с → показывать «—» / last known с флагом stale.

**Cycling Power (BLE CPS `0x1818`, ANT+ 0x0B):**
- Instantaneous power (W) — прямое поле.
- Cadence часто в том же пакете (crank-based meters).
- Wheel speed — только на page 0x11 (wheel-based power).
- Accumulated energy (кДж) — интеграл для total work.
- Torque effectiveness, pedal smoothness — advanced pages / BLE vector.

---

## 4. Сравнение с нашей реализацией Hall-датчика

Наш локальный расчёт **математически эквивалентен** CSC при одном импульсе на оборот:

| | BikeComp (Hall) | BLE CSC / ANT+ BSC |
| --- | --- | --- |
| Вход | `dt_us` между импульсами | `Δrevs`, `Δevent_time` |
| Скорость | `C × 3.6 / dt_ms` | `C × Δrevs / Δt` |
| Дистанция | `+C` на каждый импульс | `+C × Δrevs` |
| Фильтрация | Debounce, overspeed, stuck, interval guard | Обычно на стороне датчика + rollover на приёме |
| Сглаживание | Среднее по N интервалам (гармоническое) | Зависит от head unit; часто EMA или instant |

**Что можно переиспользовать без изменения формул:**
- `PulseFilter` → адаптировать для software debounce BLE event stream.
- `SpeedIntervalGuard` → применять к `Δt` от внешнего CSC при пропуске notify.
- `SpeedCalculator` → обобщить до `RevolutionCalculator(circumference, Δrevs, Δt)`.

---

## 5. Фичи для внедрения в BikeComp

Приоритеты согласованы с [tasks/future](../tasks/future/README.md) и ADR-013.
Оценка сложности: **S** (малый инкремент), **M** (средний модуль), **L** (крупная подсистема).

### 5.1. P0 — v1.1, минимальный периферийный слой

| ID | Фича | Источник | Описание | Сложность | Зависимости |
| --- | --- | --- | --- | --- | --- |
| F-01 | **BLE CSC Peripheral** (опциональный GATT) | Zephyr CSC sample, ADR-013 | Публиковать wheel revs + event time как стандартный датчик для Strava/Zwift; наш Hall остаётся primary для UI | M | BLE stack capacity, `minor` протокола |
| F-02 | **Второй Hall — каденс** | tasks/future v1.2 | Второй GPIO + тот же `PulseFilter`/`SpeedCalculator` с `circumference` = 2πr коленвала (или 1 импульс/оборот) | M | Свободный GPIO, механика |
| F-03 | **Обобщённый `MotionSource`** | velometa architecture | Интерфейс: `local_hall`, `ble_csc`, `ant_bsc` → единый `speed_x100`, `cadence_rpm`, `sensor_state` | M | Нет |
| F-04 | **Rollover-safe delta calculator** | ant-plus, pizero, Zephyr | Host-тестируемый модуль `rev_event_delta.h`: 16/32-bit counters, 1/1024 s timestamps | S | F-03 |
| F-05 | **Freshness / stale timeout** | pizero `time_threshold` | Если внешний датчик молчит > N с — сбросить мгновенное значение, флаг `SENSOR_STALE` в телеметрии | S | F-03 |

### 5.2. P1 — v1.2, BLE Central (приём датчиков)

| ID | Фича | Источник | Описание | Сложность | Зависимости |
| --- | --- | --- | --- | --- | --- |
| F-10 | **BLE Central: CSC sensor** | velometa, CSC spec | Скан, bond, подписка на CSC Measurement; расчёт speed/cadence с `wheel_circumference_mm` из конфига | L | `CONFIG_BT_CENTRAL`, RAM, F-03, F-04 |
| F-11 | **BLE Central: Heart Rate** | pizero, flutter_antplus | HRS `0x180D`, отображение на карусели, телеметрия `hr_bpm` | M | F-10 infrastructure |
| F-12 | **BLE Central: Cycling Power** | velometa, Svenbosma CPS | CPS `0x1818`: power, cadence, accumulated kJ; страницы в телеметрии | M | F-10 |
| F-13 | **Sensor pairing UI** | pizero setting.conf, mobile app | Список сохранённых датчиков (MAC, тип, имя), scan/add/remove, автоподключение при старте | M | Mobile + protocol extension |
| F-14 | **Приоритет источников скорости** | pizero `sensor_core` | `local_hall > ble_csc > gps` (когда появится GPS); конфигурируемый приоритет | S | F-03 |
| F-15 | **Wheel circumference per source** | pizero, ANT+ libs | Разные C для локального Hall и внешнего CSC; не пересчитывать историческую дистанцию | S | Уже есть политика для config — расширить |

### 5.3. P2 — v1.3+, тренировочные метрики

| ID | Фича | Источник | Описание | Сложность | Зависимости |
| --- | --- | --- | --- | --- | --- |
| F-20 | **Зоны HR / Power** | velometa | FTP, max HR в конфиге; текущая зона на дисплее | M | F-11, F-12 |
| F-21 | **Normalized Power (30 s rolling)** | pizero | NP и avg power за 3/30/60 с | M | F-12 |
| F-22 | **Total work (кДж)** | pizero, ANT+ accumulated power | Интеграл мощности; совместимо с accumulated energy из CPS | S | F-12 |
| F-23 | **W′ и W′ balance** | pizero, velometa | CP и W′ в конфиге; расчёт баланса во время интервалов | M | F-21 |
| F-24 | **Autostop по порогу скорости** | pizero `autostop_cutoff` | Дополнение к pause по отсутствию импульсов: `speed < 4 km/h` N секунд | S | Уже есть RideState — расширить |
| F-25 | **Gross average speed (бревет)** | pizero | Целевая средняя + «выигранное/проигранное» время относительно лимита | S | TripComputer |

### 5.4. P3 — v2.0+, продвинутое / нишевое

| ID | Фича | Источник | Описание | Сложность | Зависимости |
| --- | --- | --- | --- | --- | --- |
| F-30 | **ANT+ через внешний модуль** | pizero, stravaV10 | nRF52840 не имеет ANT в одном чипе без S332; варианты: отдельный ANT chip, или мост-смартфон | L | Аппаратное решение |
| F-31 | **Baro power estimate** | stravaV10 | Мощность без датчика по барометру + скорости + массе | L | BMP280/BME280, калибровка |
| F-32 | **Opposing-force power (IMU)** | BenderP | Оценка мощности по IMU XIAO Sense + Hall speed | L | IMU fusion, калибровка на велосипеде |
| F-33 | **FE-C smart trainer control** | stravaV10, ble-ant-bridge | ERG mode, симуляция подъёма | L | ANT+ или BLE FTMS |
| F-34 | **BLE ↔ ANT bridge mode** | CSC_BLE_PWR_Bridge | Режим «ретранслятор» для совместимости с Garmin | L | Отдельный продуктовый scope |
| F-35 | **Timeseries log на Flash** | velometa | 1 Гц запись всех каналов; маркеры ride без явного start | L | QSPI / внешняя Flash (tasks/future v1.2) |
| F-36 | **Multi-sensor merge** | velometa | Два power meter: median или выбор primary | M | F-12 |
| F-37 | **Cycling Dynamics (vector)** | stravaV10 | Pedal smoothness, torque effectiveness | M | CPS advanced |

### 5.5. Инфраструктурные фичи (сквозные)

| ID | Фича | Источник | Описание | Сложность |
| --- | --- | --- | --- | --- |
| F-40 | **Sensor registry в NVS** | pizero `setting.conf` | A/B слоты для списка bonded peripherals (как config/odo) | M |
| F-41 | **Расширение BLE-протокола** | — | `SensorSlot` в Config/Telemetry: type, state, last_value, battery | M |
| F-42 | **Sensor test mode 5 Гц** | уже есть для Hall | Расширить на BLE-каналы: raw notify rate, RSSI, packet loss | S |
| F-43 | **Диагностика периферии** | pizero, наш `CMD_START_DIAGNOSTIC` | Битовая маска: CSC scan, HRM notify, CPS notify за N секунд | S |
| F-44 | **BLE scheduling / coexistence** | stravaV10 | При Central+Peripheral: интервалы conn, pause advertising during scan | M |

---

## 6. Рекомендуемый порядок внедрения

```text
v1.0 (сейчас)     Hall speed, custom BLE config
       │
       ▼
v1.1              F-01 CSC Peripheral (опционально)
                  F-03 MotionSource abstraction
                  F-04 Rollover calculator (host tests)
       │
       ▼
v1.2              F-02 Второй Hall (cadence)
                  F-10..F-15 BLE Central baseline
                  F-40 Sensor registry
       │
       ▼
v1.3              F-11 HRM, F-12 CPS
                  F-20..F-25 training metrics
       │
       ▼
v2.0+             F-30 ANT+, F-31..F-37 advanced
```

---

## 7. Риски и ограничения nRF52840

| Риск | Митигация |
| --- | --- |
| Один BLE-линк как Central + свой Peripheral GATT | Zephyr/SoftDevice поддерживают multi-role, но RAM и timing tight; тестировать на Super-nRF52840 |
| Нет native ANT+ на XIAO (S140) | ANT только через отдельный чип (Dynastream) или телефон-мост; stravaV10 использует S332 |
| Потребление при постоянном Central scan | Scan duty cycle; connect only to bonded; stop scan в deep sleep |
| Конфликт с 5-мин pairing window | Отдельное «окно добавления датчика» в mobile app |
| Расширение protocol/Config | Следовать [правилу protocol/](../README.ru.md#правило-работы-с-протоколом): сначала PR в `protocol/` |

---

## 8. Ссылки

| Проект | URL |
| --- | --- |
| pizero_bikecomputer | https://github.com/hishizuka/pizero_bikecomputer |
| stravaV10 | https://github.com/vincent290587/stravaV10 |
| velometa | https://github.com/nikitaignatov/velometa |
| nrf52840_zephyr_CSC_sensor | https://github.com/spasoye/nrf52840_zephyr_CSC_sensor |
| Zephyr peripheral_csc sample | https://github.com/zephyrproject-rtos/zephyr/blob/main/samples/bluetooth/peripheral_csc/src/main.c |
| CSC_BLE_PWR_Bridge | https://github.com/skorbuly/CSC_BLE_PWR_Bridge |
| ble-ant-bridge | https://github.com/eMadman/ble-ant-bridge |
| SmartBridge | https://github.com/Nilogax/SmartBridge |
| ant-plus (Node, BSC math) | https://github.com/Loghorn/ant-plus |
| DIY-Garmin-Powermeter | https://github.com/Svenbosma/DIY-Garmin-Powermeter |
| BenderP | https://github.com/gilesp1729/BenderP |
| BLE CSC Spec | https://www.bluetooth.com/specifications/specs/cycling-speed-and-cadence-service-1-0/ |
| BLE CPS Spec | https://www.bluetooth.com/specifications/specs/cycling-power-service-1-1/ |

---

## 9. Связанные документы проекта

- [03-firmware-architecture.md](03-firmware-architecture.md) — текущие расчёты Hall и фильтры
- [07-decisions-and-risks.md](07-decisions-and-risks.md) — ADR-013 (CSC в v1.1)
- [tasks/future/README.md](../tasks/future/README.md) — roadmap после v1.0
- [protocol/ble-protocol.md](../protocol/ble-protocol.md) — контракт v1.x
