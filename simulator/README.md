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
./simulator/run_gui.sh                 # demo, 128×64
./simulator/run_gui.sh trip            # trip, 128×64
./simulator/run_gui.sh trip 32         # trip, совместимый 128×32
./simulator/run_gui.sh low_battery 64
```

По умолчанию GUI открывает основную геометрию 128×64. Второй позиционный аргумент
выбирает высоту 32 или 64; первый по-прежнему задаёт scenario. Скорость остаётся
постоянной, а меняется нижняя строка. Масштаб — 6×.

## Headless-проверка

```bash
./simulator/render.sh --scenario trip --output trip-64.png
./simulator/render.sh --display-height 32 --scenario trip --output trip-32.png
./simulator/render.sh --contact-sheet --output bikecomp-oled.png
./simulator/test.sh
```

Golden-тесты пиксель-в-пиксель сверяют девять сценариев отдельно для 128×32 и
128×64. Дополнительно проверяются граничные скорости, батарея и fallback длинной
строки. В `simulator/firmware_renderer.cpp` заданы только входные состояния.
