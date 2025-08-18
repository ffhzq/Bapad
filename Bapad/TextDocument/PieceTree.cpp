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

PieceTree::PieceTree() noexcept : buffers{}, rootNode(), length(0), lineCount(1)
{
  auto input = std::vector<unsigned char>();
  Init(input);
}

PieceTree::PieceTree(std::vector<unsigned char> input) : buffers{}, rootNode(), length(0), lineCount(1)
{
  Init(input);
}


bool PieceTree::InsertText(size_t offset, std::vector<unsigned char> input)
{
  if (offset > this->length)
  {
    return false;
  }
  NodePosition node_pos = GetNodePosition(offset);
  const size_t pieceOffset = node_pos.in_piece_offset;
  TreeNode* nodePos = node_pos.node;
  auto& buffer = gsl::at(buffers, 0);
  const size_t start_offset = buffer.value.size();
  buffer.value.insert(buffer.value.end(), input.begin(), input.end());
  auto lineStarts = createLineStarts(input);
  for (auto& i : lineStarts)
  {
    i += start_offset;
  }
  if (nodePos)
  {
    buffer.lineStarts.insert(buffer.lineStarts.end(), lineStarts.begin() + 1, lineStarts.end());
    if (nodePos && pieceOffset != nodePos->piece.length) // insert inside a piece, split then insert
    {
      nodePos = SplitPiece(nodePos, pieceOffset);
      // then insert new node behind nodePos(node1)
    }
    //insert
    const size_t offset1 = start_offset, offset2 = offset1 + input.size();
    const size_t lineIndex1 = GetLineIndexFromNodePosistion(buffer.lineStarts, NodePosition(nodePos, 0)),
      lineIndex2 = GetLineIndexFromNodePosistion(buffer.lineStarts, NodePosition(nodePos, input.size()));
    const size_t line_offset1 = offset1 - gsl::at(buffer.lineStarts, lineIndex1),
      line_offset2 = offset2 - gsl::at(buffer.lineStarts, lineIndex1);
    const Piece piece{
      BufferPosition(lineIndex1,line_offset1), // startPos
      BufferPosition(lineIndex2,line_offset2),
      0, // BufferIndex
      input.size(), // length
      lineStarts.size() - 1}; // lineCount

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
  }
  else
  {
    buffer.lineStarts.insert(buffer.lineStarts.end(), lineStarts.begin() + 1, lineStarts.end());
    const Piece piece{
      BufferPosition(buffer.lineStarts.size() - lineStarts.size(), 0), // startPos
      BufferPosition(buffer.lineStarts.size() - 1, // endPos
       buffer.value.size() - lineStarts.back()),
      0, // BufferIndex
      input.size(), // length
      lineStarts.size() - 1}; // linefeed

    rootNode.get()->right = std::make_unique<TreeNode>(piece, rootNode.get());
    length = piece.length;
    lineCount = piece.lineFeedCnt + 1;
  }
  UpdateMetadata();
  return true;
}

bool PieceTree::EraseText(size_t offset, size_t erase_length)
{
  if (rootNode->right == nullptr || offset + erase_length > length) return false;
  const size_t original_offset = offset, original_erase_length = erase_length;

  NodePosition current_node_pos = GetNodePosition(offset);
  TreeNode* current_node = current_node_pos.node;
  size_t in_piece_offset_start = current_node_pos.in_piece_offset;
  TreeNode* parrent = current_node_pos.node->left;

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
    if (available_bytes_in_piece == 0) continue;
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
  UpdateMetadata();
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
  UpdateMetadata();
  return true;
}

NodePosition PieceTree::GetNodePositionAt(TreeNode* node, size_t offset) noexcept
{
  size_t inPieceOffset = offset;
  TreeNode* ptr = node;
  if (ptr == nullptr || inPieceOffset <= ptr->piece.length) return NodePosition(ptr, inPieceOffset);
  while (ptr && inPieceOffset > ptr->piece.length)
  {
    inPieceOffset -= ptr->piece.length;
    ptr = ptr->right.get();
  }
  return NodePosition(ptr, inPieceOffset);
}

NodePosition PieceTree::GetNodePosition(size_t offset) noexcept
{
  return GetNodePositionAt(rootNode->right.get(), offset);
}

size_t PieceTree::offsetInBuffer(size_t bufferIndex, BufferPosition pos)
{
  const auto& lineStarts = gsl::at(buffers, bufferIndex).lineStarts;
  return gsl::at(lineStarts, pos.line) + pos.column;
}

TreeNode* PieceTree::SplitPiece(TreeNode* currNode, const size_t inPieceOffset)
{
  const Buffer& currBuffer = gsl::at(buffers, currNode->piece.bufferIndex);
  const Piece original_piece = currNode->piece;
  Piece& current_piece = currNode->piece;

  const size_t lineIndex = GetLineIndexFromNodePosistion(currBuffer.lineStarts, NodePosition(currNode, inPieceOffset));
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
    const size_t lineIndex2 = GetLineIndexFromNodePosistion(currBuffer.lineStarts, NodePosition(currNode, inPieceOffset));
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

std::vector<unsigned char> PieceTree::GetText(size_t offset, size_t text_length) noexcept
{
  return GetTextAt(this->rootNode->right.get(), offset, text_length);
}

std::vector<unsigned char> PieceTree::GetTextAt(TreeNode* node, size_t offset, size_t text_length) noexcept
{
  auto text = std::vector<unsigned char>();
  if (offset >= length) return text;
  if (offset + text_length > length) text_length = length - offset;
  if (text_length == 0) return text;
  text.reserve(text_length);
  auto node_pos = GetNodePositionAt(node, offset);
  size_t in_piece_offset_start = node_pos.in_piece_offset;
  TreeNode* current_node = node_pos.node;
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
    const size_t piece_start = offsetInBuffer(piece.bufferIndex, piece.start);
    const size_t piece_end = offsetInBuffer(piece.bufferIndex, piece.end);

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
// lineNumber starts from 1
std::vector<unsigned char> PieceTree::GetLine(size_t lineNumber, const size_t endOffset, size_t* retValStartOffset)
{
  if (lineNumber > lineCount || lineCount == 0 || lineNumber == 0) return std::vector<unsigned char>();
  TreeNode* node = rootNode->right.get();
  size_t nodeStartOffset = 0; //search cache
  const size_t originalLineNumber = lineNumber;
  auto retVal = std::vector<unsigned char>();
  while (node)
  {
    if (node->piece.lineFeedCnt > lineNumber - 1) // lineContent within this piece
    {
      const size_t preAccValue = getAccumulatedValue(node, lineNumber - 2);
      const size_t AccValue = getAccumulatedValue(node, lineNumber - 1);
      const Buffer& buffer = gsl::at(buffers, node->piece.bufferIndex);
      const size_t startOffset = offsetInBuffer(node->piece.bufferIndex, node->piece.start);
      nodeStartOffset += node->size_left;
      const auto& beginItor = buffer.value.begin() + startOffset;
      if (retValStartOffset)*retValStartOffset = nodeStartOffset;
      return std::vector<unsigned char>(beginItor + preAccValue, beginItor + AccValue - endOffset);
    }
    else if (node->piece.lineFeedCnt == lineNumber - 1) // lineContent in the last line of this piece.
    {
      const size_t preAccValue = getAccumulatedValue(node, lineNumber - 2);
      const Buffer& buffer = gsl::at(buffers, node->piece.bufferIndex);
      const size_t startOffset = offsetInBuffer(node->piece.bufferIndex, node->piece.start);
      const auto& beginItor = buffer.value.begin() + startOffset;
      retVal = std::vector<unsigned char>(beginItor + preAccValue, beginItor + node->piece.length);
      break;
    }
    else
    {
      lineNumber -= node->piece.lineFeedCnt;
      node = node->right.get();
      nodeStartOffset += node->piece.length;
    }
  }
  if (node) node = node->right.get();
  while (node)
  {
    const Piece& piece = node->piece;
    const auto& buffer = gsl::at(buffers, piece.bufferIndex);
    if (piece.lineFeedCnt > 0) // the last few chars in fisrt line of this piece.
    {
      const size_t accValue = getAccumulatedValue(node, 0);
      const size_t startOffset = offsetInBuffer(piece.bufferIndex, piece.start);
      auto startItor = buffer.value.begin() + startOffset;
      retVal.insert(retVal.end(), startItor, startItor + accValue - endOffset);
      return retVal;
    }
    else // few chars in this line.
    {
      const size_t startOffset = offsetInBuffer(piece.bufferIndex, piece.start);
      auto startItor = buffer.value.begin() + startOffset;
      retVal.insert(retVal.end(), startItor, startItor + piece.length);
    }
    node = node->right.get();
  }

  return retVal;
}

// |->to right   <-to left|
void PieceTree::ShrinkPiece(TreeNode* current_node, size_t shrink_to_right, size_t shrink_to_left)
{
  Piece& piece = current_node->piece;
  const Piece original_piece = piece;
  auto& lineStarts = gsl::at(buffers, piece.bufferIndex).lineStarts;
  const size_t offset1 = offsetInBuffer(piece.bufferIndex, piece.start) + shrink_to_right,
    offset2 = offsetInBuffer(piece.bufferIndex, piece.end) - shrink_to_left;

  if (offset1 <= offset2)
  {
    const size_t lineIndex1 = GetLineIndexFromNodePosistion(lineStarts, NodePosition(current_node, shrink_to_right)),
      lineIndex2 = GetLineIndexFromNodePosistion(lineStarts, NodePosition(current_node, piece.length - shrink_to_left));

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

void PieceTree::UpdateMetadata() const noexcept
{
  size_t size_delta = 0;
  size_t lf_delta = 0;
  TreeNode* n = rootNode->right.get();
  while (n)
  {
    n->lf_left = lf_delta;
    n->size_left = size_delta;

    const Piece& piece = n->piece;
    lf_delta += piece.lineFeedCnt;
    size_delta += piece.length;

    n = n->right.get();
  }
}

size_t PieceTree::getAccumulatedValue(const TreeNode* node, size_t index)
{
  if (index < 0)return 0;
  const Piece& piece = node->piece;
  const auto& lineStarts = gsl::at(buffers, piece.bufferIndex).lineStarts;
  const size_t expectedLineStartIndex = piece.start.line + index + 1;
  const size_t startOffset = offsetInBuffer(piece.bufferIndex, piece.start);
  if (expectedLineStartIndex > piece.end.line)
  {
    return offsetInBuffer(piece.bufferIndex, piece.end) - startOffset;
  }
  else
  {
    return gsl::at(lineStarts, expectedLineStartIndex) - startOffset;
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

size_t GetLineIndexFromNodePosistion(const std::vector<size_t>& lineStarts, NodePosition nodePos)
{
  if (lineStarts.empty())
  {
    throw;
  }
  const size_t startLineOffset = gsl::at(lineStarts, nodePos.node->piece.start.line) + nodePos.node->piece.start.column;
  const size_t offset = nodePos.in_piece_offset + startLineOffset;
  const size_t startLine = nodePos.node->piece.start.line, endLine = nodePos.node->piece.end.line;
  auto it = std::upper_bound(lineStarts.begin() + startLine, lineStarts.begin() + endLine + 1, offset);

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


Buffer::Buffer() noexcept : value(), lineStarts{0}
{}

Buffer::Buffer(std::vector<unsigned char> input) : value(input), lineStarts(createLineStarts(value))
{}
