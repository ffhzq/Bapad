#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "../TextDocument/FormatConversionV2.h"
#include "gtest/gtest.h"

namespace testing {
namespace internal {

// Printable for std::vector<char16_t> — needed by EXPECT_EQ across tests
template <>
inline void UniversalPrinter<std::vector<char16_t>>::Print(
    const std::vector<char16_t>& vec, std::ostream* os) {
  std::string result(vec.begin(), vec.end());
  *os << result;
}

}  // namespace internal
}  // namespace testing

// Convert a UTF-8 std::string to UTF-16 std::vector<char16_t> for test input
inline std::vector<char16_t> toWCharVector(const std::string& s) {
  std::vector<char> str(s.begin(), s.end());
  return RawToUtf16(str, CP_TYPE::UTF8);
}
