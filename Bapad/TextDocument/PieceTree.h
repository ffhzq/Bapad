#pragma once
#include "pch.h"

struct Buffer {
  std::vector<unsigned char> value;
  std::vector<size_t> lineStarts;
  Buffer() :value(), lineStarts({0})
  {};
  Buffer(std::vector<unsigned char> input) : value(input), lineStarts(createLineStarts(value))
  {}
};

struct BufferPosition {
  size_t index; //  行, index in Buffer.lineStarts
  size_t offset; // 列
  BufferPosition() noexcept : index(0), offset(0)
  {}

  BufferPosition(const size_t& index, const size_t& offset)
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

  Piece(const BufferPosition& start, const BufferPosition& end, const size_t& bufferIndex, const size_t& length, const size_t& lineFeedCnt)
    : start(start), end(end), bufferIndex(bufferIndex), length(length), lineFeedCnt(lineFeedCnt)
  {}
};

struct Node {
public:
  Piece piece;
  Node* left;
  Node* right;
  Node() noexcept :piece(), left(nullptr), right(nullptr)
  {}

  Node(const Piece& piece, Node* left, Node* right)
    : piece(piece), left(left), right(right)
  {}
};

class PieceTree {
public:

  PieceTree(std::vector<unsigned char> input);
  ~PieceTree() noexcept;

  PieceTree() = delete;
  PieceTree(const PieceTree&) = delete;
  PieceTree& operator=(const PieceTree&) = delete;
  PieceTree(PieceTree&&) = delete;
  PieceTree& operator=(PieceTree&&) = delete;

  bool InsertText(size_t offset, unsigned char* str, size_t length) noexcept;
  bool EraseText(size_t offset, size_t length) noexcept;
  bool ReplaceText(size_t offset, unsigned char* str, size_t length, size_t erase_length) noexcept;
private:
  std::vector<Buffer> buffers;
  Node rootNode; //dump
  size_t lineCount;
  size_t length;

  size_t GetNodeIndex(size_t offset);
};

std::vector<size_t> createLineStarts(const std::vector<unsigned char>& str);