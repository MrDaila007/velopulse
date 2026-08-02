#!/usr/bin/env python3
"""Listen to Hall pin in labeled phases over Serial."""

from __future__ import annotations

import re
import sys
import time

import serial

PORT = "/dev/ttyACM0"
BAUD = 115200
HALL_RE = re.compile(
    r"Hall: pin=D0\+D1.*level=(HIGH|LOW), .*edge=(\w+), raw_pulses=(\d+), "
    r"accepted=(\d+), debounce_rej=(\d+), overspeed_rej=(\d+)"
)

PHASES = [
    (8, "1. BASELINE — ничего не трогать"),
    (10, "2. МАГНИТ У ДАТЧИКА — поднесите/удержите"),
    (10, "3. МАГНИТ УБРАТЬ — датчик в покое"),
    (8, "4. ЗАМКНУТЬ D1 на D0 (проводом) и держать"),
    (8, "5. ОТПУСТИТЬ — убрать провод"),
    (8, "6. ПОДНЕСТИ/УБРАТЬ МАГНИТ 3–4 раза"),
]


def send(port: serial.Serial, cmd: str, wait: float = 0.4) -> str:
    port.reset_input_buffer()
    port.write((cmd + "\n").encode("ascii"))
    time.sleep(wait)
    return port.read(8192).decode("utf-8", errors="replace")


def parse_lines(text: str) -> list[dict]:
    rows = []
    for m in HALL_RE.finditer(text):
        rows.append(
            {
                "level": m.group(1),
                "edge": m.group(2),
                "raw": int(m.group(3)),
                "accepted": int(m.group(4)),
                "debounce": int(m.group(5)),
                "overspeed": int(m.group(6)),
            }
        )
    return rows


def summarize(label: str, rows: list[dict]) -> None:
    if not rows:
        print(f"  {label}: нет данных Hall:")
        return
    levels = [r["level"] for r in rows]
    raw_start, raw_end = rows[0]["raw"], rows[-1]["raw"]
    acc_end = rows[-1]["accepted"]
    level_set = sorted(set(levels))
    print(f"  {label}:")
    print(f"    level: {level_set}  (последний={levels[-1]})")
    print(f"    raw_pulses: {raw_start} -> {raw_end}  (delta {raw_end - raw_start})")
    print(f"    accepted: {acc_end}  debounce_rej: {rows[-1]['debounce']}")


def main() -> int:
    print(f"Serial {PORT} @ {BAUD}")
    with serial.Serial(PORT, BAUD, timeout=0.3) as port:
        time.sleep(0.5)
        cfg = send(port, "dump-config", 1.2)
        edge = re.search(r"active_edge=(\d+)", cfg)
        deb = re.search(r"debounce_ms=(\d+)", cfg)
        print(
            f"Config: active_edge={edge.group(1) if edge else '?'} "
            f"(0=FALLING 1=RISING 2=CHANGE), debounce_ms={deb.group(1) if deb else '?'}"
        )
        st = send(port, "hall-status", 0.6)
        print(st.strip())
        print()
        send(port, "hall-watch", 0.3)

        all_rows: list[dict] = []
        phase_rows: list[tuple[str, list[dict]]] = []

        for duration, label in PHASES:
            print(f"\n>>> {label} ({duration}s)")
            t0 = time.time()
            chunk_rows: list[dict] = []
            while time.time() - t0 < duration:
                text = port.read(4096).decode("utf-8", errors="replace")
                new = parse_lines(text)
                for r in new:
                    chunk_rows.append(r)
                    all_rows.append(r)
                    print(
                        f"    [{time.time()-t0:4.1f}s] D1={r['level']} "
                        f"raw={r['raw']} accepted={r['accepted']}"
                    )
                time.sleep(0.1)

            phase_rows.append((label, chunk_rows))
            summarize(label.split("—", 1)[0].strip(), chunk_rows)

        send(port, "hall-stop", 0.3)

    print("\n========== ИТОГ ==========")
    if not all_rows:
        print("Нет строк Hall: — прошивка с hall-watch залита?")
        return 1

    levels = sorted({r["level"] for r in all_rows})
    raw_delta = all_rows[-1]["raw"] - all_rows[0]["raw"]
    print(f"Все уровни D1 за сессию: {levels}")
    print(f"raw_pulses суммарно: +{raw_delta}, accepted={all_rows[-1]['accepted']}")

    if levels == ["LOW"]:
        print("\nВывод: D1 ВСЕГДА LOW — геркон замкнут или короткое D1–D0.")
        print("  → отключите геркон и повторите hall-status")
    elif raw_delta == 0:
        print("\nВывод: уровень не даёт импульсов при active_edge=FALLING.")
        print("  → нужен переход HIGH→LOW; попробуйте hall-rising или hall-change")
    else:
        print("\nВывод: ISR видит импульсы — смотрите accepted/debounce.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
