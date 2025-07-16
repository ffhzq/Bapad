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
      left = left->right.get();
    }
    TreeNode* pre = nullptr;
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
  if (EraseText(offset, erase_length) == false)
  {
    return false;
  }
  if (InsertText(offset, input) == false)
  {
    return false;
  }
  return true;
}

TreeNode* PieceTree::GetNodePosition(size_t offset, size_t& inPieceOffset) noexcept
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
  inPieceOffset = offset;
  return ptr;
}

TreeNode* PieceTree::SplitPiece(TreeNode* currNode, const size_t inPieceOffset)
{
  const Buffer& currBuffer = buffers[currNode->piece.bufferIndex];
  const Piece original_piece = currNode->piece;
  Piece& current_piece = currNode->piece;

  const size_t lineIndex = getLineIndexFromOffset(currBuffer.lineStarts, inPieceOffset - 1);
  current_piece.end = BufferPosition{lineIndex, inPieceOffset - currBuffer.lineStarts[lineIndex] - 1};
  current_piece.length = inPieceOffset;
  current_piece.lineFeedCnt = lineIndex;

  const size_t lineIndex2 = getLineIndexFromOffset(currBuffer.lineStarts, inPieceOffset);
  Piece p2{
     BufferPosition{lineIndex2, inPieceOffset - currBuffer.lineStarts[lineIndex2]}, // p1.end + 1
     original_piece.end,
     original_piece.bufferIndex,
     original_piece.length - current_piece.length,
     original_piece.lineFeedCnt - current_piece.lineFeedCnt
  };

  auto Node2 = std::make_unique<TreeNode>(p2, currNode);
  Node2->right = std::move(currNode->right);
  if (Node2->right != nullptr) Node2->right->left = Node2.get();
  currNode->right = std::move(Node2);
  return currNode;
}

std::vector<unsigned char> PieceTree::GetText(size_t offset, size_t text_length)
{
  auto text = std::vector<unsigned char>();
  if (offset >= length) return text;
  if (offset + text_length > length) text_length = length - offset;
  if (text_length == 0) return text;
  text.reserve(text_length);

  size_t in_piece_offset_start = 0;
  TreeNode* current_node = GetNodePosition(offset, in_piece_offset_start);
  if (current_node == nullptr) return text;
  if (current_node->piece.length == in_piece_offset_start)
  {
    current_node = current_node->right.get();
    in_piece_offset_start = 0;
  }
  size_t bytes_copied = 0;
  while (current_node != nullptr && bytes_copied < text_length)
  {
    const Piece& piece = current_node->piece;
    const Buffer& buffer = buffers[piece.bufferIndex];
    const std::vector<unsigned char>& buffer_value = buffer.value;

    in_piece_offset_start = (bytes_copied == 0 ? in_piece_offset_start : 0); // first piece or not
    const size_t piece_start = buffer.lineStarts[piece.start.index] + piece.start.offset;
    const size_t piece_end = buffer.lineStarts[piece.end.index] + piece.end.offset;

    const size_t remain_bytes_to_be_copied = text_length - bytes_copied;
    const size_t available_bytes_in_piece = piece.length - in_piece_offset_start;
    const size_t bytes_to_be_copied = (std::min)(available_bytes_in_piece, remain_bytes_to_be_copied);

    text.insert(text.end(),
      buffer_value.begin() + piece_start + in_piece_offset_start,
      buffer_value.begin() + piece_start + in_piece_offset_start + bytes_to_be_copied);

    bytes_copied += bytes_to_be_copied;
    current_node = current_node->right.get();
  }
  return text;
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

size_t getLineIndexFromOffset(const std::vector<size_t>& lineStarts, size_t inPieceOffset)
{
  if (lineStarts.empty())
  {
    throw;
  }
  auto it = std::upper_bound(lineStarts.begin(), lineStarts.end(), inPieceOffset);

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
