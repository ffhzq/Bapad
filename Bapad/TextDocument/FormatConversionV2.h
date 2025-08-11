#pragma once
#include "pch.h"
#include "FormatConversion.h"

std::vector<wchar_t> RawToUtf16(std::vector<unsigned char>& rawData, const CP_TYPE rawDataCodpage);
std::vector<unsigned char> Utf16toRaw(std::vector<wchar_t>& utf16Data, const CP_TYPE rawDataCodpage);