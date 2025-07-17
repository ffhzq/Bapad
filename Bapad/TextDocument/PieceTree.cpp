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

bool PieceTree::InsertText(size_t offset, std::vector<unsigned char> input)
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

bool PieceTree::EraseText(size_t offset, size_t erase_length)
{
  if (rootNode->right == nullptr || offset + erase_length > length) return false;
  const size_t original_offset = offset, original_erase_length = erase_length;
  size_t in_piece_offset_start = 0;
  TreeNode* current_node = GetNodePosition(offset, in_piece_offset_start);
  TreeNode* parrent = current_node->left;
  if (current_node == rootNode.get() || parrent == nullptr) return false;
  current_node = parrent;
  size_t bytes_erased = 0;
  while (current_node != nullptr && bytes_erased < erase_length)
  {
    current_node = current_node->right.get();
    if (in_piece_offset_start != 0) // if erase from middle, then split piece from middle
    {
      current_node = SplitPiece(current_node, in_piece_offset_start);
      parrent = current_node;
      current_node = current_node->right.get();
      in_piece_offset_start = 0;
    }

    const Piece& piece = current_node->piece;
    const Buffer& buffer = gsl::at(buffers, piece.bufferIndex);
    const std::vector<unsigned char>& buffer_value = buffer.value;
    const size_t remain_bytes_to_be_erased = erase_length - bytes_erased;
    const size_t available_bytes_in_piece = piece.length - in_piece_offset_start;
    const size_t bytes_to_be_erased = (std::min)(available_bytes_in_piece, remain_bytes_to_be_erased);

    ShrinkPiece(current_node, bytes_to_be_erased, 0);
    bytes_erased += bytes_to_be_erased;
  }
  if (current_node == nullptr)
  {
    parrent->right = nullptr;
  }
  else
  {
    parrent->right = current_node->piece.length == 0 ? std::move(current_node->right) : std::move(current_node->left->right);
  }

  return true;
}

bool PieceTree::ReplaceText(size_t offset, std::vector<unsigned char> input, size_t erase_length)
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

TreeNode* PieceTree::GetNodePosition(size_t offset, size_t& inPieceOffset) const noexcept
{
  const size_t originalOffset = offset;
  if (offset > length) return nullptr;
  TreeNode* ptr = rootNode->right.get();
  if (offset == 0)return ptr;
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
  const Buffer& currBuffer = gsl::at(buffers, currNode->piece.bufferIndex);
  const Piece original_piece = currNode->piece;
  Piece& current_piece = currNode->piece;

  const size_t lineIndex = getLineIndexFromOffset(currBuffer.lineStarts, inPieceOffset);
  current_piece.end = BufferPosition{lineIndex, inPieceOffset - gsl::at(currBuffer.lineStarts,lineIndex)};
  current_piece.length = inPieceOffset;
  current_piece.lineFeedCnt = lineIndex;
  Piece p2;
  if (inPieceOffset == 0)
  {
    p2 = original_piece;
  }
  else
  {
    const size_t lineIndex2 = getLineIndexFromOffset(currBuffer.lineStarts, inPieceOffset);
    p2 = Piece{
       BufferPosition{lineIndex2, inPieceOffset - gsl::at(currBuffer.lineStarts,lineIndex2)},
       original_piece.end,
       original_piece.bufferIndex,
       original_piece.length - current_piece.length,
       original_piece.lineFeedCnt - current_piece.lineFeedCnt
    };
  }
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
    const Buffer& buffer = gsl::at(buffers, piece.bufferIndex);
    const std::vector<unsigned char>& buffer_value = buffer.value;

    in_piece_offset_start = (bytes_copied == 0 ? in_piece_offset_start : 0); // first piece or not
    const size_t piece_start = gsl::at(buffer.lineStarts, piece.start.index) + piece.start.offset;
    const size_t piece_end = gsl::at(buffer.lineStarts, piece.end.index) + piece.end.offset;

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
// |->to right   <-to left|
void PieceTree::ShrinkPiece(TreeNode* current_node, size_t shrink_to_right, size_t shrink_to_left)
{
  Piece& piece = current_node->piece;
  const Piece original_piece = piece;
  auto& lineStarts = gsl::at(buffers, piece.bufferIndex).lineStarts;
  const size_t offset1 = gsl::at(lineStarts, piece.start.index) + piece.start.offset + shrink_to_right,
    offset2 = gsl::at(lineStarts, piece.end.index) + piece.end.offset - shrink_to_left;

  if (offset1 <= offset2)
  {
    const size_t lineIndex1 = getLineIndexFromOffset(lineStarts, offset1),
      lineIndex2 = getLineIndexFromOffset(lineStarts, offset2);

    const size_t line_offset1 = offset1 - gsl::at(lineStarts, lineIndex1),
      line_offset2 = offset2 - gsl::at(lineStarts, lineIndex1);

    piece.start = BufferPosition(lineIndex1, line_offset1);
    piece.end = BufferPosition(lineIndex2, line_offset2);
    piece.length = offset2 - offset1;
    piece.lineFeedCnt = lineIndex2 - lineIndex1;

    length -= original_piece.length - piece.length;
    lineCount -= original_piece.lineFeedCnt - piece.lineFeedCnt;
  }
  else
  {
    throw;
  }
}

std::vector<size_t> createLineStarts(const std::vector<unsigned char>& str)
{
  std::vector<size_t> lineStarts{0}; // first line starts in 0.
  const size_t length = str.size();
  for (size_t i = 0; i < length; ++i)
  {
    auto ch = gsl::at(str, i);
    if (ch == static_cast<unsigned int>(CharCode::CarriageReturn))
    {
      if (i + 1 < length && gsl::at(str, i + 1) == static_cast<unsigned int>(CharCode::LineFeed))
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
