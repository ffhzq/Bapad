#pragma once
#include <stdexcept>
#include <stdint.h>
#include <string>

class MyException : public std::runtime_error {
public:
  enum class ConversionType {
    FromUtf8ToUtf16 = 0,
    FromUtf16ToUtf8,
    FromUtf8ToUtf32,
    FromUtf32ToUtf8,
    FromUtf16ToUtf32,
    FromUtf32ToUtf16,
    FromUtf16BEToUtf32,
    FromAsciiToUtf16,
    FromUtf16ToAscii,
    CopyUtf16,
    SwapUtf16
  };

  MyException(const char* message, uint32_t errorCode, ConversionType type);

  MyException(const std::string message, uint32_t errorCode, ConversionType type);

  uint32_t ErrorCode() const noexcept;

  ConversionType Direction() const noexcept;

private:
  uint32_t _errorCode;
  ConversionType _conversionType;
};

MyException::MyException(const char* message, uint32_t errorCode, ConversionType type)
  : std::runtime_error(message),
  _errorCode(errorCode),
  _conversionType(type)

{}

MyException::MyException(const std::string message, uint32_t errorCode, ConversionType type)
  : std::runtime_error(message),
  _errorCode(errorCode),
  _conversionType(type)

{}


inline uint32_t MyException::ErrorCode() const noexcept
{
  return _errorCode;
}

inline MyException::ConversionType MyException::Direction() const noexcept
{
  return _conversionType;
}