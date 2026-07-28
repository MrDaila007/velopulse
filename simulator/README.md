# Симулятор OLED BikeComp

Симулятор использует [u8g2-python-simulator](https://github.com/colinoflynn/u8g2-python-simulator)
и исходные шрифты [u8g2](https://github.com/olikraus/u8g2). Обе зависимости загружаются
в `.vendor/` на зафиксированных commits; в репозиторий они не копируются.

## Установка

```bash
./simulator/setup.sh
```

Требуются Python 3, `venv`, Tk и `xvfb-run`. Python-зависимость Pillow устанавливается
в локальный каталог `simulator/.venv`.

## Интерактивный GUI

```bash
./simulator/run_gui.sh          # demo: IDLE → MOV → PAUSE каждые 4 секунды
./simulator/run_gui.sh moving   # фиксированный кадр
./simulator/run_gui.sh idle
./simulator/run_gui.sh paused
```

Окно имеет реальные 128×32 пикселя с масштабом 6× и автоматически перечитывает
`draw_bikecomp.py` после сохранения. Клавиши upstream-симулятора: `s` — PNG, `g` —
запись GIF, `i` — инверсия.

## Headless-проверка

```bash
./simulator/render.sh --scenario moving --output moving.png
./simulator/render.sh --contact-sheet --output bikecomp-oled.png
./simulator/test.sh
```

PNG создаются относительно каталога `simulator/`. Golden-тесты сверяют кадры `idle`,
`moving` и `paused` пиксель-в-пиксель с файлами в `golden/`. После намеренного изменения
разметки обновите эталоны тремя командами `render.sh` с `--scale 1` и внимательно
просмотрите контактный лист до принятия изменений.

Разметка в `draw_bikecomp.py` должна оставаться синхронной с
`firmware/src/display_manager.cpp`: координаты, формат скорости, состояния и дистанции.
