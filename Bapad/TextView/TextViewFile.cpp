#include "TextView.h"

//
//	
//
LONG TextView::OpenFile(TCHAR *szFileName)
{
	ClearFile();

	if(m_pTextDoc->init(szFileName))
	{
		m_nLineCount   = m_pTextDoc->linecount();
		m_nLongestLine = m_pTextDoc->longestline(4);

		m_nVScrollPos  = 0;
		m_nHScrollPos  = 0;

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

	m_nLineCount   = m_pTextDoc->linecount();
	m_nLongestLine = m_pTextDoc->longestline(4);

	m_nVScrollPos  = 0;
	m_nHScrollPos  = 0;

	UpdateMetrics();

	return TRUE;
}
