#include "pch.h"
#include "TextView.h"

std::vector<char16_t> TextView::ReadFileToUTF16(wchar_t* file_path) {
  std::vector<char> read_buffer;
  try {
    std::filesystem::path path(file_path);

    uintmax_t file_size = std::filesystem::file_size(path);
    read_buffer.resize(static_cast<size_t>(file_size));

    std::ifstream ifs(path, std::ios::binary);

    if (!ifs) {
      throw std::runtime_error("can't open the file.");
    }

    ifs.read(read_buffer.data(), file_size);
    if (!ifs) {
      throw std::runtime_error("read file failed.");
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::wcerr << L"fileSystem error: " << e.what() << std::endl;
    read_buffer.clear();
  } catch (const std::exception& e) {
    std::wcerr << L"error: " << e.what() << std::endl;
    read_buffer.clear();
  }
  int header_size = 0;
  CP_TYPE file_format = CP_TYPE::UNKNOWN;

  // Detect file format
  file_format = DetectFileFormat(read_buffer, header_size);
  read_buffer.erase(read_buffer.begin(), read_buffer.begin() + header_size);
  auto utf16Text = RawToUtf16(read_buffer, file_format);
  return utf16Text;
}

LONG TextView::OpenFile(WCHAR* szFileName) {
  ClearFile();
  if (pTextDoc->Initialize(ReadFileToUTF16(szFileName))) {
    lineCount = pTextDoc->GetLineCount();
    longestLine = GetLongestLine();
    UpdateMetrics();
    return TRUE;
  }

  return FALSE;
}

LONG TextView::ClearFile() {
  if (pTextDoc) pTextDoc->Clear();

  lineCount = pTextDoc->GetLineCount();
  longestLine = GetLongestLine();

  vScrollPos = 0;
  hScrollPos = 0;

  cursorOffset = 0;
  selectionStart = 0;
  selectionEnd = 0;

  currentLine = 0;
  caretPosX = 0;

  UpdateMetrics();

  return TRUE;
}