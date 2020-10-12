// Bapad.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Bapad.h"



#define MAX_LOADSTRING 100

// Global Variables:

const wchar_t ClassName[] = L"Bapad";

HINSTANCE hInst;// current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


HWND		hwndMain;
HWND		hwndTextView;

WCHAR szFileName[MAX_PATH];
WCHAR szFileTitle[MAX_PATH];


// Forward declarations of functions included in this code module:
ATOM                RegisterMainWindow(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL ShowOpenFileDlg(HWND hWnd, wchar_t* fileName, wchar_t* titleName);
void ShowAboutDlg(HWND hWndParent);
void SetWindowFileName(HWND hWnd, wchar_t* fileName);
BOOL DoOpenFile(HWND hWnd, WCHAR* fileName, WCHAR* fileTitle);
void HandleDropFiles(HWND hWnd, HDROP hDrop);



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


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM RegisterMainWindow(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BAPAD));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_BAPAD);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable
    hwndMain = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hwndMain)
    {
        return FALSE;
    }

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  XXXXXXXXXX                     WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int width = 0, height = 0;
    HFONT hFont;

    switch (message)
    {
        case WM_CREATE:
            hwndTextView = CreateTextView(hWnd);

            hFont = CreateFontW(-13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Courier New");

            // change the font
            SendMessageW(hwndTextView, WM_SETFONT, (WPARAM)hFont, 0);

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
                    TextView_Clear(hwndTextView);

                    break;
                }
                // get a filename to open
                case IDM_FILE_OPEN:
                    if (ShowOpenFileDlg(hWnd, szFileName, szFileTitle))
                    {
                        DoOpenFile(hWnd, szFileName, szFileTitle);
                    }
                    break;
                


                case IDM_FILE_EXIT:
                    DestroyWindow(hWnd);
                    break;

                case IDM_HELP_ABOUT:
                    DialogBoxW(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);//ShowAboutDlg(hwnd);
                    break;
            }
        }
        break;


        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_SETFOCUS:
            SetFocus(hwndTextView);
            break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_SIZE:
            width = (short)LOWORD(lParam);
            height = (short)HIWORD(lParam);

            MoveWindow(hwndTextView, 0, 0, width, height, TRUE);
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
    MessageBox(hwndParent,
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
    if (TextView_OpenFile(hwndTextView, szFileName))
    {
        SetWindowFileName(hwnd, szFileTitle);
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