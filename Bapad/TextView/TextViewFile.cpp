#include "pch.h"
#include "TextView.h"


//
//	
//
LONG TextView::OpenFile(WCHAR *szFileName)
{
	ClearFile();

	if(textDoc->init(szFileName))
	{
		lineCount   = textDoc->getLinecount();
		longestLine = textDoc->getLongestline(4);

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
	if(textDoc)
		textDoc->clear();

	lineCount   = textDoc->getLinecount();
	longestLine = textDoc->getLongestline(4);

	vScrollPos  = 0;
	hScrollPos  = 0;

	UpdateMetrics();

	return TRUE;
}