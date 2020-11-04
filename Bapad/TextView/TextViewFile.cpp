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

