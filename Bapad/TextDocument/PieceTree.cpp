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

PieceTree::PieceTree() noexcept : buffers{}, rootNode(), _lastChangeBufferPos(), length(0), lineCount(1)
{
  auto input = std::vector<wchar_t>();
  Init(input);
}

PieceTree::PieceTree(std::vector<wchar_t> input) : buffers{}, rootNode(), _lastChangeBufferPos(), length(0), lineCount(1)
{
  Init(input);
}


bool PieceTree::InsertText(size_t offset, const std::vector<wchar_t>& input)
{
  if (offset > this->length)
  {
    return false;
  }
  NodePosition nodePosition = GetNodePosition(offset);
  const size_t pieceOffset = nodePosition.in_piece_offset;
  TreeNode* node = nodePosition.node;
  auto& buffer = gsl::at(buffers, 0);
  const size_t start_offset = buffer.value.size();
  buffer.value.insert(buffer.value.end(), input.begin(), input.end());
  auto lineStarts = createLineStarts(input);
  for (auto& i : lineStarts)
  {
    i += start_offset;
  }
  buffer.lineStarts.insert(buffer.lineStarts.end(), lineStarts.begin() + 1, lineStarts.end());
  if (node != rootNode.get())
  {
    assert(node);
    if (node && pieceOffset != node->piece.length) // insert inside a piece, split then insert
    {
      if (pieceOffset == 0) node = node->left;
      else
        node = SplitPiece(node, pieceOffset);
    }
    Piece& piece = node->piece;

    if (piece.bufferIndex == 0 &&
      piece.end.line == _lastChangeBufferPos.line &&
      piece.end.column == _lastChangeBufferPos.column &&
      offset == piece.length + node->size_left)
    { // append to piece
      // appendToNode(nodePos, input);
      const size_t endIndex = buffer.lineStarts.size() - 1,
        endColumn = buffer.value.size() - gsl::at(buffer.lineStarts, endIndex);
      const BufferPosition newEnd{ endIndex,endColumn };
      const size_t newLength = piece.length + input.size();
      const size_t newLfCount = piece.lineFeedCnt + lineStarts.size() - 1;
      piece.end = newEnd;
      piece.length = newLength;
      piece.lineFeedCnt = newLfCount;
      _lastChangeBufferPos = newEnd;
      length += input.size();
      lineCount += lineStarts.size() - 1;
      UpdateMetadata();
      return true;
    }
  }

  const BufferPosition start = _lastChangeBufferPos;
  const Piece newPiece{
    start, // startPos
    BufferPosition(buffer.lineStarts.size() - 1, // endPos
      buffer.value.size() - buffer.lineStarts.back()),
    0, // BufferIndex
    input.size(), // length
    lineStarts.size() - 1 }; // lineCount
  _lastChangeBufferPos = newPiece.end;
  auto newNode = std::make_unique<TreeNode>(newPiece, node);
  if (node->right == nullptr)
  {
    newNode->right = nullptr;
  }
  else
  {
    newNode->right = std::move(node->right);
    newNode->right->left = newNode.get();
  }
  node->right = std::move(newNode);

  length += newPiece.length;
  lineCount += newPiece.lineFeedCnt;
  UpdateMetadata();
  return true;
}

bool PieceTree::EraseText(size_t offset, size_t eraseLength)
{
  if (rootNode->right == nullptr || offset + eraseLength > length) return false;
  const size_t original_offset = offset, original_erase_length = eraseLength;

  const NodePosition startNodePosition = GetNodePosition(offset);
  TreeNode* currentNode = startNodePosition.node;
  if (currentNode == rootNode.get()) currentNode = currentNode->right.get();
  if (!currentNode) return false;

  // if erase from middle
  if (startNodePosition.in_piece_offset != 0)
  {
    currentNode = SplitPiece(currentNode, startNodePosition.in_piece_offset);
    currentNode = currentNode->right.get();
  }
  TreeNode* parrentNode = currentNode->left;
  currentNode = parrentNode;
  size_t bytesErased = 0;
  while (currentNode != nullptr && bytesErased < eraseLength)
  {
    currentNode = currentNode->right.get();
    if (currentNode->piece.length == 0)
    {
      continue;
    }
    const size_t remainBytesToBeErased = eraseLength - bytesErased;
    const size_t bytesToBeErased = (std::min)(currentNode->piece.length, remainBytesToBeErased);
    ShrinkPiece(currentNode, bytesToBeErased, 0);
    bytesErased += bytesToBeErased;
  }
  if (parrentNode) {
    // delete pieces
    if (currentNode == nullptr)
    {
      parrentNode->right = nullptr;
    }
    else
    {
      parrentNode->right = currentNode->piece.length == 0 ? std::move(currentNode->right) : std::move(currentNode->left->right);
    }
  }
  else {
    return false;
  }
  UpdateMetadata();
  return true;
}

bool PieceTree::ReplaceText(size_t offset, const std::vector<wchar_t> & input, size_t erase_length)
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
  return GetNodePositionAt(rootNode.get(), offset);
}

size_t PieceTree::offsetInBuffer(size_t bufferIndex, BufferPosition pos) const
{
  const auto& lineStarts = gsl::at(buffers, bufferIndex).lineStarts;
  return gsl::at(lineStarts, pos.line) + pos.column;
}

TreeNode* PieceTree::SplitPiece(TreeNode* currNode, const size_t inPieceOffset)
{
  const Buffer& currBuffer = gsl::at(buffers, currNode->piece.bufferIndex);
  const Piece original_piece = currNode->piece;
  Piece& current_piece = currNode->piece;
  
  const size_t newStartOffset = offsetInBuffer(currNode->piece.bufferIndex, current_piece.start) + inPieceOffset;
  const size_t newLineIndex = GetLineIndexFromNodePosistion(currBuffer.lineStarts, NodePosition(currNode, inPieceOffset));
  current_piece.end = BufferPosition{ newLineIndex, newStartOffset - gsl::at(currBuffer.lineStarts,newLineIndex) };
  current_piece.length = inPieceOffset;
  current_piece.lineFeedCnt = newLineIndex - original_piece.start.line;
  Piece p2;
  if (inPieceOffset == 0)
  {
    throw;
    p2 = original_piece;
  }
  else
  {
    p2 = Piece{
       current_piece.end,
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
  this->UpdateMetadata();
  return currNode;
}

std::vector<wchar_t> PieceTree::GetText(size_t offset, size_t text_length)
{
  return GetTextAt(this->rootNode->right.get(), offset, text_length);
}

std::vector<wchar_t> PieceTree::GetTextAt(TreeNode* node, size_t offset, size_t text_length)
{
  auto text = std::vector<wchar_t>();
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
    const auto& buffer_value = buffer.value;

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
std::vector<wchar_t> PieceTree::GetLine(size_t lineNumber, const size_t endOffset, size_t* retValStartOffset) const
{
  if (lineNumber > lineCount || lineCount == 0 || lineNumber == 0) return std::vector<wchar_t>();
  TreeNode* node = rootNode->right.get();
  size_t nodeStartOffset = 0; //search cache
  const size_t originalLineNumber = lineNumber;
  auto retVal = std::vector<wchar_t>();
  while (node)
  {
    if (node->piece.lineFeedCnt > lineNumber - 1) // lineContent within this piece
    {
      const size_t preAccValue = lineNumber < 2 ? 0 : getAccumulatedValue(node, lineNumber - 2);
      const size_t AccValue = getAccumulatedValue(node, lineNumber - 1);
      const Buffer& buffer = gsl::at(buffers, node->piece.bufferIndex);
      const size_t startOffset = offsetInBuffer(node->piece.bufferIndex, node->piece.start);
      nodeStartOffset += node->size_left;
      const auto& beginItor = buffer.value.begin() + startOffset;
      if (retValStartOffset) *retValStartOffset = preAccValue + node->size_left;
      return std::vector<wchar_t>(beginItor + preAccValue, beginItor + AccValue - endOffset);
    }
    else if (node->piece.lineFeedCnt == lineNumber - 1) // lineContent in the last line of this piece.
    {
      const size_t preAccValue = lineNumber < 2 ? 0 : getAccumulatedValue(node, lineNumber - 2);
      const Buffer& buffer = gsl::at(buffers, node->piece.bufferIndex);
      const size_t startOffset = offsetInBuffer(node->piece.bufferIndex, node->piece.start);
      const auto& beginItor = buffer.value.begin() + startOffset;
      if (retValStartOffset) *retValStartOffset = preAccValue + node->size_left;
      retVal = std::vector<wchar_t>(beginItor + preAccValue, beginItor + node->piece.length);
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
      line_offset2 = offset2 - gsl::at(lineStarts, lineIndex2);

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

size_t PieceTree::getAccumulatedValue(const TreeNode* node, size_t index) const
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

size_t PieceTree::getLongestLine() const
{// todo: brute force
  size_t longestLineSize = 0;
  for (size_t i = 1; i <= lineCount; ++i)
  {
    auto lineContent(GetLine(i, 0, nullptr));
    longestLineSize = (std::max)(lineContent.size(), longestLineSize);
  }
  return longestLineSize;
}

std::vector<size_t> createLineStarts(const std::vector<wchar_t>& str)
{
  std::vector<size_t> lineStarts{ 0 }; // first line starts in 0.
  const size_t length = str.size();
  for (size_t i = 0; i < length; ++i)
  {
    auto ch = gsl::at(str, i);
    if (ch == static_cast<wchar_t>(CharCode::CarriageReturn))
    {
      if (i + 1 < length && gsl::at(str, i + 1) == static_cast<wchar_t>(CharCode::LineFeed))
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
    else if (ch == static_cast<wchar_t>(CharCode::LineFeed))
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


Buffer::Buffer() noexcept : value(), lineStarts{ 0 }
{
}

Buffer::Buffer(std::vector<wchar_t> input) : value(input), lineStarts(createLineStarts(value))
{
}
