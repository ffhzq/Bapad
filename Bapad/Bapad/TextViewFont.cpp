#include "pch.h"
#include "TextView.h"


//
//	Wrapper for GetTextExtentPoint32.
//
int TextView::BaTextWidth(HDC hdc, std::span<char16_t> bufSpan,
                          int nTabOrigin) {
  SIZE sz;
  int width = 0;

  const int TABWIDTHPIXELS = tabWidthChars * fontWidth;
  const size_t len = bufSpan.size();
  for (int i = 0, lasti = 0; i <= len; i++) {
    if (i == len || gsl::at(bufSpan, i) == '\t' || gsl::at(bufSpan, i) < 32) {
      if (lasti == i) continue;
      GetTextExtentPoint32W(hdc,
                            reinterpret_cast<LPCWSTR>(&gsl::at(bufSpan, lasti)),
                            i - lasti, &sz);
      width += sz.cx;

      if (i < len && gsl::at(bufSpan, i) == '\t') {
        width += TABWIDTHPIXELS - ((width - nTabOrigin) % TABWIDTHPIXELS);
        lasti = i + 1;
      } else if (i < len && gsl::at(bufSpan, i) < 32) {
        // width += GetCtrlCharWidth(hdc, gsl::at(bufSpan, i),
        // &gsl::at(fontAttr, 0));
        GetTextExtentPoint32W(
            hdc, reinterpret_cast<LPCWSTR>(&gsl::at(bufSpan, i)), 1, &sz);
        width += sz.cx;
        lasti = i + 1;
      }
    }
  }

  return width;
}

//
//	Set default font, WM_SETFONT handler: set a new default font
//
LONG TextView::OnSetFont(HFONT hFont) {
  // default font is always #0
  SetFont(hFont, 0);

  fontWidth = gsl::at(fontAttr, 0).tm.tmAveCharWidth;
  lineHeight = gsl::at(fontAttr, 0).tm.tmHeight;
  UpdateMetrics();

  return 0;
}

LONG TextView::AddFont(HFONT hFont) {
  const size_t idx = fontAttr.size();
  fontAttr.resize(idx + 1);
  SetFont(hFont, idx);
  UpdateMetrics();
  return 0;
}

LONG TextView::SetFont(HFONT hFont, size_t idx) {
  FONT* font = &fontAttr[idx];
  // need a DC to query font data
  HDC hdc = GetDC(nullptr);
  HANDLE hold = SelectObject(hdc, hFont);

  // get font settings
  font->hFont = hFont;
  GetTextMetricsW(hdc, &font->tm);

  // pre-calc the control-characters for this font
  // InitCtrlCharFontAttr(hdc, font);

  // cleanup
  SelectObject(hdc, hold);
  ReleaseDC(0, hdc);

  RecalculateLineHeight();

  return 0;
}

//
//	Update the lineheight based on current font settings
//
VOID TextView::RecalculateLineHeight() {
  lineHeight = 0;
  maxAscent = 0;

  // find the tallest font in the TextView
  for (int i = 0; i < fontAttr.size(); i++) {
    // always include a font's external-leading
    int fontheight = fontAttr[i].tm.tmHeight + fontAttr[i].tm.tmExternalLeading;

    lineHeight = std::max(lineHeight, fontheight);
    maxAscent = std::max(maxAscent, fontAttr[i].tm.tmAscent);
  }

  // add on the above+below spacings
  lineHeight += heightAbove + heightBelow;

  // force caret resize if we've got the focus
  if (GetFocus() == hWnd) {
    OnKillFocus(0);
    OnSetFocus(0);
  }
}

//
//	Set spacing (in pixels) above and below each line -
//  this is in addition to the external-leading of a font
//
LONG TextView::SetLineSpacing(int nAbove, int nBelow) {
  heightAbove = nAbove;
  heightBelow = nBelow;
  RecalculateLineHeight();
  return TRUE;
}

int TextView::TabWidth() const { return tabWidthChars * fontWidth; }

int TextView::GetLongestLine() {  // todo: cache length, check if text be
                                  // inserted or replaced
  HDC hdc = GetDC(hWnd);
  HANDLE hFont = SelectObject(hdc, gsl::at(fontAttr, 0).hFont);
  int longestLinePixel = 0;
  for (size_t i = 0; i < lineCount; ++i) {
    auto itor = pTextDoc->IterateLineByLineNumber(i, nullptr, nullptr);
    auto lineContent(itor.GetLine());
    const int textLength = BaTextWidth(hdc, lineContent, 0);
    longestLinePixel = (std::max)(textLength / fontWidth, longestLinePixel);
  }
  ReleaseDC(hWnd, hdc);
  return longestLinePixel;
}
