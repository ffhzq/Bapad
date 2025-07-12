#include "PieceTree.h"

enum class CharCode {
  /**
   * The `\n` character.
   */
  LineFeed = 10,
  /**
   * The `\r` character.
   */
  CarriageReturn = 13,
};

PieceTree::PieceTree(std::vector<unsigned char> input) : buffers{}, rootNode()
{
  buffers.push_back(Buffer());
  buffers.emplace_back(Buffer(input));
  Piece piece{BufferPosition(0,0),BufferPosition(buffers[1].lineStarts.size() - 1,buffers[1].lineStarts[buffers[1].lineStarts.size() - 1]),
    1,
    buffers[1].value.size(),buffers[1].lineStarts.size() - 1};
  rootNode.left = nullptr;
  rootNode.right = new Node(piece, &rootNode, nullptr);
}

PieceTree::~PieceTree() noexcept
{
  Node* i = rootNode.right;
  while (i != nullptr)
  {
    Node* cur = i;
    i = i->right;
    delete cur;
  }
}

bool PieceTree::InsertText(size_t offset, unsigned char* str, size_t length) noexcept
{
  return false;
}

bool PieceTree::EraseText(size_t offset, size_t length) noexcept
{
  return false;
}

bool PieceTree::ReplaceText(size_t offset, unsigned char* str, size_t length, size_t erase_length) noexcept
{
  return false;
}

size_t PieceTree::GetNodeIndex(size_t offset)
{
  return size_t();
}

std::vector<size_t> createLineStarts(const std::vector<unsigned char>& str)
{
  std::vector<size_t> lineStarts;
  size_t length = str.size();
  for (size_t i = 0; i < length; ++i)
  {
    auto ch = str[i];
    if (ch == static_cast<unsigned int>(CharCode::CarriageReturn))
    {
      if (i + 1 < length && str[i + 1] == static_cast<unsigned int>(CharCode::LineFeed))
      {
        // \r\n
        lineStarts.push_back(i + 2);
        ++i;
      }
      else
      {
        // \r
        lineStarts.push_back(i + 1);
      }

    }
    else if (ch == static_cast<unsigned int>(CharCode::LineFeed))
    {
      // \n
      lineStarts.push_back(i + 1);
    }
  }
  if (lineStarts.empty()) lineStarts.push_back(0);
  return lineStarts;
}