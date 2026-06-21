#ifndef TEXTDOCUMENT_PIECETREE_H_
#define TEXTDOCUMENT_PIECETREE_H_

#include <memory>
#include <vector>

/// A buffer stores raw UTF-16 text and a precomputed line-start index.
/// Buffer 0 is the "change buffer" — inserted text is appended here.
/// Buffer 1+ are "original buffers" — the initial document content.
struct Buffer {
  std::vector<char16_t> value;
  std::vector<size_t> lineStarts;
  Buffer() noexcept;
  Buffer(std::vector<char16_t> input);
};

/// Cursor position within a Buffer, expressed in (line, column) coordinates.
/// Both line and column are 0-based.
struct BufferPosition {
  size_t line;
  size_t column;
  BufferPosition() noexcept : line(0), column(0) {}

  BufferPosition(const size_t& index, const size_t& offset) noexcept
      : line(index), column(offset) {}
};

/// A Piece references a contiguous range within one Buffer.
/// It records the start and end BufferPositions, the buffer index,
/// the total character length, and the number of line feeds in this range.
struct Piece {
  BufferPosition start;
  BufferPosition end;
  size_t bufferIndex;
  size_t length;
  size_t lineFeedCnt;
  Piece() noexcept
      : start(), end(), bufferIndex(0), length(0), lineFeedCnt(0) {}

  Piece(const BufferPosition& start, const BufferPosition& end,
        const size_t& bufferIndex, const size_t& length,
        const size_t& lineFeedCnt) noexcept
      : start(start),
        end(end),
        bufferIndex(bufferIndex),
        length(length),
        lineFeedCnt(lineFeedCnt) {}
};

/// A node in the piece list (essentially a doubly-linked list).
///
/// Fields:
///   - piece:     the Piece this node owns
///   - left:      previous node (or parent sentinel)
///   - right:     next node (owned via unique_ptr)
///   - size_left: sum of piece.length of all nodes before this one
///   - lf_left:   sum of piece.lineFeedCnt of all nodes before this one
///
/// size_left and lf_left are maintained by UpdateMetadata() and enable
/// O(1) determination of a node's position in the document without
/// scanning from the beginning.
struct TreeNode {
 public:
  Piece piece;
  TreeNode* left;
  std::unique_ptr<TreeNode> right;
  size_t size_left;
  size_t lf_left;
  TreeNode() noexcept
      : piece(), left(nullptr), right(nullptr), size_left(0), lf_left(0) {}

  TreeNode(const Piece& piece, TreeNode* _left) noexcept
      : piece(piece), left(_left), right(nullptr), size_left(0), lf_left(0) {}
};

/// Result of a position lookup: which node an offset falls in,
/// and the offset within that node's piece.
struct NodePosition {
  TreeNode* node;
  size_t in_piece_offset;

  NodePosition() = default;

  NodePosition(TreeNode* node, const size_t& in_piece_offset) noexcept
      : node(node), in_piece_offset(in_piece_offset) {}
};

/// Piece Table data structure for efficient text editing.
///
/// The piece table stores the original document content in buffer[1] and
/// all subsequent edits in buffer[0] (the change buffer). Each logical
/// text range is represented as a Piece that references a range within
/// one buffer. Edits only append new data to the change buffer and
/// rearrange piece references — they never modify or copy existing text.
///
/// Pieces are arranged as a linked list (via TreeNode.left / .right).
/// Cumulative metadata (size_left, lf_left) is maintained to support
/// O(n) traversal from any point and O(1) offset-to-position queries
/// when the search cache (see GetNodePosition) hits.
class PieceTree {
 public:
  std::vector<Buffer> buffers;
  std::unique_ptr<TreeNode> rootNode;
  BufferPosition _lastChangeBufferPos;
  size_t lineCount;
  size_t length;

  PieceTree() noexcept;
  PieceTree(std::vector<char16_t> input);
  ~PieceTree() noexcept = default;

  PieceTree(const PieceTree&) = delete;
  PieceTree& operator=(const PieceTree&) = delete;
  PieceTree(PieceTree&&) = default;
  PieceTree& operator=(PieceTree&&) = default;

  /// Initialize the tree with document content. Creates an empty buffer[0]
  /// and stores the input as buffer[1] with one initial Piece.
  void Init(const std::vector<char16_t>& input) {
    buffers.push_back(Buffer());
    rootNode = std::make_unique<TreeNode>();
    buffers.emplace_back(Buffer(input));  // original
    if (input.empty()) return;
    const Buffer& buffer = buffers.back();
    const Piece piece{
        BufferPosition(0, 0),
        BufferPosition(buffer.lineStarts.size() - 1,
                       buffer.value.size() - buffer.lineStarts.back()),
        1,
        buffer.value.size(),
        buffer.lineStarts.size() - 1};

    rootNode.get()->left = nullptr;
    rootNode.get()->right = std::make_unique<TreeNode>(piece, rootNode.get());
    length += piece.length;
    lineCount += piece.lineFeedCnt;
  }

  /// Insert text at a given offset. The input is appended to buffer[0],
  /// and a new Piece referencing it is inserted into the piece list.
  /// @return true on success, false if offset > document length.
  bool InsertText(size_t offset, const std::vector<char16_t>& input);

  /// Erase a range of text by shrinking or removing Pieces.
  /// @return true on success, false if the range is invalid or empty.
  bool EraseText(size_t offset, size_t erase_length);

  /// Combined erase-then-insert at the same offset.
  /// Equivalent to EraseText(offset, erase_length) + InsertText(offset, input).
  bool ReplaceText(size_t offset, const std::vector<char16_t>& input,
                   size_t erase_length);

  /// Locate the node containing a given offset, searching from @p node
  /// forward. Used internally by GetTextAt and similar operations that
  /// traverse the piece list.
  NodePosition GetNodePositionAt(TreeNode* node, size_t offset) noexcept;

  /// Locate the node containing a given offset.
  /// Uses and updates the internal search cache for O(1) hit on repeated
  /// queries at the same location.
  NodePosition GetNodePosition(size_t offset) noexcept;

  /// Compute the absolute byte offset of a BufferPosition within its buffer.
  size_t offsetInBuffer(size_t bufferIndex, BufferPosition pos) const;

  /// Split a node into two pieces at a given intra-piece offset.
  /// @return The original (left) node; its right sibling is linked via ->right.
  TreeNode* SplitPiece(TreeNode* currNode, const size_t inPieceOffset);

  /// Retrieve a range of text starting from @p node, offset by @p offset
  /// within that node, for @p text_length characters.
  std::vector<char16_t> GetTextAt(TreeNode* node, size_t offset,
                                  size_t text_length);

  /// Retrieve a range of text from the document.
  std::vector<char16_t> GetText(size_t offset, size_t text_length);

  /// Retrieve the content of a single line (1-based line number).
  /// @param lineNumber      1-based line number.
  /// @param endOffset       Number of trailing characters to omit.
  /// @param retValStartOffset [out] Absolute offset of the line start.
  /// @return Line content. Includes trailing \\n except possibly the last line
  ///         (depending on endOffset). Empty if lineNumber is out of range.
  std::vector<char16_t> GetLine(size_t lineNumber, const size_t endOffset,
                                size_t* retValStartOffset) const;

  /// Shrink a piece from one or both sides (used during erase).
  /// @param shrink_to_right  Characters to remove from the start.
  /// @param shrink_to_left   Characters to remove from the end.
  void ShrinkPiece(TreeNode* current_node, size_t shrink_to_right,
                   size_t shrink_to_left);

  /// Recompute size_left and lf_left for every node from root to end.
  /// Prefer the overload that takes a "from" node (incremental update).
  void UpdateMetadata() const noexcept;

  /// Recompute size_left and lf_left for nodes after @p from.
  /// @p from itself must already have correct metadata.
  void UpdateMetadata(const TreeNode* from) const noexcept;

  /// For a given node, get the accumulated line offset within the piece
  /// at a specific line index (0-based relative to the piece's start).
  size_t getAccumulatedValue(const TreeNode* node, size_t index) const;

  /// Brute-force computation of the longest line in the document (O(n²)).
  /// todo: cache this value and update incrementally on edits.
  size_t getLongestLine() const;

 private:
  /// Single-entry search cache for GetNodePosition.
  mutable NodePosition _searchCache;
  mutable bool _cacheValid = false;
};

/// Build the line-start index for a UTF-16 string.
/// Each entry is the byte offset of the start of a line.
/// Line endings (\\r, \\n, \\r\\n) determine line boundaries.
/// @return A vector of offsets — the first entry is always 0.
std::vector<size_t> createLineStarts(const std::vector<char16_t>& str);

/// Determine which line (within the piece's buffer) a NodePosition falls on.
/// Uses binary search (upper_bound) on the buffer's lineStarts array,
/// restricted to the piece's [start.line, end.line] range.
/// @param lineStarts Line-start index of the relevant buffer.
/// @param nodePos    NodePosition describing a location within a piece.
/// @return 0-based line index within that buffer.
size_t GetLineIndexFromNodePosistion(const std::vector<size_t>& lineStarts,
                                     NodePosition nodePos);

#endif
