// TextView.cpp : Defines the functions for the static library.
//
#include "pch.h"
#include "TextView.h"

// TODO: This is an example of a library function
void fnTextView()
{
}



TextView::TextView(HWND hwnd) : pTextDoc(new TextDocument())
{
	hWnd = hwnd;

	// Set the default font
	OnSetFont((HFONT)GetStockObject(ANSI_FIXED_FONT));

	// Scrollbar related data
	vScrollPos = 0;
	hScrollPos = 0;
	vScrollMax = 0;
	hScrollMax = 0;

	// File-related data
	lineCount = 0;
	longestLine = 0;


	SetupScrollbars();
}

//
//	Destructor for TextView class
//
TextView::~TextView()
{
	if (pTextDoc)
		pTextDoc.release();
	//delete m_pTextDoc;
}

VOID TextView::UpdateMetrics()
{
	RECT rect;
	GetClientRect(hWnd, &rect);

	OnSize(0, rect.right, rect.bottom);
	RefreshWindow();
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
            break;
        // Draw contents of TextView whenever window needs updating
        case WM_PAINT:
            return ptv->OnPaint();
        // Set a new font 
        case WM_SETFONT:
            return ptv->OnSetFont((HFONT)wParam);
        case WM_SIZE:
            return ptv->OnSize(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
        case WM_VSCROLL:
            return ptv->OnVScroll(LOWORD(wParam), HIWORD(wParam));
        case WM_HSCROLL:
            return ptv->OnHScroll(LOWORD(wParam), HIWORD(wParam));
        case WM_MOUSEACTIVATE:
            return ptv->OnMouseActivate((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
        case WM_MOUSEWHEEL:
            return ptv->OnMouseWheel((short)HIWORD(wParam));
            
        case WM_SETFOCUS:
            return ptv->OnSetFocus((HWND)wParam);

        case WM_KILLFOCUS:
            return ptv->OnKillFocus((HWND)wParam);

        case WM_LBUTTONDOWN:
            return ptv->OnLButtonDown(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_LBUTTONUP:
            return ptv->OnLButtonUp(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_MOUSEMOVE:
            return ptv->OnMouseMove(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            //
        case TXM_OPENFILE:
            return ptv->OpenFile(reinterpret_cast<wchar_t*>(lParam));

        case TXM_CLEAR:
            return ptv->ClearFile();
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
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

ATOM RegisterTextView(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.lpfnWndProc = TextViewWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(TextView*);
    wcex.hInstance = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor = 0;
    wcex.hbrBackground = (HBRUSH)(0);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = TEXTVIEW_CLASS;
    wcex.hIconSm = 0;

    return RegisterClassExW(&wcex);
}