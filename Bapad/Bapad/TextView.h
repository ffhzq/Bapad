#pragma once
#include "..\TextDocument\TextDocument.h"
#include "FontStruct.h"
#include "TextViewWin32.h"
#include "usp10.h"

//#define LONGEST_LINE 0x100

constexpr int LONGEST_LINE = 0x100;
constexpr int TEXTBUFSIZE = 32;
constexpr int MAX_FONTS = 32;


class TextView {
public:

  TextView(HWND hwnd);
  ~TextView();

  TextView(const TextView&) = delete;
  TextView& operator=(const TextView&) = delete;
  TextView(TextView&&) = delete;
  TextView& operator=(TextView&&) = delete;

  LRESULT WINAPI WndProc(UINT msg, WPARAM wParam, LPARAM lParam);



private:

  void    PaintLine(HDC hdc, ULONG64 nLineNo);
  void    PaintText(HDC hdc, ULONG64 nLineNo, RECT* rect);

  size_t  ApplyTextAttributes(size_t nLineNo, size_t offset, std::vector<wchar_t>& szText, std::vector<ATTR>& attr);
  int     BaTextOut(HDC hdc, int xpos, int ypos, gsl::span<wchar_t> szText, int nTabOrigin, const ATTR& attr);

  int     PaintCtrlChar(HDC hdc, int xpos, int ypos, ULONG64 chValue, FONT* fa);
  void    InitCtrlCharFontAttr(HDC hdc, FONT* fa);

  void    RefreshWindow();
  LONG    InvalidateRange(size_t nStart, size_t nFinish);

  int     GetCtrlCharWidth(HDC hdc, ULONG64 chValue, FONT* fa);
  int     BaTextWidth(HDC hdc, std::span<wchar_t> bufSpan, int nTabOrigin);
  int     TabWidth() const;
  int     GetLongestLine();

  BOOL    MouseCoordToFilePos(
    LONGLONG mx,
    LONGLONG my,
    size_t& pnLineNo,
    size_t& pnFileOffset,
    int& px);
  void  RepositionCaret();


  COLORREF  GetColour(UINT idx);

  VOID    SetupScrollbars();
  VOID    UpdateMetrics();
  VOID    RecalculateLineHeight();
  bool    PinToBottomCorner();

  void    Scroll(LONG64 dx, LONG64 dy);
  HRGN    ScrollRgn(LONG64 dx, LONG64 dy, bool fReturnUpdateRgn);
  VOID    ScrollToPosition(int xpos, size_t lineno);
  HWND hWnd;

  // Font-related data
  std::vector<FONT> fontAttr;
  HFONT font;
  int fontWidth;
  int lineHeight;
  int maxAscent;
  int heightAbove;
  int heightBelow;

  // Scrollbar related data
  ULONG64 vScrollPos;
  ULONG64 vScrollMax;
  LONG64  hScrollPos;
  LONG64  hScrollMax;

  size_t  longestLine;
  ULONG64 windowLines;
  ULONG64 windowColumns;

  // Display related data
  int tabWidthChars;
  size_t  selectionStart;
  size_t  selectionEnd;
  size_t  cursorOffset;
  int     caretPosX;
  size_t  currentLine;

  COLORREF  rgbColourList[TXC_MAX_COLOURS];

  // File-related data
  size_t    lineCount;


  // Runtime related data
  bool mouseDown;
  UINT_PTR scrollTimer;
  int scrollCounter;
  bool hideCaret;
  std::unique_ptr<TextDocument> pTextDoc;


  //处理消息
  LONG OnPaint();
  LONG OnSetFont(HFONT hFont);
  LONG OnSize(UINT nFlags, int width, int height);
  LONG OnVScroll(UINT nSBCode, UINT nPos);
  LONG OnHScroll(UINT nSBCode, UINT nPos);
  LONG OnMouseWheel(int nDelta);
  LONG OnTimer(UINT nTimer);

  //鼠标
  LONG OnMouseActivate(HWND hwndTop, UINT nHitTest, UINT nMessage);
  LONG OnLButtonDown(UINT nFlags, int x, int y);
  LONG OnLButtonUp(UINT nFlags, int x, int y);
  LONG OnMouseMove(UINT nFlags, int x, int y);

  LONG OnSetFocus(HWND hwndOld);
  LONG OnKillFocus(HWND hwndNew);

  //用户操作

  LONG OpenFile(WCHAR* szFileName);
  LONG ClearFile();

  LONG AddFont(HFONT);
  LONG SetFont(HFONT, size_t idx);
  LONG SetLineSpacing(int nAbove, int nBelow);
  COLORREF SetColour(UINT idx, COLORREF rgbColour);

  LONG OnChar(UINT nChar, UINT nFlags);
  ULONG EnterText(WCHAR* inputText, ULONG inputTextLength);
  LRESULT NotifyParent(UINT nNotifyCode, NMHDR* optional = 0);
  void  SyncMetrics(BOOL fAdvancing);
  void  UpdateCaretXY(int xpos, ULONG lineno) noexcept;
  void  UpdateCaretOffset(BOOL fAdvancing);

  // Cursor move
  void MoveCharNext();
  void MoveCharPre();
  void MoveLineUp(int numOfLines);
  void MoveLineDown(int numOfLines);
  void MoveLineStart(int lineNumber);
  void MoveLineEnd(int lineNumber);

};

void    PaintRect(HDC hdc, int x, int y, int width, int height, COLORREF fill);
void    DrawCheckedRect(HDC hdc, RECT* rect, COLORREF fg, COLORREF bg);
bool    IsKeyPressed(UINT nVirtKey) noexcept;