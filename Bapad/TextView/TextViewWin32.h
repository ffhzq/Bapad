#pragma once
#include "pch.h"

//	TextView API declared here
//
ATOM RegisterTextView(HINSTANCE hInstance);
HWND CreateTextView(HWND hwndParent);

//
//	TextView Window Messages defined here
//
#define TXM_BASE         (WM_USER)
#define TXM_OPENFILE     (TXM_BASE + 0)
#define TXM_CLEAR        (TXM_BASE + 1)

//
//	TextView Message Macros defined here
//
#define TextView_OpenFile(hwndTV, szFile)	SendMessage((hwndTV), TXM_OPENFILE, 0, (LPARAM)(szFile))
#define TextView_Clear(hwndTV)				SendMessage((hwndTV), TXM_CLEAR, 0, 0)

//
//	TextView Macros defined here
//
#define TEXTVIEW_CLASS L"TextView32"

//
//	TextView colours
//
#define TXC_BACKGROUND		0			// normal background colour
#define TXC_FOREGROUND		1			// normal foreground colour

const size_t TXC_MAX_COLOURS = 6;			// keep this updated!
