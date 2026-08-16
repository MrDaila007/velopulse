#!/usr/bin/env python3
"""Adafruit/Nordic legacy BLE DFU using the PC Bluetooth adapter (BlueZ/bleak)."""
from __future__ import annotations

import argparse
import asyncio
import struct
import sys
import zipfile
from pathlib import Path

from bleak import BleakClient, BleakScanner

UUID_DFU_SERVICE = "00001530-1212-efde-1523-785feabcd123"
UUID_DFU_CONTROL = "00001531-1212-efde-1523-785feabcd123"
UUID_DFU_PACKET = "00001532-1212-efde-1523-785feabcd123"
UUID_BIKE_SERVICE = "7c9a0001-4b7d-4f2e-9c1a-2e6d5f8b31a4"

OP_START_DFU = 1
OP_INITIALIZE_DFU = 2
OP_RECEIVE_FIRMWARE = 3
OP_VALIDATE = 4
OP_ACTIVATE_RESET = 5
OP_PKT_RCPT_REQ = 8
OP_RESPONSE = 16
OP_PKT_RCPT_NOTIF = 17
SUCCESS = 1
MODE_APPLICATION = 4
PACKET_SIZE = 20
PACKETS_BETWEEN_NOTIF = 10


def log(msg: str) -> None:
    print(msg, flush=True)


def load_zip(path: Path) -> tuple[bytes, bytes]:
    with zipfile.ZipFile(path) as zf:
        manifest = zf.read("manifest.json").decode()
        if "firmware.bin" not in zf.namelist():
            raise SystemExit(f"unexpected DFU zip contents: {zf.namelist()}")
        return zf.read("firmware.dat"), zf.read("firmware.bin")


async def scan_named(timeout: float) -> list[tuple[str, str, int, list[str]]]:
    found = await BleakScanner.discover(timeout=timeout, return_adv=True)
    hits = []
    for addr, (dev, adv) in found.items():
        name = (dev.name or adv.local_name or "").strip()
        uuids = [u.lower() for u in (adv.service_uuids or [])]
        hits.append((addr, name, adv.rssi or 0, uuids))
    return hits


async def find_device(timeout: float, prefer_dfu: bool, attempts: int = 4) -> tuple[str, str]:
    last_error = "BikeComp/DfuTarg not found"
    for attempt in range(1, attempts + 1):
        hits = await scan_named(timeout)
        dfu = []
        bike = []
        for addr, name, rssi, uuids in hits:
            lname = name.lower()
            if UUID_DFU_SERVICE in uuids or lname in {"dfutarg", "adadfu"} or "dfu" in lname:
                dfu.append((rssi, addr, name or "DfuTarg"))
            if UUID_BIKE_SERVICE in uuids or lname.startswith("bikecomp"):
                bike.append((rssi, addr, name or "BikeComp"))
            if name:
                log(f"  scan {addr} rssi={rssi} name={name!r}")
        pool = dfu if prefer_dfu and dfu else (dfu or bike)
        if pool:
            pool.sort(reverse=True)
            _, addr, name = pool[0]
            return addr, name
        last_error = f"BikeComp/DfuTarg not found (attempt {attempt}/{attempts})"
        log(last_error)
    raise RuntimeError(last_error)


class LegacyDfu:
    def __init__(self, client: BleakClient) -> None:
        self.client = client
        self._response: asyncio.Event = asyncio.Event()
        self._last: bytes = b""
        self._receipt: asyncio.Event = asyncio.Event()

    def _on_notify(self, _handle: int, data: bytearray) -> None:
        payload = bytes(data)
        if not payload:
            return
        if payload[0] == OP_PKT_RCPT_NOTIF:
            self._receipt.set()
            return
        self._last = payload
        self._response.set()

    async def start_notify(self) -> None:
        await self.client.start_notify(UUID_DFU_CONTROL, self._on_notify)

    async def _wait_response(self, opcode: int, timeout: float) -> None:
        try:
            await asyncio.wait_for(self._response.wait(), timeout)
        except TimeoutError as exc:
            raise RuntimeError(f"timeout waiting DFU response for opcode {opcode}") from exc
        self._response.clear()
        data = self._last
        if len(data) < 3 or data[0] != OP_RESPONSE or data[1] != opcode:
            raise RuntimeError(f"unexpected DFU response: {data.hex()}")
        if data[2] != SUCCESS:
            raise RuntimeError(f"DFU opcode {opcode} failed, status={data[2]}")

    async def control(self, opcode: int, extra: bytes = b"", wait: bool = True, timeout: float = 30.0) -> None:
        if wait:
            self._response.clear()
        await self.client.write_gatt_char(UUID_DFU_CONTROL, bytes([opcode]) + extra, response=True)
        if wait:
            await self._wait_response(opcode, timeout)

    async def packet(self, data: bytes) -> None:
        await self.client.write_gatt_char(UUID_DFU_PACKET, data, response=False)

    async def upload(self, init_packet: bytes, firmware: bytes) -> None:
        size_pkt = struct.pack("<III", 0, 0, len(firmware))
        log(f"START DFU application {len(firmware)} bytes")
        await self.control(OP_START_DFU, bytes([MODE_APPLICATION]), wait=False)
        await self.packet(size_pkt)
        await self._wait_response(OP_START_DFU, 30.0)

        log("INIT packet")
        await self.control(OP_INITIALIZE_DFU, b"\x00", wait=False)
        for i in range(0, len(init_packet), PACKET_SIZE):
            await self.packet(init_packet[i : i + PACKET_SIZE])
        await self.control(OP_INITIALIZE_DFU, b"\x01", timeout=60.0)

        await self.control(OP_PKT_RCPT_REQ, struct.pack("<H", PACKETS_BETWEEN_NOTIF), wait=False)

        log("RECEIVE firmware image")
        await self.control(OP_RECEIVE_FIRMWARE, wait=False)
        sent_packets = 0
        for i in range(0, len(firmware), PACKET_SIZE):
            chunk = firmware[i : i + PACKET_SIZE]
            if PACKETS_BETWEEN_NOTIF and (sent_packets + 1) % PACKETS_BETWEEN_NOTIF == 0:
                self._receipt.clear()
            await self.packet(chunk)
            sent_packets += 1
            if PACKETS_BETWEEN_NOTIF and sent_packets % PACKETS_BETWEEN_NOTIF == 0:
                try:
                    await asyncio.wait_for(self._receipt.wait(), 10.0)
                except TimeoutError as exc:
                    raise RuntimeError(f"packet receipt timeout at {i} bytes") from exc
            if i == 0 or (i // PACKET_SIZE) % 200 == 0:
                log(f"  {min(i + len(chunk), len(firmware))}/{len(firmware)} bytes")
        await self._wait_response(OP_RECEIVE_FIRMWARE, 60.0)

        log("VALIDATE")
        await self.control(OP_VALIDATE, timeout=30.0)
        log("ACTIVATE and reset")
        try:
            await self.control(OP_ACTIVATE_RESET, wait=False)
        except Exception:
            # Device often disconnects immediately after activate.
            pass


async def trigger_buttonless(address: str) -> None:
    log(f"connect {address} to start OTA")
    disconnected = asyncio.Event()

    def on_disconnect(_client: BleakClient) -> None:
        disconnected.set()

    last_error: Exception | None = None
    for attempt in range(1, 4):
        disconnected.clear()
        try:
            async with BleakClient(
                address, disconnected_callback=on_disconnect, timeout=20.0
            ) as client:
                if not client.is_connected:
                    raise RuntimeError("not connected")
                try:
                    await client.pair()
                    log("paired")
                except Exception as exc:
                    log(f"pair skipped: {exc}")
                await client.start_notify(UUID_DFU_CONTROL, lambda *_: None)
                await client.write_gatt_char(UUID_DFU_CONTROL, b"\x01", response=True)
                log("wrote DFU start, waiting for reboot into bootloader")
                try:
                    await asyncio.wait_for(disconnected.wait(), 8.0)
                except TimeoutError:
                    log("no disconnect event, continuing anyway")
                await asyncio.sleep(2.0)
                return
        except Exception as exc:
            last_error = exc
            log(f"buttonless attempt {attempt}/3 failed: {exc}")
            await asyncio.sleep(2.0)
    raise RuntimeError(f"buttonless DFU failed: {last_error}")


async def run(zip_path: Path) -> None:
    init_packet, firmware = load_zip(zip_path)
    log(f"package {zip_path} init={len(init_packet)} app={len(firmware)}")

    log("looking for BikeComp...")
    addr, name = await find_device(15.0, prefer_dfu=False)
    log(f"found {name} {addr}")
    if "dfu" not in name.lower() and "ada" not in name.lower():
        await trigger_buttonless(addr)
        log("looking for DfuTarg...")
        addr, name = await find_device(15.0, prefer_dfu=True)
        log(f"found {name} {addr}")

    async with BleakClient(addr) as client:
        dfu = LegacyDfu(client)
        await dfu.start_notify()
        await dfu.upload(init_packet, firmware)

    log("waiting for application reboot...")
    await asyncio.sleep(4.0)
    try:
        addr, name = await find_device(10.0, prefer_dfu=False)
        log(f"advertising again as {name} {addr}")
    except Exception as exc:
        log(f"post-dfu scan: {exc}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "zip_path",
        nargs="?",
        default=str(
            Path(__file__).resolve().parent.parent
            / ".pio"
            / "build"
            / "xiao_ble_sense"
            / "firmware.zip"
        ),
    )
    args = parser.parse_args()
    zip_path = Path(args.zip_path).resolve()
    if not zip_path.is_file():
        raise SystemExit(f"missing {zip_path}")
    asyncio.run(run(zip_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
