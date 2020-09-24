#include "TextView.h"

//
//	
//
LONG TextView::OpenFile(TCHAR *szFileName)
{
	ClearFile();

	if(m_pTextDoc->init(szFileName))
	{
		lineCount   = m_pTextDoc->linecount();
		longestLine = m_pTextDoc->longestline(4);

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
	if(m_pTextDoc)
		m_pTextDoc->clear();

	lineCount   = m_pTextDoc->linecount();
	longestLine = m_pTextDoc->longestline(4);

	vScrollPos  = 0;
	hScrollPos  = 0;

	UpdateMetrics();

	return TRUE;
}
