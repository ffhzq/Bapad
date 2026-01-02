#include "pch.h"
#include "TextView.h"

bool IsKeyPressed(UINT nVirtKey) noexcept
{
  return GetKeyState(nVirtKey) < 0 ? true : false;
}

void TextView::MoveCharNext()
{
  size_t  linestartCharOffset = 0;
  size_t  lineLenghCharOffset = 0;
  TextIterator itor = pTextDoc->IterateLineByLineNumber(currentLine , &linestartCharOffset, &lineLenghCharOffset);
  auto lineContent = itor.GetLine();
  const int index = cursorOffset - linestartCharOffset;

  if (index + 1 > lineContent.size())
  {
    ;
  }
  else if (index + 2 > lineContent.size() && currentLine + 1 < lineCount)
  {    
    const size_t lineCount = pTextDoc->GetLineCount();
    pTextDoc->IterateLineByLineNumber(currentLine + 1, &cursorOffset, nullptr);
  }
  else
  {
    if (currentLine + 1 < lineCount && lineLenghCharOffset - 2 == cursorOffset) // if hit CRLF, then go next line
    {
      pTextDoc->IterateLineByLineNumber(currentLine + 1, &cursorOffset, nullptr);
    }
    else
      cursorOffset += 1;
  }
  SyncMetrics(TRUE);
}

void TextView::MoveCharPrev() 
{
  size_t  lineNumber = 0;
  size_t  linestartCharOffset = 0;
  TextIterator itor = pTextDoc->IterateLineByCharOffset(cursorOffset, &lineNumber, &linestartCharOffset);
  auto lineContent = itor.GetLine();
  const int index = cursorOffset - linestartCharOffset;

  if (index == 0)
  {
    if (lineNumber == 0)
    {
      ;
    }
    else
    {
      cursorOffset -= 2; // CRLF 2 chars
      currentLine -= 1;
    }
  }
  else
  {
    cursorOffset -= 1;
  }
  SyncMetrics(TRUE);
}
void TextView::MoveLineUp(int numOfLines) 
{
  size_t linestartCharOffset = 0;
  size_t newLinestartCharOffset = 0;
  size_t newLineLenghCharOffset = 0;

  auto originalItor = pTextDoc->IterateLineByLineNumber(currentLine, &linestartCharOffset, nullptr);
  const int lineIndexOffset = cursorOffset - linestartCharOffset;
  if (currentLine < numOfLines)
    numOfLines = currentLine;

  auto itor = pTextDoc->IterateLineByLineNumber(currentLine - numOfLines, &newLinestartCharOffset, &newLineLenghCharOffset);
  if (newLineLenghCharOffset > lineIndexOffset)
    cursorOffset = newLinestartCharOffset + lineIndexOffset;
  else
    cursorOffset = newLinestartCharOffset + newLineLenghCharOffset;
  if (currentLine - numOfLines != lineCount - 1 && lineIndexOffset + 2 > newLineLenghCharOffset)
  {
    cursorOffset = newLinestartCharOffset + newLineLenghCharOffset - 2;
  }
  SyncMetrics(TRUE);
}
void TextView::MoveLineDown(int numOfLines) 
{
  const int n = pTextDoc->GetLineCount();
  size_t linestartCharOffset = 0;
  size_t newLinestartCharOffset = 0;
  size_t newLineLenghCharOffset = 0;

  auto originalItor = pTextDoc->IterateLineByLineNumber(currentLine, &linestartCharOffset, nullptr);
  const int lineIndexOffset = cursorOffset - linestartCharOffset;
  if (currentLine + numOfLines >= n)
    numOfLines = n - currentLine - 1;
  auto itor = pTextDoc->IterateLineByLineNumber(currentLine + numOfLines, &newLinestartCharOffset, &newLineLenghCharOffset);
  if (newLineLenghCharOffset > lineIndexOffset)
    cursorOffset = newLinestartCharOffset + lineIndexOffset;
  else
    cursorOffset = newLinestartCharOffset + newLineLenghCharOffset;
  if (currentLine + numOfLines != lineCount - 1 && lineIndexOffset + 2 > newLineLenghCharOffset)
  {
    cursorOffset = newLinestartCharOffset + newLineLenghCharOffset - 2;
  }
  SyncMetrics(TRUE);
}
void TextView::MoveLineStart(int lineNumber) 
{
  size_t newLinestartCharOffset = 0;
  auto itor = pTextDoc->IterateLineByLineNumber(lineNumber, &newLinestartCharOffset, nullptr);
  cursorOffset = newLinestartCharOffset + 0;
  SyncMetrics(TRUE);
}
void TextView::MoveLineEnd(int lineNumber) 
{
  size_t newLinestartCharOffset = 0;
  size_t newLineLenghCharOffset = 0;
  auto itor = pTextDoc->IterateLineByLineNumber(lineNumber, &newLinestartCharOffset, &newLineLenghCharOffset);
  cursorOffset = newLinestartCharOffset + newLineLenghCharOffset;
  if (lineCount > lineNumber + 1)
    cursorOffset -= 2;
  SyncMetrics(TRUE);
}