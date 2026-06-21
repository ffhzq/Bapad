#include "pch.h"
#include "TestHelpers.h"
#include "../TextDocument/TextDocument.h"

#include <chrono>
#include <iostream>

// ===========================================================================
// Performance benchmarks
//
// Run with: --gtest_filter=Perf_*
// Skip with: --gtest_filter=-Perf_*
// These tests don't assert timing thresholds — run them on different
// branches to compare output.
// ===========================================================================

class PerfTest : public ::testing::Test {
 protected:
  std::unique_ptr<TextDocument> doc;

  void SetUp() override {
    doc = std::make_unique<TextDocument>();
  }

  // Print a timing row: name, N, size, duration
  void Report(const char* name, size_t n, size_t size, double ms) {
    std::cout << "  " << name << "\t" << n << "\t" << size << "\t" << ms
              << "ms" << std::endl;
  }
};

// ---------------------------------------------------------------------------
// Insert 10,000 short lines at the end of an empty document
// (exercises the Append-to-last-piece fast path)
// ---------------------------------------------------------------------------
TEST_F(PerfTest, Append10kLines) {
  constexpr int N = 10000;
  auto line = toWCharVector("Hello\n");

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i)
    doc->InsertText(doc->GetDocLength(), line);
  auto end = std::chrono::steady_clock::now();

  auto ms = std::chrono::duration<double, std::milli>(end - start).count();
  Report("Append10kLines", N, doc->GetDocLength(), ms);
}

// ---------------------------------------------------------------------------
// Insert 1,000 lines at the beginning
// (worst case for UpdateMetadata — every insert shifts all subsequent pieces)
// ---------------------------------------------------------------------------
TEST_F(PerfTest, Insert1kLinesAtBeginning) {
  constexpr int N = 1000;
  auto line = toWCharVector("Head\n");

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i)
    doc->InsertText(0, line);
  auto end = std::chrono::steady_clock::now();

  auto ms = std::chrono::duration<double, std::milli>(end - start).count();
  Report("Insert1kAtBeginning", N, doc->GetDocLength(), ms);
}

// ---------------------------------------------------------------------------
// Get 1,000 random lines by line number on a large document
// (exercises the Traverse-line-by-line path used in paint loops)
// ---------------------------------------------------------------------------
TEST_F(PerfTest, RandomGetLine1k) {
  constexpr int LINES = 10000;
  constexpr int READS = 1000;

  auto line = toWCharVector("Line\n");
  for (int i = 0; i < LINES; ++i)
    doc->InsertText(doc->GetDocLength(), line);

  // Pre-generate random offsets
  std::vector<size_t> offsets(READS);
  for (auto& o : offsets)
    o = static_cast<size_t>(rand()) % doc->GetDocLength();

  auto start = std::chrono::steady_clock::now();
  for (auto o : offsets)
    doc->LineNumFromCharOffset(o);
  auto end = std::chrono::steady_clock::now();

  auto ms = std::chrono::duration<double, std::milli>(end - start).count();
  Report("RandomGetLine1k", READS, LINES, ms);
}

// ---------------------------------------------------------------------------
// Insert 500 pieces, then Undo all of them
// (exercises both piece splitting and the undo stack)
// ---------------------------------------------------------------------------
TEST_F(PerfTest, Undo500Operations) {
  constexpr int N = 500;
  auto line = toWCharVector("Middle\n");
  // Start with a single "Base" document, then keep inserting in the middle
  doc->Initialize(toWCharVector("Start\nEnd"));

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i)
    doc->InsertText(6, line);
  for (int i = 0; i < N; ++i)
    doc->Undo();
  auto end = std::chrono::steady_clock::now();

  auto ms = std::chrono::duration<double, std::milli>(end - start).count();
  Report("Undo500Ops", N, N * 2, ms);
}

// ---------------------------------------------------------------------------
// Initialize a document with 100,000 characters
// (exercises PieceTree construction + lineStarts calculation)
// ---------------------------------------------------------------------------
TEST_F(PerfTest, Initialize100kChars) {
  constexpr int TARGET = 100000;
  std::vector<char16_t> input;
  input.reserve(TARGET);
  for (int i = 0; i < TARGET; ++i)
    input.push_back(i % 50 == 0 ? u'\n' : u'a');

  auto start = std::chrono::steady_clock::now();
  doc->Initialize(input);
  auto end = std::chrono::steady_clock::now();

  auto ms = std::chrono::duration<double, std::milli>(end - start).count();
  Report("Init100kChars", doc->GetLineCount(), TARGET, ms);
}

// ---------------------------------------------------------------------------
// GetText spanning multiple pieces (50 chars × 100 reads)
// (exercises walking the piece chain)
// ---------------------------------------------------------------------------
TEST_F(PerfTest, MultiPieceGetText) {
  // Build a document with many small pieces
  doc->Initialize(toWCharVector("X\n"));
  auto chunk = toWCharVector(std::string(50, 'a') + "\n");
  constexpr int PIECES = 100;
  for (int i = 0; i < PIECES; ++i)
    doc->InsertText(2, chunk);

  constexpr int READS = 100;
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < READS; ++i)
    doc->GetText(0, doc->GetDocLength());
  auto end = std::chrono::steady_clock::now();

  auto ms = std::chrono::duration<double, std::milli>(end - start).count();
  Report("MultiPieceGetText", READS, doc->GetDocLength(), ms);
}
