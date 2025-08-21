#include "TextView.h"

bool IsKeyPressed(UINT nVirtKey) noexcept
{
  return GetKeyState(nVirtKey) < 0 ? true : false;
}