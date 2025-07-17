#include "TextView.h"


TextView::TextView(HWND hwnd)
    :
    hWnd(hwnd),
    // Font-related data
    fontAttr(1),
    heightAbove(0),
    heightBelow(0),
    // Scrollbar related data
    vScrollPos(0),
    hScrollPos(0),
    vScrollMax(0),
    hScrollMax(0),
    // File-related data
    lineCount(0),
    longestLine(0),
    // Display-related data
    tabWidthChars(4),
    // Runtime data
    mouseDown(false),
    scrollTimer(0),
    scrollCounter(0),
    selectionStart(0),
    selectionEnd(0),
    cursorOffset(0),
    //
    pTextDoc(std::make_unique<TextDocument>())
{

    // Default display colours
    rgbColourList[TXC_FOREGROUND] = SYSCOL(COLOR_WINDOWTEXT);
    rgbColourList[TXC_BACKGROUND] = SYSCOL(COLOR_WINDOW);
    rgbColourList[TXC_HIGHLIGHTTEXT] = SYSCOL(COLOR_HIGHLIGHTTEXT);
    rgbColourList[TXC_HIGHLIGHT] = SYSCOL(COLOR_HIGHLIGHT);
    rgbColourList[TXC_HIGHLIGHTTEXT2] = SYSCOL(COLOR_INACTIVECAPTIONTEXT);
    rgbColourList[TXC_HIGHLIGHT2] = SYSCOL(COLOR_INACTIVECAPTION);


    // Set the default font
    auto hFont = static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT));
    if (hFont == nullptr)
        hFont = static_cast<HFONT>(GetStockObject(SYSTEM_FONT));
    OnSetFont(hFont);

    UpdateMetrics();
    //SetupScrollbars();
}

//
//	Destructor for TextView class
//
TextView::~TextView()
{
  this->pTextDoc.reset(nullptr);
    //DestroyCursor(m_hMarginCursor);
}

VOID TextView::UpdateMetrics()
{
    RECT rect;
    GetClientRect(hWnd, &rect);

    OnSize(0, rect.right, rect.bottom);
    RefreshWindow();

    RepositionCaret();
}

LONG TextView::OnSetFocus(HWND hwndOld)
{
    CreateCaret(hWnd, (HBITMAP)NULL, 2, lineHeight);
    RepositionCaret();

    ShowCaret(hWnd);
    RefreshWindow();
    return 0;
}

LONG TextView::OnKillFocus(HWND hwndNew)
{
    HideCaret(hWnd);
    DestroyCaret();
    RefreshWindow();
    return 0;
}





//WIN32
LRESULT CALLBACK TextViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TextView* ptv = (TextView*)GetWindowLongPtrW(hWnd, 0);

    switch (message)
    {
        // First message received by any window - make a new TextView object
        // and store pointer to it in our extra-window-bytes
        case WM_NCCREATE:
            if ((ptv = new TextView(hWnd)) == 0)
                return FALSE;
            SetWindowLongPtrW(hWnd, 0, reinterpret_cast<LONG_PTR>(ptv));
            return TRUE;
            // Last message received by any window - delete the TextView object
        case WM_NCDESTROY:
            delete ptv;
            SetWindowLongPtrW(hWnd, 0, 0);
            break;


        default:
            if (ptv)
                return ptv->WndProc(message, wParam, lParam);
            else
                return 0;//return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

LRESULT WINAPI TextView::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // Draw contents of TextView whenever window needs updating
        case WM_PAINT:
            return OnPaint();
            // Set a new font 
        case WM_SETFONT:
            return OnSetFont((HFONT)wParam);

        case WM_SIZE:
            return OnSize(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));

        case WM_VSCROLL:
            return OnVScroll(LOWORD(wParam), HIWORD(wParam));

        case WM_HSCROLL:
            return OnHScroll(LOWORD(wParam), HIWORD(wParam));

        case WM_MOUSEACTIVATE:
            return OnMouseActivate((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

        case WM_MOUSEWHEEL:
            return OnMouseWheel((short)HIWORD(wParam));

        case WM_SETFOCUS:
            return OnSetFocus((HWND)wParam);

        case WM_KILLFOCUS:
            return OnKillFocus((HWND)wParam);

        case WM_LBUTTONDOWN:
            return OnLButtonDown(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_LBUTTONUP:
            return OnLButtonUp(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_MOUSEMOVE:
            return OnMouseMove(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_TIMER:
            return OnTimer(wParam);

        case TXM_OPENFILE:
            return OpenFile(reinterpret_cast<wchar_t*>(lParam));

        case TXM_CLEAR:
            return ClearFile();

        case WM_CHAR:
            OnChar(wParam, lParam);

            //case TXM_SETLINESPACING:return SetLineSpacing(wParam, lParam);

        case TXM_ADDFONT:
            return AddFont((HFONT)wParam);

        case TXM_SETCOLOR:
            return SetColour(wParam, lParam);



        case TXM_GETFORMAT:
            return pTextDoc->GetFileFormat();

        default:
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

HWND CreateTextView(HWND hwndParent)
{
    return CreateWindowExW(WS_EX_CLIENTEDGE,
        TEXTVIEW_CLASS, L"",
        WS_VSCROLL | WS_HSCROLL | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        hwndParent,
        0,
        GetModuleHandleW(0),
        0);
}

BOOL RegisterTextView()
{
    WNDCLASSEXW wcex{ 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_DBLCLKS;// 0;
    wcex.lpfnWndProc = TextViewWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(TextView*);
    wcex.hInstance = GetModuleHandleW(0);
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursorW(NULL, IDC_IBEAM);
    wcex.hbrBackground = (HBRUSH)(0);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = TEXTVIEW_CLASS;
    wcex.hIconSm = 0;

    return RegisterClassExW(&wcex) ? TRUE : FALSE;
}

VOID TextView::UpdateCaretXY(int xpos, ULONG lineno)
{
    bool visible = false;

    // convert x-coord to window-relative
    xpos -= hScrollPos * fontWidth;


    // only show caret if it is visible within viewport
    if (lineno >= vScrollPos && lineno <= vScrollPos + windowLines)
    {
        if (xpos >= 0)
            visible = true;
    }

    // hide caret if it was previously visible
    if (visible == false && hideCaret == false)
    {
        hideCaret = true;
        HideCaret(hWnd);
    }
    // show caret if it was previously hidden
    else if (visible == true && hideCaret == true)
    {
        hideCaret = false;
        ShowCaret(hWnd);
    }

    // set caret position if within window viewport
    if (hideCaret == false)
    {
        SetCaretPos(xpos, (lineno - vScrollPos) * lineHeight);
    }

}