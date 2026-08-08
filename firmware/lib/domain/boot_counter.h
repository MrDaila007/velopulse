#pragma once

#include <stdint.h>

#include "storage_manager.h"

namespace bike {

// LittleFS path for the persistent reboot counter (Device Info boot_count).
constexpr char kBootCountPath[] = "/boot_cnt";

// Reads `/boot_cnt` (uint16 LE), increments by one (saturating at UINT16_MAX),
// and writes it back. Missing/corrupt file starts at 1. Returns the new value;
// on write failure still returns the incremented in-RAM value.
uint16_t loadAndIncrementBootCount(StorageBackend& backend,
                                   const char* path = kBootCountPath);

}  // namespace bike
