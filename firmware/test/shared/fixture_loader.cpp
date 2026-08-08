#include "fixture_loader.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace bike {
namespace test_support {

std::string fixturePath(const char* name, const char* ext) {
  std::ostringstream path;
  path << PROTOCOL_FIXTURES_DIR << "/" << name << "." << ext;
  return path.str();
}

bool loadFixtureText(const char* name, const char* ext, std::string& text) {
  std::ifstream input(fixturePath(name, ext).c_str());
  if (!input) {
    return false;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  text = buffer.str();
  return true;
}

int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool loadFixtureHex(const char* name, std::vector<uint8_t>& bytes) {
  std::string text;
  if (!loadFixtureText(name, "hex", text)) {
    return false;
  }
  bytes.clear();
  int high = -1;
  for (char c : text) {
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      continue;
    }
    const int nibble = hexNibble(c);
    if (nibble < 0) {
      return false;
    }
    if (high < 0) {
      high = nibble;
    } else {
      bytes.push_back(static_cast<uint8_t>((high << 4) | nibble));
      high = -1;
    }
  }
  return high < 0 && !bytes.empty();
}

bool jsonHasNumber(const std::string& json, const char* key, long long expected) {
  const std::string needle = std::string("\"") + key + "\": ";
  const size_t pos = json.find(needle);
  if (pos == std::string::npos) {
    return false;
  }
  size_t i = pos + needle.size();
  while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) {
    ++i;
  }
  char* end = nullptr;
  const long long value = strtoll(json.c_str() + i, &end, 10);
  return end != json.c_str() + i && value == expected;
}

bool jsonHasString(const std::string& json, const char* key, const char* expected) {
  const std::string needle =
      std::string("\"") + key + "\": \"" + expected + "\"";
  return json.find(needle) != std::string::npos;
}

}  // namespace test_support
}  // namespace bike
