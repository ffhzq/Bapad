#pragma once

//
//	ATTR - text character attribute
//	
struct  ATTR
{
	COLORREF	fg;			// foreground colour
	COLORREF	bg;			// background colour
	ULONG		style;		// possible font-styling information

};

//
//	FONT - font attributes
//
struct FONT
{
	// Windows font information
	HFONT		hFont;
	TEXTMETRIC	tm;

	// dimensions needed for control-character 'bitmaps'
	int			nInternalLeading;
	int			nDescent;

};
