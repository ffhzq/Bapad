#pragma once

//
//	ATTR - text character attribute
//	
struct  ATTR
{
    ATTR()
        :
        fg(0),
        bg(0),
        style(0)
    {
    }
    COLORREF	fg;			// foreground colour
    COLORREF	bg;			// background colour
    ULONG		style;		// possible font-styling information

};

//
//	FONT - font attributes
//
struct FONT
{
    FONT()
        :
        hFont(0),
        tm({ 0 }),
        nInternalLeading(0),
        nDescent(0)
    {
    }
    // Windows font information
    HFONT		hFont;
    TEXTMETRIC	tm;

    // dimensions needed for control-character 'bitmaps'
    int			nInternalLeading;
    int			nDescent;

};
