# Организация задач

Источники backlog:

- `docs/02-development-plan.md` — этапы Э0–Э7;
- `docs/05-hardware-design.md` — аппаратные проверки;
- `docs/06-testing-and-acceptance.md` — приёмка и регрессия;
- `STATUS.md` — фактически подтверждённое состояние.

## Статусы

- `[x]` — выполнено и подтверждено.
- `[ ]` — не выполнено или выполнено частично.
- Для частичной задачи рядом указана недостающая часть.

## Правила

1. Этап закрывается только после задач и его Definition of Done.
2. Hardware/acceptance закрываются только с датой и артефактом проверки.
3. Изменение BLE-протокола начинается в `protocol/`, затем fixtures, firmware и mobile.
4. Embedded-тесты используют отдельные test-файлы и возвращают production firmware.
5. После каждого инкремента обновляются `STATUS.md` и соответствующий task-файл.

## Владельцы папок

- `firmware/` — firmware/domain/storage/BLE.
- `hardware/` — BOM, монтаж, измерения и стенды.
- `mobile/` — Flutter-приложение.
- `verification/` — acceptance, soak, field tests и release.
- `future/` — задачи за пределами v1.0.
- `firmware/zephyr.md` — миграция на Zephyr RTOS (`firmware-zephyr/`).
