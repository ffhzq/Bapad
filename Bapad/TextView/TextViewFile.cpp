#include "pch.h"
#include "TextView.h"


//
//	
//
LONG TextView::OpenFile(WCHAR *szFileName)
{
	ClearFile();

	if(pTextDoc->init(szFileName))
	{
		lineCount   = pTextDoc->getLinecount();
		longestLine = pTextDoc->getLongestline(4);

		vScrollPos  = 0;
		hScrollPos  = 0;

		UpdateMetrics();
		return TRUE;
	}

	return FALSE;
}

//
//
//
LONG TextView::ClearFile()
{
	if(pTextDoc)
		pTextDoc->clear();

	lineCount   = pTextDoc->getLinecount();
	longestLine = pTextDoc->getLongestline(4);

	vScrollPos  = 0;
	hScrollPos  = 0;

	UpdateMetrics();

	return TRUE;
}

LONG TextView::AddFont(HFONT)
{
	return 0;
}

LONG TextView::SetFont(HFONT, int idx)
{
	return 0;
}

LONG TextView::SetLineSpacing(int nAbove, int nBelow)
{
	return 0;
}
