#pragma once
#include "pch.h"

std::vector<size_t> createLineStarts(const std::vector<unsigned char>& str);
size_t getLineIndexFromOffset(const std::vector<size_t>& lineStarts, size_t inPieceOffset);

struct Buffer {
  std::vector<unsigned char> value;
  std::vector<size_t> lineStarts;
  Buffer(std::vector<unsigned char> input) : value(input), lineStarts(createLineStarts(value))
  {}
};

struct BufferPosition {
  size_t index; //  line offset, index in Buffer.lineStarts
  size_t offset; // column offset
  BufferPosition() noexcept : index(0), offset(0)
  {}

  BufferPosition(const size_t& index, const size_t& offset) noexcept
    : index(index), offset(offset)
  {}
};

struct Piece {
  BufferPosition start; // start offset in buffers[bufferIndex] 
  BufferPosition end;
  size_t bufferIndex;
  size_t length;
  size_t lineFeedCnt;
  Piece() noexcept :start(), end(), bufferIndex(0), length(0), lineFeedCnt(0)
  {}

  Piece(const BufferPosition& start, const BufferPosition& end, const size_t& bufferIndex, const size_t& length, const size_t& lineFeedCnt) noexcept
    : start(start), end(end), bufferIndex(bufferIndex), length(length), lineFeedCnt(lineFeedCnt)
  {}
};

struct TreeNode {
public:
  Piece piece;
  TreeNode* left;
  std::unique_ptr<TreeNode> right;
  TreeNode() noexcept :piece(), left(nullptr), right(nullptr)
  {}

  TreeNode(const Piece& piece, TreeNode* _left) noexcept
    : piece(piece), left(_left), right(nullptr)
  {}
};

struct PieceTree {

  std::vector<Buffer> buffers;
  std::unique_ptr<TreeNode> rootNode;
  size_t lineCount;
  size_t length;

  PieceTree(std::vector<unsigned char> input);
  ~PieceTree() noexcept;

  PieceTree() = delete;
  PieceTree(const PieceTree&) = delete;
  PieceTree& operator=(const PieceTree&) = delete;
  PieceTree(PieceTree&&) = delete;
  PieceTree& operator=(PieceTree&&) = delete;

  bool InsertText(size_t offset, std::vector<unsigned char> input);
  bool EraseText(size_t offset, size_t erase_length);
  bool ReplaceText(size_t offset, std::vector<unsigned char> input, size_t erase_length);

  // use (TreeNode*, inPieceOffset) locate the insertion position.
  TreeNode* GetNodePosition(size_t offset, size_t& inPieceOffset) const noexcept;
  TreeNode* SplitPiece(TreeNode* currNode, const size_t inPieceOffset);
  std::vector<unsigned char> GetText(size_t offset, size_t text_length);
  void ShrinkPiece(TreeNode* current_node, size_t shrink_to_right, size_t shrink_to_left);
};