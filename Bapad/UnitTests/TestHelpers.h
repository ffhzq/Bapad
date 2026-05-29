#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "../TextDocument/FormatConversionV2.h"
#include "gtest/gtest.h"

namespace testing {
namespace internal {

// Printable for std::vector<char16_t> — needed by EXPECT_EQ across tests
// Assumes test data is ASCII-range (all existing tests use ASCII chars only)
template <>
inline void UniversalPrinter<std::vector<char16_t>>::Print(
    const std::vector<char16_t>& vec, std::ostream* os) {
  std::string result;
  result.reserve(vec.size());
  for (char16_t c : vec) {
    result.push_back(static_cast<char>(c));
  }
  *os << result;
}

}  // namespace internal
}  // namespace testing

// gtest 1.8.1 tries to stream char16_t via ostream, which was deleted in C++20.
inline std::ostream& operator<<(std::ostream& os, char16_t c) {
  os << static_cast<int>(c);
  return os;
}

// Convert a UTF-8 std::string to UTF-16 std::vector<char16_t> for test input
inline std::vector<char16_t> toWCharVector(const std::string& s) {
  std::vector<char> str(s.begin(), s.end());
  return RawToUtf16(str, CP_TYPE::UTF8);
}
