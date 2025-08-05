#pragma once
#include "pch.h"

struct Buffer {
  std::vector<unsigned char> value;
  std::vector<size_t> lineStarts;
  Buffer() noexcept;
  Buffer(std::vector<unsigned char> input);
};

struct BufferPosition {
  size_t line; //  line offset, index in Buffer.lineStarts
  size_t column; // column offset
  BufferPosition() noexcept : line(0), column(0)
  {}

  BufferPosition(const size_t& index, const size_t& offset) noexcept
    : line(index), column(offset)
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
  size_t size_left;
  size_t lf_left;
  TreeNode() noexcept :piece(), left(nullptr), right(nullptr), size_left(0), lf_left(0)
  {}

  TreeNode(const Piece& piece, TreeNode* _left) noexcept
    : piece(piece), left(_left), right(nullptr), size_left(0), lf_left(0)
  {}
};

struct NodePosition {
  TreeNode* node;
  size_t in_piece_offset;

  NodePosition() = default;

  NodePosition(TreeNode* node, const size_t& in_piece_offset) noexcept
    : node(node), in_piece_offset(in_piece_offset)
  {}
};
enum class ActionType {
  ActionInvalid,
  ActionInsertion,
  ActionErase,
  ActionReplace
};
struct EditAction {
  std::vector<unsigned char> insertedText;
  std::vector<unsigned char> erasedText;
  size_t ActionOffsetBytes; // OffsetBytes in piece.
  ActionType actionType;
};


class PieceTree {
public:
  std::vector<Buffer> buffers;
  std::unique_ptr<TreeNode> rootNode;
  std::stack<EditAction> undoStack;
  std::stack<EditAction> redoStack;
  size_t lineCount;
  size_t length;

  PieceTree() noexcept;
  PieceTree(std::vector<unsigned char> input);
  ~PieceTree() noexcept = default;


  PieceTree(const PieceTree&) = delete;
  PieceTree& operator=(const PieceTree&) = delete;
  PieceTree(PieceTree&&) = default;
  PieceTree& operator=(PieceTree&&) = default;

  void Init(const std::vector<unsigned char>& input)
  {
      buffers.push_back(Buffer());
      rootNode = std::make_unique<TreeNode>();
      buffers.emplace_back(Buffer(input)); // original
      const Buffer& buffer = buffers.back();
      const Piece piece{
        BufferPosition(0,0), // startPos
        BufferPosition(buffer.lineStarts.size() - 1, // endPos
          buffer.value.size() - buffer.lineStarts.back()),
        1, // BufferIndex
        buffer.value.size(), // length
        buffer.lineStarts.size() - 1}; // lineCount

      rootNode.get()->left = nullptr;
      rootNode.get()->right = std::make_unique<TreeNode>(piece, rootNode.get());
      length += piece.length;
      lineCount += piece.lineFeedCnt;
  }
  bool InsertText(size_t offset, std::vector<unsigned char> input);
  bool EraseText(size_t offset, size_t erase_length);
  bool ReplaceText(size_t offset, std::vector<unsigned char> input, size_t erase_length);

  // use (TreeNode*, inPieceOffset) locate the insertion position.
  NodePosition GetNodePositionAt(TreeNode* node, size_t offset) noexcept;
  NodePosition GetNodePosition(size_t offset) noexcept;
  size_t offsetInBuffer(size_t bufferIndex, BufferPosition pos);
  TreeNode* SplitPiece(TreeNode* currNode, const size_t inPieceOffset);
  std::vector<unsigned char> GetTextAt(TreeNode* node, size_t offset, size_t text_length);
  std::vector<unsigned char> GetText(size_t offset, size_t text_length);
  std::vector<unsigned char> GetLine(size_t lineNumber, const size_t endOffset = 0, size_t* retValStartOffset = nullptr);
  void ShrinkPiece(TreeNode* current_node, size_t shrink_to_right, size_t shrink_to_left);

  void UpdateMetadata() const noexcept;

  size_t getAccumulatedValue(const TreeNode* node, size_t index);
};

std::vector<size_t> createLineStarts(const std::vector<unsigned char>& str);
size_t GetLineIndexFromNodePosistion(const std::vector<size_t>& lineStarts, NodePosition nodePos);

