#pragma once

//
//	ATTR - text character attribute
//
struct ATTR {
  ATTR() noexcept : fg(0), bg(0), style(0) {}
  COLORREF fg;  // foreground colour
  COLORREF bg;  // background colour
  ULONG style;  // possible font-styling information
};

//
//	FONT - font attributes
//
struct FONT {
  FONT() noexcept : hFont(nullptr), tm({0}), nInternalLeading(0), nDescent(0) {}
  // Windows font information
  HFONT hFont;
  TEXTMETRICW tm;

  // dimensions needed for control-character 'bitmaps'
  int nInternalLeading;
  int nDescent;
};
