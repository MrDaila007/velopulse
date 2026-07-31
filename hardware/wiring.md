# BikeComp wiring notes

Обновлено: 2026-08-01

## LDR auto-brightness divider (task 1.10)

```text
D3/P0.29 (LIGHT_POWER) → LDR → ADC_NODE → R_FIXED → GND
                                ├── D2/A2/P0.28 (LIGHT_ADC)
                                └── 100 nF → GND
```

| Сигнал | Пин | Назначение |
| --- | --- | --- |
| LIGHT_POWER | D3 | Коммутируемое питание LDR (HIGH только при выборке) |
| LIGHT_ADC | D2 / A2 | Узел делителя, 12-bit SAADC |

### Сборка

1. Начальный `R_FIXED` = **22 kΩ** (допустимы 10 kΩ или 47 kΩ после калибровки).
2. Установить **100 nF** между ADC_NODE и GND рядом с LDR.
3. Разместить LDR снаружи корпуса под козырьком; избегать засветки от OLED.
4. Проверить мультиметром: при отключённом D3 ток через делитель ≈ 0.

### Калибровка

1. Прошить production firmware (`pio run -e xiao_ble_sense -t upload`).
2. Открыть Serial 115200 и отправить `ambient-raw` (или `tools/ldr_calibrate.py`).
3. Записать `raw` в трёх условиях:
   - **dark** — LDR полностью закрыт
   - **room** — комнатное освещение
   - **bright** — яркий свет / улица
4. Цель: полезный диапазон ~100–3900; `raw_bright > raw_dark`.
5. Если диапазон узкий — заменить `R_FIXED` на 10 или 47 kΩ и повторить.
6. Обновить `firmware/platformio.ini`:

```ini
-DBIKECOMP_AMBIENT_RAW_DARK=<dark>
-DBIKECOMP_AMBIENT_RAW_BRIGHT=<bright>
```

7. Перепрошить и проверить плавность пяти уровней, caps 10/60/100% и ток ≤ 20 µA.

### Без LDR

Отключённая цепь даёт `raw=0`, `valid=0`; яркость возвращается к пользовательскому
максимуму (`brightness_pct`).

## Battery divider (verified)

```text
BAT+ → 1 MΩ → P0.31 → 1 MΩ → GND
```

## OLED (verified)

| Сигнал | Пин |
| --- | --- |
| SDA | D4 |
| SCL | D5 |
| VCC | 3V3 |
| GND | GND |

I²C address `0x3C`.

## Hall sensor (pending)

| Сигнал | Пин |
| --- | --- |
| HALL_OUT | D0 |
| VCC | 3V3 |
| GND | GND |
