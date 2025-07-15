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
    buffers.size() - 1, // BufferIndex
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
    buffers.size() - 1, // BufferIndex
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

  length += piece.length;
  lineCount += piece.lineFeedCnt;

  return true;
}

bool PieceTree::EraseText(size_t offset, size_t erase_length) noexcept
{
  if (rootNode->right == nullptr || offset + erase_length > length) return false;
  const size_t original_offset = offset, original_erase_length = erase_length;
  size_t left_node_offset = 0, right_node_offset = 0;
  TreeNode* left = GetNodePosition(offset, left_node_offset), * right = GetNodePosition(offset + erase_length, right_node_offset);
  if (left == right) // in a piece
  {
    if (left_node_offset != 0 && right_node_offset != left->piece.length) // case1: in the middle of a piece
    {
      Piece& piece1 = left->piece;
      Piece piece2 = piece1;
      size_t lineIndex1 = getLineIndexFromOffset(buffers[piece1.bufferIndex].lineStarts, left_node_offset);
      size_t lineIndex2 = getLineIndexFromOffset(buffers[piece1.bufferIndex].lineStarts, right_node_offset);
      piece1.end = BufferPosition(lineIndex1, left_node_offset - buffers[piece1.bufferIndex].lineStarts[lineIndex1]);
      piece1.length = left_node_offset;
      piece1.lineFeedCnt = lineIndex1;
      piece2.start = BufferPosition(lineIndex2, right_node_offset - buffers[piece1.bufferIndex].lineStarts[lineIndex2]);
      piece2.length -= left_node_offset + erase_length;
      piece2.lineFeedCnt = piece2.end.index - piece2.start.index;

      auto newNode = std::make_unique<TreeNode>(piece2, left);
      newNode->right = std::move(left->right);
      left->right = std::move(newNode);

      length -= erase_length;
      lineCount -= lineIndex2 - lineIndex1;

    }
    else // case2: from head or to tail of piece
    {
      //if(erase_length == right_node_offset) // delete whole piece

      Piece& piece = left->piece;
      const Piece original_piece = piece;
      size_t lineIndex1 = getLineIndexFromOffset(buffers[piece.bufferIndex].lineStarts, left_node_offset);
      size_t lineIndex2 = getLineIndexFromOffset(buffers[piece.bufferIndex].lineStarts, right_node_offset);
      piece.start = BufferPosition(lineIndex1, left_node_offset - buffers[piece.bufferIndex].lineStarts[lineIndex1]);
      piece.end = BufferPosition(lineIndex2, right_node_offset - buffers[piece.bufferIndex].lineStarts[lineIndex2]);

      piece.length -= erase_length;
      piece.lineFeedCnt = lineIndex2 - lineIndex1;

      length -= erase_length;
      lineCount -= original_piece.lineFeedCnt - piece.lineFeedCnt;
    }
  }
  else // case3: several pieces
  {
    TreeNode* leftmost = left;
    if (left_node_offset == 0)
    {
      leftmost = left->left;
    }
    else // shrink first piece.
    {
      Piece& piece = left->piece;
      const Piece original_piece = piece;
      size_t lineIndex = getLineIndexFromOffset(buffers[piece.bufferIndex].lineStarts, left_node_offset);
      piece.end = BufferPosition(lineIndex, left_node_offset - buffers[piece.bufferIndex].lineStarts[lineIndex]);
      piece.length = left_node_offset;
      piece.lineFeedCnt = lineIndex;
      length -= original_piece.length - piece.length;
      lineCount -= original_piece.lineFeedCnt - piece.lineFeedCnt;
    }
    TreeNode* pre = leftmost;
    while (left != right)
    {
      const Piece& piece = left->piece;
      length -= piece.length;
      lineCount -= piece.lineFeedCnt;
      pre = left;
      left = left->right.get();
    }
    leftmost->right = std::move(pre->right); // delete all pieces except the last one.

    // then handle the last piece.
    if (right_node_offset != right->piece.length) // shrink it.
    {
      Piece& piece = right->piece;
      const Piece original_piece = piece;
      size_t lineIndex = getLineIndexFromOffset(buffers[piece.bufferIndex].lineStarts, right_node_offset);
      piece.start = BufferPosition(lineIndex, right_node_offset - buffers[piece.bufferIndex].lineStarts[lineIndex]);
      piece.length -= right_node_offset;
      piece.lineFeedCnt = piece.end.index - lineIndex;

      length -= original_piece.length - piece.length;
      lineCount -= original_piece.lineFeedCnt - piece.lineFeedCnt;
    }
    else // delete whole piece.
    {
      const Piece& piece = right->piece;
      length -= piece.length;
      lineCount -= piece.lineFeedCnt;
      leftmost->right = std::move(right->right);
    }

  }
  return true;
}

bool PieceTree::ReplaceText(size_t offset, std::vector<unsigned char> input, size_t erase_length) noexcept
{
  return false;
}

TreeNode* PieceTree::GetNodePosition(size_t offset, size_t& inNodeOffset) noexcept
{
  const size_t originalOffset = offset;
  if (offset > length) return nullptr;
  // assume rootNode->right is not nullptr.
  TreeNode* ptr = rootNode->right.get();
  while (offset > ptr->piece.length)
  {
    offset -= ptr->piece.length;
    ptr = ptr->right.get();
  }
  inNodeOffset = offset;
  return ptr;
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
