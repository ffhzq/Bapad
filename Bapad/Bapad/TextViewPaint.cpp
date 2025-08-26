#include "TextView.h"



extern "C" COLORREF MixRGB(COLORREF, COLORREF);

size_t TextView::ApplyTextAttributes(size_t nLineNo, size_t nOffset, std::vector<wchar_t>& szText, std::vector<ATTR>& attr)
{
  size_t font = nLineNo % fontAttr.size();
  const COLORREF fg = RGB(rand() % 200, rand() % 200, rand() % 200);
  const size_t nTextLen = szText.size();
  assert(nTextLen == attr.size());
  if (attr.empty() || szText.empty())
  {
    return 0;
  }

  const size_t selstart = (std::min)(selectionStart, selectionEnd);
  const size_t selend = (std::max)(selectionStart, selectionEnd);



  for (size_t i = 0; i < nTextLen; i++)
  {
    // apply highlight colours 
    if (nOffset + i >= selstart && nOffset + i < selend)
    {
      if (GetFocus() == hWnd)
      {
        gsl::at(attr, i).fg = GetColour(TXC_HIGHLIGHTTEXT);
        gsl::at(attr, i).bg = GetColour(TXC_HIGHLIGHT);
      }
      else
      {
        gsl::at(attr, i).fg = GetColour(TXC_HIGHLIGHTTEXT2);
        gsl::at(attr, i).bg = GetColour(TXC_HIGHLIGHT2);
      }
    }
    // normal text colours
    else
    {
      gsl::at(attr, i).fg = GetColour(TXC_FOREGROUND);
      gsl::at(attr, i).bg = GetColour(TXC_BACKGROUND);
    }

    if (gsl::at(szText, i) == ' ')
      font = (font + 1) % fontAttr.size();

    gsl::at(attr, i).style = font;
  }

  //
  //	Turn any CR/LF at the end of a line into a single 'space' character
  //
  return StripCRLF(szText, false);
}

//
//	Strip CR/LF combinations from the end of a line and
//  replace with a single space character (for drawing purposes)
//  fAllow : flag of allowing to strip string
//  bool -> int : true -> 1, false -> 0
//
size_t StripCRLF(std::vector<wchar_t>& szText, bool fAllow) noexcept
{
  if (szText.empty()) return 0;
  size_t nLength = szText.size();
  if (nLength >= 2)
  { // todo: distinguish CRLF LF CR modes.
    if (gsl::at(szText, nLength - 2) == '\r' && gsl::at(szText, nLength - 1) == '\n')
    {
      gsl::at(szText, nLength - 2) = ' ';
      nLength = nLength - (size_t{ 1 } + static_cast<int>(fAllow));
      return nLength;
    }
  }

  if (nLength >= 1)
  {
    // todo: check CR LF
    if (gsl::at(szText, nLength - 1) == '\r' || gsl::at(szText, nLength - 1) == '\n')
    {
      gsl::at(szText, nLength - 1) = ' ';
      nLength = nLength - (static_cast<int>(fAllow));
    }
  }
  return nLength;
}

//
//	Perform a full redraw of the entire window
//
VOID TextView::RefreshWindow()
{
  InvalidateRect(hWnd, nullptr, FALSE);
}



//
//	Draw tabbed text using specified colours and font, return width of output text
//
//	We could just use TabbedTextOut - however, we need to parse the text because
//  it might contain ascii-control characters which must be output separately.
//  because of this we'll just handle the tabs at the same time.
//	
int TextView::BaTextOut(HDC hdc, int xpos, int ypos, gsl::span<wchar_t> szText, int nTabOrigin, const ATTR& attr)
{
  const size_t nLen = szText.size();
  const int xold = xpos;
  SIZE sz;

  const int TABWIDTHPIXELS = tabWidthChars * fontWidth;

  FONT& font = gsl::at(fontAttr, attr.style);

  // configure the device context
  SetTextColor(hdc, attr.fg);
  SetBkColor(hdc, attr.bg);
  SelectObject(hdc, font.hFont);

  SetBkMode(hdc, OPAQUE);
  constexpr DWORD flag = ETO_CLIPPED | ETO_OPAQUE;

  // loop over each character
  for (size_t i = 0, lasti = 0; i <= nLen; i++)
  {
    const int yoff = maxAscent + heightAbove - font.tm.tmAscent;

    // output any "deferred" text before handling tab/control chars
    if (i == nLen || szText[i] == '\t' || szText[i] < 32)
    {
      RECT rect;

      // get size of text
      GetTextExtentPoint32W(hdc, &szText[lasti], i - lasti, &sz);
      SetRect(&rect, xpos, ypos, xpos + sz.cx, ypos + lineHeight);

      // draw the text and erase it's background at the same time
      ExtTextOutW(hdc, xpos, ypos + yoff, flag, &rect, &szText[lasti], i - lasti, nullptr);

      xpos += sz.cx;
    }

    // special case for TAB and Control characters
    if (i < nLen)
    {
      // TAB characters
      if (szText[i] == '\t')
      {
        // calculate distance in pixels to the next tab-stop
        const int width = TABWIDTHPIXELS - ((xpos - nTabOrigin) % TABWIDTHPIXELS);

        // draw a blank space 
        PaintRect(hdc, xpos, ypos, width, lineHeight, attr.bg);

        xpos += width;
        lasti = i + 1;
      }
      // ASCII-CONTROL characters
      else if (szText[i] < 32)
      {
        xpos += PaintCtrlChar(hdc, xpos, ypos, szText[i], &font);
        lasti = i + 1;
      }
    }
  }

  // return the width of text drawn
  return xpos - xold;
}



//
//	Painting procedure for TextView objects
//
LONG TextView::OnPaint()
{
  PAINTSTRUCT ps;

  ULONG i, first, last;

  BeginPaint(hWnd, &ps);

  // select which font we will be using
  //SelectObject(ps.hdc, font);

  // figure out which lines to redraw
  first = vScrollPos + ps.rcPaint.top / lineHeight;
  last = vScrollPos + ps.rcPaint.bottom / lineHeight;

  // make sure we never wrap around the 4gb boundary
  if (last < first) last = -1;

  // draw the display line-by-line
  for (i = first; i <= last; i++)
  {
    PaintLine(ps.hdc, i);
  }

  EndPaint(hWnd, &ps);

  return 0;
}

//
//	Draw the specified line (including margins etc)
//
void TextView::PaintLine(HDC hdc, ULONG64 nLineNo)
{
  RECT  rect;

  // Get the area we want to update
  GetClientRect(hWnd, &rect);

  // calculate rectangle for entire length of line in window
  rect.left = (-hScrollPos * fontWidth);
  rect.top = ((nLineNo - vScrollPos) * lineHeight);
  rect.right = (rect.right);
  rect.bottom = (rect.top + lineHeight);

  //	check we have data to draw on this line
  if (nLineNo >= lineCount)
  {
    SetBkColor(hdc, GetColour(TXC_BACKGROUND));
    ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
    return;
  }

  //	paint the line's text
  PaintText(hdc, nLineNo, &rect);
}

//
//	Draw a line of text into the TextView window
//
void TextView::PaintText(HDC hdc, ULONG64 nLineNo, RECT* rect)
{
  size_t  charoff = 0;

  size_t  colNo = 0;

  int xpos = rect->left;
  const int ypos = rect->top;
  const int xtab = rect->left;


  TextIterator itor = pTextDoc->IterateLineByLineNumber(nLineNo, &charoff, nullptr);


  auto buff = itor.GetLine();
  size_t len = buff.size();
  std::vector<ATTR> attr;
  attr.resize(len);
  len = ApplyTextAttributes(nLineNo, charoff, buff, attr);

  if (len == 0)
    Sleep(0);

  //
  //	Display the text by breaking it into spans of colour/style
  //
  for (int i = 0, lasti = 0; i <= len; i++)
  {
    // if the colour or font changes, then need to output 
    if (i == len ||
      gsl::at(attr, i).fg != gsl::at(attr, lasti).fg ||
      gsl::at(attr, i).bg != gsl::at(attr, lasti).bg ||
      gsl::at(attr, i).style != gsl::at(attr, lasti).style)
    {
      if (i - lasti != 0)
      {
        const auto buffSpan = gsl::make_span(buff).subspan(lasti, i - lasti);
        xpos += BaTextOut(hdc, xpos, ypos, buffSpan, xtab, gsl::at(attr, lasti));
      }
      lasti = i;
    }
  }
  charoff += len;


  //
  // Erase to the end of the line
  //
  rect->left = xpos;
  SetBkColor(hdc, GetColour(TXC_BACKGROUND));
  ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, rect, nullptr, 0, nullptr);
}

void PaintRect(HDC hdc, int x, int y, int width, int height, COLORREF fill)
{
  const RECT rect = {x, y, x + width, y + height};

  fill = SetBkColor(hdc, fill);
  ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
  SetBkColor(hdc, fill);
}



//
//	Return an RGB value corresponding to the specified HVC_xxx index
//
//	If the RGB value has the top bit set (0x80000000) then it is
//  not a real RGB value - instead the low 31bits specify one
//  of the GetSysColor COLOR_xxx indices. This allows us to use
//	system colours without worrying about colour-scheme changes etc.
//
COLORREF TextView::GetColour(UINT idx)
{
  if (idx >= TXC_MAX_COLOURS)
    return 0;

  return REALIZE_SYSCOL(rgbColourList[idx]);

}

COLORREF TextView::SetColour(UINT idx, COLORREF rgbColour)
{
  COLORREF rgbOld;

  if (idx >= TXC_MAX_COLOURS)
    return 0;

  rgbOld = rgbColourList[idx];
  rgbColourList[idx] = rgbColour;

  return rgbOld;
}

//
//	Paint a checkered rectangle, with each alternate
//	pixel being assigned a different colour
//
void DrawCheckedRect(HDC hdc, RECT* rect, COLORREF fg, COLORREF bg)
{
  static WORD wCheckPat[8] =
  {
      0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555
  };

  HBITMAP hbmp;
  HBRUSH  hbr, hbrold;
  COLORREF fgold, bgold;

  hbmp = CreateBitmap(8, 8, 1, 1, wCheckPat);
  hbr = CreatePatternBrush(hbmp);

  hbrold = (HBRUSH)SelectObject(hdc, hbr);

  fgold = SetTextColor(hdc, fg);
  bgold = SetBkColor(hdc, bg);

  PatBlt(hdc, rect->left, rect->top,
    rect->right - rect->left,
    rect->bottom - rect->top,
    PATCOPY);

  SetBkColor(hdc, bgold);
  SetTextColor(hdc, fgold);

  SelectObject(hdc, hbrold);
  DeleteObject(hbr);
  DeleteObject(hbmp);
}

COLORREF MixRGB(COLORREF rgbCol1, COLORREF rgbCol2)
{
  return RGB(
    (GetRValue(rgbCol1) + GetRValue(rgbCol2)) / 2,
    (GetGValue(rgbCol1) + GetGValue(rgbCol2)) / 2,
    (GetBValue(rgbCol1) + GetBValue(rgbCol2)) / 2
  );
}

COLORREF RealizeColour(COLORREF col)
{
  COLORREF result = col;

  if (col & 0x80000000)
    result = GetSysColor(col & 0xff);

  if (col & 0x40000000)
    result = MixRGB(GetSysColor((col & 0xff00) >> 8), result);

  if (col & 0x20000000)
    result = MixRGB(GetSysColor((col & 0xff00) >> 8), result);

  return result;
}