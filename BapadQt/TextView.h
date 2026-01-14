#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include <QAbstractScrollArea>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>

#include "TextDocument.h"

class TextView : public QAbstractScrollArea {
  Q_OBJECT
 public:
  TextView();
  bool Clear();
  bool NewFile(std::vector<char16_t> utf16Content);

 protected:
  bool viewportEvent(QEvent* event);
  void paintEvent(QPaintEvent* event);

  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseDoubleClickEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* e);
  void scrollContentsBy(int dx, int dy);
  void contextMenuEvent(QContextMenuEvent* e);

  void keyPressEvent(QKeyEvent* e);
  void inputMethodEvent(QInputMethodEvent* event);
  QVariant inputMethodQuery(Qt::InputMethodQuery query);

 private:
  void OnResetLineHeight();
  void UpdateScrollBars();

  void OnPaintText(QPainter* painter);
  int LongestLineLength();
  int GetCursorOffset(const QPoint& pixel_pos) const;
  QPoint GetPixelPosition(int offset) const;

  void DeleteSelection();
  void UnSelect();
  void HandleInputText(const QString& text);
  bool HandleCtrlKey(QKeyEvent* event);
  void ScrollToCursor();
  void SyncCursor();
  void DoAutoScroll();
  void UpdateSelectionOnMouseMove(const QPoint& pos);

  void MoveLineStart();
  void MoveLineEnd();
  void MoveLineUp(int numOfLines = 1);
  void MoveLineDown(int numOfLines = 1);
  void GoPageUp();
  void GoPageDown();

  void Cut();
  void Copy();
  void Paste();
  void Undo();
  void Redo();

  int cursor_offset_;
  int selection_start_offset_;
  int selection_end_offset_;
  bool is_mouse_pressed = false;
  bool caret_visible_ = false;
  QTimer caret_timer_;
  QTimer scroll_timer_;
  int line_height_;
  QFont current_font_;

  int margin_right_;

  QTimer* auto_scroll_timer_;
  QPoint last_mouse_pos_;  // 记录鼠标在视口外的位置

  std::unique_ptr<TextDocument> text_document_;
 private slots:
  void ToggleCursorVisibility();
};

#endif  // TEXTVIEW_H
