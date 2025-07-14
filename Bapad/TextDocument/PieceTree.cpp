#include "pch.h"
#include "PieceTree.h"

enum class CharCode {
  /**
   * The `\n` character.   */

  LineFeed = 10,
  /**
   * The `\r` character.
   */
  CarriageReturn = 13,
};

PieceTree::PieceTree(std::vector<unsigned char> input) : buffers{}, rootNode(std::make_unique<TreeNode>()), length(0), lineCount(1)
{
  buffers.emplace_back(Buffer(std::vector<unsigned char>()));
  buffers.emplace_back(Buffer(input));

  const Buffer& buffer = buffers.back();
  const Piece piece{
    BufferPosition(0,0), // startPos
    BufferPosition(buffer.lineStarts.size() - 1, // endPos
      buffer.value.size() - buffer.lineStarts.back()),
    buffers.size(), // BufferIndex
    buffer.value.size(), // length
    buffer.lineStarts.size() - 1}; // lineCount

  rootNode.get()->left = nullptr;
  rootNode.get()->right = std::make_unique<TreeNode>(piece, rootNode.get());
  length += piece.length;
  lineCount += piece.lineFeedCnt;
}

PieceTree::~PieceTree() noexcept
{}

bool PieceTree::InsertText(size_t offset, std::vector<unsigned char> input) noexcept
{
  if (offset > this->length)
  {
    return false;
  }
  size_t pieceOffset = 0;
  TreeNode* nodePos = GetNodePosition(offset, pieceOffset);
  buffers.emplace_back(Buffer(input));
  const Buffer& buffer = buffers.back();

  if (pieceOffset != nodePos->piece.length) // insert inside a piece, split then insert
  {
    nodePos = SplitPiece(nodePos, pieceOffset);
    // then insert new node behind nodePos(node1)
  }
  //insert
  const Piece piece{
    BufferPosition(0,0), // startPos
    BufferPosition(buffer.lineStarts.size() - 1, // endPos
      buffer.value.size() - buffer.lineStarts.back()),
    buffers.size(), // BufferIndex
    buffer.value.size(), // length
    buffer.lineStarts.size() - 1}; // lineCount

  auto NewNode = std::make_unique<TreeNode>(piece, nodePos);
  if (nodePos->right == nullptr)
  {
    NewNode->right = nullptr;
  }
  else
  {
    NewNode->right = std::move(nodePos->right);
    NewNode->right->left = NewNode.get();
  }
  nodePos->right = std::move(NewNode);

  return true;
}

bool PieceTree::EraseText(size_t offset, size_t length) noexcept
{
  return false;
}

bool PieceTree::ReplaceText(size_t offset, std::vector<unsigned char> input, size_t erase_length) noexcept
{
  return false;
}

TreeNode* PieceTree::GetNodePosition(size_t offset, size_t& inNodeOffset) noexcept
{
  const size_t originalOffset = offset;
  if (offset > length) return nullptr;
  if (offset == 0) return rootNode.get();// special case 1: in the tail of root piece. // inNodeOffset is no need
  // assume rootNode->right is not nullptr.
  TreeNode* ptr = rootNode->right.get();
  while (ptr != nullptr)
  {
    if (offset >= 0 && offset >= ptr->piece.length)
    {
      offset -= ptr->piece.length;
      if (offset == 0) // case 1: in the tail of a piece.
      {
        inNodeOffset = ptr->piece.length;
        return ptr;
      }
      ptr = ptr->right.get();
    }
    else // case 2: inside a piece.
    {
      inNodeOffset = offset;
      return ptr;
    }
  }
  return nullptr;
}

TreeNode* PieceTree::SplitPiece(TreeNode* currNode, const size_t inNodeOffset)
{
  auto parrentNode = currNode->left;
  size_t offset = inNodeOffset;
  size_t linePos = 0;
  const Buffer& currBuffer = buffers[currNode->piece.bufferIndex];
  const Piece& currPiece = currNode->piece;


  size_t lineIndex = getLineIndexFromOffset(currBuffer.lineStarts, offset);
  Piece p1{
  currPiece.start,
  BufferPosition{lineIndex, offset - currBuffer.lineStarts[lineIndex]},
  currPiece.bufferIndex,
  inNodeOffset,
  lineIndex
  };
  Piece p2{
     BufferPosition{lineIndex, offset - currBuffer.lineStarts[lineIndex] + 1}, // p1.end + 1
     currPiece.end,
     currPiece.bufferIndex,
     currPiece.length - inNodeOffset,
     currPiece.lineFeedCnt - lineIndex
  };

  auto Node1 = std::make_unique<TreeNode>(p1, parrentNode);
  auto Node2 = std::make_unique<TreeNode>(p2, Node1.get());

  Node2->right = std::move(currNode->right);
  currNode->right = nullptr;
  Node1->right = std::move(Node2);
  parrentNode->right = std::move(Node1); // then currNode has been deconstructed.
  currNode = nullptr;
  return parrentNode->right.get();
}

std::vector<size_t> createLineStarts(const std::vector<unsigned char>& str)
{
  std::vector<size_t> lineStarts{0}; // first line starts in 0.
  const size_t length = str.size();
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
  return lineStarts;
}

size_t getLineIndexFromOffset(const std::vector<size_t>& lineStarts, size_t offset)
{
  if (lineStarts.empty())
  {
    throw;
  }
  auto it = std::upper_bound(lineStarts.begin(), lineStarts.end(), offset);

  const size_t lineIndex = std::distance(lineStarts.begin(), it);

  if (it == lineStarts.begin())
  {
    return 0;
  }
  else
  {
    return lineIndex - 1;
  }
}
