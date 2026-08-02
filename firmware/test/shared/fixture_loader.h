#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bike {
namespace test_support {

#ifndef PROTOCOL_FIXTURES_DIR
#define PROTOCOL_FIXTURES_DIR "../protocol/fixtures"
#endif

std::string fixturePath(const char* name, const char* ext);

bool loadFixtureText(const char* name, const char* ext, std::string& text);

bool loadFixtureHex(const char* name, std::vector<uint8_t>& bytes);

bool jsonHasNumber(const std::string& json, const char* key, long long expected);

bool jsonHasString(const std::string& json, const char* key, const char* expected);

}  // namespace test_support
}  // namespace bike
