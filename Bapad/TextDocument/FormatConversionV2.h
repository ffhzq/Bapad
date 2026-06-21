#ifndef TEXTDOCUMENT_FORMATCONVERSIONV2_H_
#define TEXTDOCUMENT_FORMATCONVERSIONV2_H_

#include <vector>

/// Code-page / encoding type used by Bapad internally.
enum class CP_TYPE { ANSI, UTF8, UTF16, UTF16BE, UTF32, UTF32BE, UNKNOWN = -1 };

/// Byte-order mark lookup entry.
struct _BOM_LOOKUP {
  unsigned long bom;
  int headerLength;
  CP_TYPE codePageType;
};

/// Detect file encoding from its BOM (byte order mark) and fall back to
/// heuristic UTF-8 detection.
/// @param docBuffer  Raw file content bytes.
/// @param headerSize [out] Number of BOM bytes to skip (0 if no BOM found).
/// @return Detected CP_TYPE. Returns UNKNOWN for empty buffers.
CP_TYPE DetectFileFormat(std::vector<char> docBuffer, int& headerSize) noexcept;

/// Swap the high and low bytes of a 16-bit word (host endianness ↔ opposite).
template <typename T>
inline auto SwapWord16(T& ch16) noexcept {
  T left_val = ((ch16) << 8);
  T right_val = ((ch16) >> 8);
  ch16 = left_val | right_val;
  return ch16;
}

/// Check whether a byte sequence is valid UTF-8.
/// @note On MSVC (signed char), bytes >= 0x80 are cast to unsigned char before
/// comparison to avoid misidentifying multi-byte sequences as ASCII.
bool IsUTF8(std::vector<char> buffer) noexcept;

/// Convert raw bytes to UTF-16 (the internal representation used by Bapad).
/// @param rawData        Raw file content bytes.
/// @param rawDataCodpage Encoding of the raw data (UTF8, UTF16, UTF16BE, or
///                       ANSI). UTF32 / UNKNOWN throws std::runtime_error.
/// @return Decoded UTF-16 data. Empty vector on ICU conversion failure.
std::vector<char16_t> RawToUtf16(std::vector<char>& rawData,
                                 const CP_TYPE rawDataCodpage);

/// Convert UTF-16 back to raw bytes (inverse of RawToUtf16).
/// @throw std::runtime_error Always — not yet implemented.
std::vector<char> Utf16toRaw(std::vector<char16_t>& utf16Data,
                             const CP_TYPE rawDataCodpage);

/// Normalize line endings in UTF-16 data: CR (\\r) and CRLF (\\r\\n) are
/// converted to LF (\\n). LF is left unchanged.
/// @param utf16Data  Input UTF-16 text with mixed line endings.
/// @return Text with all line endings normalized to LF (\\n).
std::vector<char16_t> NormalizeLineEndings(
    const std::vector<char16_t>& utf16Data);

#endif
