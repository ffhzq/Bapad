// Bapad.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Bapad.h"

HWND		g_hwndMain;
HWND		g_hwndTextView;
HFONT		g_hFont;

void ShowProperties(HWND hwndParent);
void LoadRegSettings();
void SaveRegSettings();

HINSTANCE hInst;// current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

WCHAR szFileName[MAX_PATH];
WCHAR szFileTitle[MAX_PATH];


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    //MessageBox(NULL, L"Goodbye, cruel world!", L"Note", MB_OK);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_BAPAD, szWindowClass, MAX_LOADSTRING);
    RegisterMainWindow(hInstance);
    RegisterTextView(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BAPAD));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM RegisterMainWindow(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{ 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BAPAD));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)0;// (COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDM_);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    g_hwndMain = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!g_hwndMain)
    {
        return FALSE;
    }

    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int width = 0, height = 0;

    switch (message)
    {
        case WM_CREATE:
            g_hwndTextView = CreateTextView(hWnd);

            // automatically create new document when we start
            PostMessageW(hWnd, WM_COMMAND, IDM_FILE_NEW, 0);

            // tell windows that we can handle drag+drop'd files
            DragAcceptFiles(hWnd, TRUE);
            break;

        case WM_DROPFILES:
            HandleDropFiles(hWnd, (HDROP)wParam);
            break;

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_FILE_NEW:
                {
                    wchar_t a[] = L"Untitled";
                    SetWindowFileName(hWnd, a);
                    TextView_Clear(g_hwndTextView);

                    break;
                }
                // get a filename to open
                case IDM_FILE_OPEN:
                    if (ShowOpenFileDlg(hWnd, szFileName, szFileTitle))
                    {
                        DoOpenFile(hWnd, szFileName, szFileTitle);
                    }
                    break;
                case IDM_VIEW_FONT:
                    ShowProperties(hWnd);
                    break;
                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;

                case IDM_HELP_ABOUT:
                    ShowAboutDlg(hWnd);
                    //DialogBoxW(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
            }
        }
        break;


        case WM_DESTROY:
            PostQuitMessage(0);
            DeleteObject(g_hFont);
            break;
        case WM_SETFOCUS:
            SetFocus(g_hwndTextView);
            break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_SIZE:
            width = (short)LOWORD(lParam);
            height = (short)HIWORD(lParam);

            MoveWindow(g_hwndTextView, 0, 0, width, height, TRUE);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}




// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}


BOOL ShowOpenFileDlg(HWND hwnd, wchar_t* pstrFileName, wchar_t* pstrTitleName)
{
    const wchar_t* szFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";

    //IFileOpenDialog

    OPENFILENAMEW ofn = { sizeof(ofn) };

    ofn.hwndOwner = hwnd;
    ofn.hInstance = GetModuleHandle(0);
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = pstrFileName;
    ofn.lpstrFileTitle = pstrTitleName;

    ofn.nFilterIndex = 1;
    ofn.nMaxFile = _MAX_PATH;
    ofn.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;

    // flags to control appearance of open-file dialog
    ofn.Flags = OFN_EXPLORER |
        OFN_ENABLESIZING |
        OFN_ALLOWMULTISELECT |
        OFN_FILEMUSTEXIST;

    return GetOpenFileName(&ofn);
}

void ShowAboutDlg(HWND hwndParent)
{
    MessageBoxW(hwndParent,
        ClassName,//+ L"\r\n\r\n",
        ClassName,
        MB_OK | MB_ICONINFORMATION
    );
}

void SetWindowFileName(HWND hwnd, wchar_t* szFileName)
{
    wchar_t ach[MAX_PATH + sizeof(ClassName) + 4];

    wsprintf(ach, _T("%s - %s"), szFileName, ClassName);
    SetWindowTextW(hwnd, ach);
}

BOOL DoOpenFile(HWND hwnd, WCHAR* szFileName, WCHAR* szFileTitle)
{
    int fmt, fmtlook[] =
    {
        IDM_VIEW_ASCII, IDM_VIEW_UTF8, IDM_VIEW_UTF16, IDM_VIEW_UTF16BE
    };

    if (TextView_OpenFile(g_hwndTextView, szFileName))
    {
        SetWindowFileName(hwnd, szFileTitle);

        fmt = static_cast<int>(TextView_GetFormat(g_hwndTextView));

        CheckMenuRadioItem(GetMenu(hwnd),
            IDM_VIEW_ASCII, IDM_VIEW_UTF16BE,
            fmtlook[fmt], MF_BYCOMMAND);

        return TRUE;
    }
    else
    {
        MessageBoxW(hwnd, _T("Error opening file"), ClassName, MB_ICONEXCLAMATION);
        return FALSE;
    }
}

//
//	How to process WM_DROPFILES
//
void HandleDropFiles(HWND hwnd, HDROP hDrop)
{
    wchar_t buf[MAX_PATH];
    wchar_t* name;

    if (DragQueryFileW(hDrop, 0, buf, MAX_PATH))
    {
        wcscpy_s(szFileName, buf);

        name = wcsrchr(szFileName, '\\');
        wcscpy_s(szFileTitle, name ? name + 1 : buf);

        DoOpenFile(hwnd, szFileName, szFileTitle);
    }

    DragFinish(hDrop);
}
