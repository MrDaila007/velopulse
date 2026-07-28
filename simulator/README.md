# Симулятор OLED BikeComp

Симулятор использует [u8g2-python-simulator](https://github.com/colinoflynn/u8g2-python-simulator)
как графический backend. Форматирование, координаты и последовательность команд рисования
не дублируются в Python: они компилируются напрямую из C++-модулей прошивки
`display_formatter.cpp` и `display_layout.cpp`. Python-файл только переводит полученные
команды в API upstream-симулятора.

При изменении C++-рендерера host-бинарник автоматически пересобирается. Поэтому кадры и
golden-тесты всегда проверяют текущий код прошивки, а не отдельно переписанный макет.

## Установка

```bash
./simulator/setup.sh
```

Требуются C++17-компилятор, Python 3, `venv`, Tk и `xvfb-run`. Python-зависимость Pillow
устанавливается в локальный каталог `simulator/.venv`.

## Интерактивный GUI

```bash
./simulator/run_gui.sh             # пять нижних страниц, смена раз в 4 секунды
./simulator/run_gui.sh trip
./simulator/run_gui.sh average
./simulator/run_gui.sh maximum
./simulator/run_gui.sh time
./simulator/run_gui.sh odometer
./simulator/run_gui.sh idle
./simulator/run_gui.sh paused
./simulator/run_gui.sh battery_unknown
./simulator/run_gui.sh low_battery
```

Скорость всегда остаётся в верхней зоне, батарея всегда видна справа сверху, меняется
только нижняя строка. Окно имеет реальные 128×32 пикселя с масштабом 6×. Клавиши
upstream-симулятора: `s` — PNG, `g` — запись GIF, `i` — инверсия.

## Headless-проверка

```bash
./simulator/render.sh --scenario trip --output trip.png
./simulator/render.sh --contact-sheet --output bikecomp-oled.png
./simulator/test.sh
```

Golden-тесты сверяют пять страниц карусели, состояния IDLE/PAUSE, неизвестный заряд
и предупреждение `LOW BATT` пиксель-в-пиксель. В `simulator/firmware_renderer.cpp`
находятся только входные тестовые состояния; сам интерфейс там не описывается.
