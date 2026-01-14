#include "Textview.h"

#include <QClipboard>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QScrollBar>
#include <QTextLayout>

#include "FormatConversionV2.h"

TextView::TextView()
    : cursor_offset_(0),
      selection_start_offset_(0),
      selection_end_offset_(0),
      line_height_(0),
      current_font_(),
      text_document_(new TextDocument()),
      caret_timer_(QTimer(this)),
      margin_right_(2) {
  current_font_.setFamily("Sarasa Fixed SC");
  current_font_.setPixelSize(18);
  QFontInfo info(current_font_);
  if (info.exactMatch()) {
    ;
  } else {
    current_font_ = QFont();
  }
  setFont(current_font_);
  OnResetLineHeight();
  connect(&caret_timer_, &QTimer::timeout, this,
          &TextView::ToggleCursorVisibility);
  caret_timer_.start(500);
  setAttribute(Qt::WA_InputMethodEnabled);

  auto_scroll_timer_ = new QTimer(this);
  connect(auto_scroll_timer_, &QTimer::timeout, this, &TextView::DoAutoScroll);
}

bool TextView::Clear() {
  if (text_document_) text_document_->Clear();
  // todo reset text, scroll bar
  cursor_offset_ = selection_start_offset_ = selection_end_offset_ = 0;
  is_mouse_pressed = false;
  caret_visible_ = false;
  return true;
}

bool TextView::NewFile(std::vector<char16_t> utf16Content) {
  Clear();
  text_document_->Initialize(NormalizeLineEndings(utf16Content));
  UpdateScrollBars();
  return true;
}
void TextView::scrollContentsBy(int dx, int dy) {
  // viewport()->scroll(dx, dy);
  QAbstractScrollArea::scrollContentsBy(dx, dy);
  viewport()->update();
}

void TextView::OnPaintText(QPainter* painter) {
  int vertical_offset = verticalScrollBar()->value();
  int horizontal_offset = horizontalScrollBar()->value();  // todo:*-1

  int first_visible_line = vertical_offset / line_height_;
  int last_visible_line =
      (vertical_offset + viewport()->height()) / line_height_;

  int total_lines = text_document_->GetLineCount();
  if (last_visible_line >= total_lines) {
    last_visible_line = total_lines - 1;
  }

  for (int line = first_visible_line; line <= last_visible_line; ++line) {
    TextIterator itor =
        text_document_->IterateLineByLineNumber(line, nullptr, nullptr);
    QString text(itor.GetLine());

    const int line_y = line * line_height_;
    const int base_line_y =
        line_y - vertical_offset + QFontMetrics(current_font_).ascent();

    painter->drawText(-horizontal_offset, base_line_y, text);
  }
}

int TextView::LongestLineLength() {  // todo: buffer
  QFontMetrics fm(current_font_);
  int longest_line_length = 0;
  for (int i = 0; i <= text_document_->GetLineCount(); ++i) {
    TextIterator itor =
        text_document_->IterateLineByLineNumber(i, nullptr, nullptr);
    QString line_content = QString(itor.GetLine());
    longest_line_length =
        std::max(fm.horizontalAdvance(line_content), longest_line_length);
  }
  return longest_line_length;
}

int TextView::GetCursorOffset(const QPoint& pixel_pos) const {
  int vertical_scroll_offset = verticalScrollBar()->value();
  int absolute_y = pixel_pos.y() + vertical_scroll_offset;
  int raw_line = absolute_y / line_height_;
  if (raw_line < 0) raw_line = 0;
  size_t line_index =
      std::min((size_t)raw_line, text_document_->GetLineCount() - 1);
  size_t line_start_offset = 0;
  QString line_text = QString(
      text_document_
          ->IterateLineByLineNumber(line_index, &line_start_offset, nullptr)
          .GetLine());
  if (line_text.endsWith('\n')) line_text.erase(line_text.end() - 1);
  QTextLayout layout(line_text, current_font_);
  layout.beginLayout();
  QTextLine line = layout.createLine();
  line.setLineWidth(this->viewport()->width());
  layout.endLayout();

  int horizontal_scroll_offset = horizontalScrollBar()->value();
  qreal hit_point = pixel_pos.x() + horizontal_scroll_offset;
  int unit_index_in_line = line.xToCursor(hit_point);
  size_t global_offset = line_start_offset + unit_index_in_line;
  // qDebug() <<"GetCursorOffset(): x and y"<<pixelPos.x()<<","<<pixelPos.y()<<"
  // globalOffset is "<< globalOffset<<" lineIndex: " << lineIndex;
  return global_offset;
}

QPoint TextView::GetPixelPosition(int offset) const {
  int line_number = text_document_->LineNumFromCharOffset(offset);
  int absolute_y = line_number * line_height_;
  int viewport_y = absolute_y - verticalScrollBar()->value();

  size_t line_start_offset = 0;
  QString line_text =
      QString(text_document_
                  ->IterateLineByCharOffset(offset, nullptr, &line_start_offset)
                  .GetLine());
  QString precursor_text = line_text.left(offset - line_start_offset);
  int text_width_pixels =
      QFontMetrics(current_font_).horizontalAdvance(precursor_text);

  int viewport_x = text_width_pixels - horizontalScrollBar()->value();
  // qDebug() << "GetPixelPosition(): x,y" << viewport_x << viewport_y
  //          << " lineIndex: " << line_number << "globalOffset:" << offset;
  return QPoint(viewport_x, viewport_y);
}

void TextView::DeleteSelection() {
  if (selection_end_offset_ != selection_start_offset_) {
    size_t start = std::min(selection_start_offset_, selection_end_offset_);
    size_t end = std::max(selection_start_offset_, selection_end_offset_);
    text_document_->EraseText(start, end - start);
    cursor_offset_ = selection_end_offset_ = selection_start_offset_ = start;
    UpdateScrollBars();
  }
}

void TextView::UnSelect() {
  selection_start_offset_ = selection_end_offset_ = cursor_offset_;
}

void TextView::HandleInputText(const QString& text) {
  if (!text.isEmpty()) {
    DeleteSelection();
    std::vector<char16_t> chars(text.length());
    for (int i = 0; i < text.length(); ++i) {
      chars[i] = text.at(i).unicode();
    }
    text_document_->InsertText(cursor_offset_, chars);
    cursor_offset_ += chars.size();
    selection_end_offset_ = selection_start_offset_ = cursor_offset_;
    return;
  }
}

bool TextView::HandleCtrlKey(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Z:  // Undo
      if (text_document_->CanUndo()) {
        Undo();
        SyncCursor();
      }
      return true;

    case Qt::Key_Y:  // Redo

      if (text_document_->CanRedo()) {
        Redo();
        SyncCursor();
      }
      return true;

    case Qt::Key_X:  // Cut
      Cut();
      SyncCursor();
      return true;

    case Qt::Key_C:  // Copy
      Copy();
      SyncCursor();
      return true;

    case Qt::Key_V:  // Paste
      Paste();
      SyncCursor();
      return true;
  }
  return false;
}

void TextView::ScrollToCursor() {
  QPoint cursor_pos = GetPixelPosition(cursor_offset_);

  QScrollBar* vbar = verticalScrollBar();
  QScrollBar* hbar = horizontalScrollBar();

  int v_value = vbar->value();
  int h_value = hbar->value();

  int vw = viewport()->width();
  int vh = viewport()->height();

  if (cursor_pos.y() < 0) {
    vbar->setValue(v_value + cursor_pos.y());
  }

  else if (cursor_pos.y() + line_height_ > vh) {
    vbar->setValue(v_value + (cursor_pos.y() + line_height_ - vh));
  }

  if (cursor_pos.x() < 0) {
    hbar->setValue(h_value + cursor_pos.x());
  } else if (cursor_pos.x() > vw) {
    hbar->setValue(h_value + (cursor_pos.x() - vw) + margin_right_);
  }

  viewport()->update();
}

void TextView::SyncCursor() {
  UpdateScrollBars();
  ScrollToCursor();

  caret_visible_ = true;
  caret_timer_.stop();
  caret_timer_.start(500);

  viewport()->update();
}

void TextView::DoAutoScroll() {
  int v_value = verticalScrollBar()->value();
  int h_value = horizontalScrollBar()->value();

  if (last_mouse_pos_.y() < 0) {
    verticalScrollBar()->setValue(v_value - line_height_);  // 向上滚一行
  } else if (last_mouse_pos_.y() > viewport()->height()) {
    verticalScrollBar()->setValue(v_value + line_height_);  // 向下滚一行
  }

  if (last_mouse_pos_.x() < 0) {
    horizontalScrollBar()->setValue(h_value - 20);
  } else if (last_mouse_pos_.x() > viewport()->width()) {
    horizontalScrollBar()->setValue(h_value + 20);
  }

  UpdateSelectionOnMouseMove(last_mouse_pos_);
}

void TextView::UpdateSelectionOnMouseMove(const QPoint& pos) {
  int current_offset = GetCursorOffset(pos);

  if (current_offset < 0) {
    current_offset = 0;
  } else if (current_offset > (int)text_document_->GetDocLength()) {
    current_offset = (int)text_document_->GetDocLength();
  }

  cursor_offset_ = current_offset;
  selection_end_offset_ = current_offset;

  caret_visible_ = true;
  caret_timer_.stop();
  caret_timer_.start(500);

  // 5. 触发重绘
  viewport()->update();
}

void TextView::MoveLineStart() {
  size_t linestart_offset = 0;
  auto itor = text_document_->IterateLineByCharOffset(cursor_offset_, nullptr,
                                                      &linestart_offset);
  cursor_offset_ = linestart_offset;
  selection_start_offset_ = selection_end_offset_ = cursor_offset_;
}

void TextView::MoveLineEnd() {
  size_t linestart_offset = 0;
  auto itor = text_document_->IterateLineByCharOffset(cursor_offset_, nullptr,
                                                      &linestart_offset);
  const auto line_content = itor.GetLine();
  cursor_offset_ = linestart_offset + line_content.size();
  if (*line_content.crbegin() == u'\n') cursor_offset_ -= 1;
  selection_start_offset_ = selection_end_offset_ = cursor_offset_;
}

void TextView::MoveLineUp(int num_of_lines) {
  const int current_line =
      text_document_->LineNumFromCharOffset(cursor_offset_);
  if (current_line - num_of_lines < 0) num_of_lines = current_line;

  const QPoint pixel_low = GetPixelPosition(cursor_offset_);
  QPoint pixel_high = pixel_low;
  pixel_high.setY(pixel_low.y() - line_height_ * num_of_lines);

  const int cursor_offset_new = GetCursorOffset(pixel_high);
  cursor_offset_ = selection_start_offset_ = selection_end_offset_ =
      cursor_offset_new;
}

void TextView::MoveLineDown(int num_of_lines) {
  const int current_line =
      text_document_->LineNumFromCharOffset(cursor_offset_);
  const int line_count = text_document_->GetLineCount();

  if (current_line + num_of_lines >= line_count)
    num_of_lines = line_count - current_line - 1;

  const QPoint pixel_high = GetPixelPosition(cursor_offset_);
  QPoint pixel_low = pixel_high;
  pixel_low.setY(pixel_high.y() + line_height_ * num_of_lines);

  const int cursor_offset_new = GetCursorOffset(pixel_low);
  cursor_offset_ = selection_start_offset_ = selection_end_offset_ =
      cursor_offset_new;
}

void TextView::GoPageUp() {
  const int lines_in_window_max = viewport()->height() / line_height_;
  MoveLineUp(lines_in_window_max + 1);
}

void TextView::GoPageDown() {
  const int lines_in_window_max = viewport()->height() / line_height_;
  MoveLineDown(lines_in_window_max + 1);
}

void TextView::Cut() {
  QClipboard* clipboard = QGuiApplication::clipboard();
  const size_t start = std::min(selection_start_offset_, selection_end_offset_);
  const size_t end = std::max(selection_start_offset_, selection_end_offset_);
  const size_t len = end - start;
  if (len == 0) return;
  clipboard->setText(QString(text_document_->GetText(start, len)));

  text_document_->EraseText(start, len);
  cursor_offset_ = start;
  selection_start_offset_ = selection_end_offset_ = cursor_offset_;
}

void TextView::Copy() {
  QClipboard* clipboard = QGuiApplication::clipboard();
  const size_t start = std::min(selection_start_offset_, selection_end_offset_);
  const size_t end = std::max(selection_start_offset_, selection_end_offset_);
  const size_t len = end - start;
  if (len == 0) return;
  clipboard->setText(QString(text_document_->GetText(start, len)));
}

void TextView::Paste() {
  QClipboard* clipboard = QGuiApplication::clipboard();
  const size_t start = std::min(selection_start_offset_, selection_end_offset_);
  const size_t end = std::max(selection_start_offset_, selection_end_offset_);
  const size_t len = end - start;
  auto clipboard_text = clipboard->text();
  std::vector<char16_t> chars(clipboard_text.length());
  for (int i = 0; i < clipboard_text.length(); ++i) {
    chars[i] = clipboard_text.at(i).unicode();
  }
  const auto normalized_text = NormalizeLineEndings(chars);
  if (start != end)
    text_document_->ReplaceText(start, normalized_text, len);
  else
    text_document_->InsertText(start, NormalizeLineEndings(chars));
  cursor_offset_ = start + normalized_text.size();
  selection_start_offset_ = selection_end_offset_ = cursor_offset_;
}

void TextView::Undo() {
  cursor_offset_ = text_document_->Undo();
  selection_start_offset_ = selection_end_offset_ = cursor_offset_;
}

void TextView::Redo() {
  cursor_offset_ = text_document_->Redo();
  selection_start_offset_ = selection_end_offset_ = cursor_offset_;
}

void TextView::ToggleCursorVisibility() {
  caret_visible_ = !caret_visible_;
  viewport()->update();
}

void TextView::UpdateScrollBars() {
  QSize area_size = viewport()->size();
  int content_width = LongestLineLength() + margin_right_;
  int content_height = text_document_->GetLineCount() * line_height_;

  const int vertical_max = std::max(0, content_height - area_size.height());
  const int horizontal_max = std::max(0, content_width - area_size.width());

  verticalScrollBar()->setSingleStep(line_height_);
  horizontalScrollBar()->setSingleStep(20);
  verticalScrollBar()->setPageStep(area_size.height());
  horizontalScrollBar()->setPageStep(area_size.width());
  verticalScrollBar()->setRange(0, vertical_max);
  horizontalScrollBar()->setRange(0, horizontal_max);
}

bool TextView::viewportEvent(QEvent* event) {
  if (event->type() == QEvent::Resize) {
    UpdateScrollBars();
  }
  return QAbstractScrollArea::viewportEvent(event);
}

void TextView::paintEvent(QPaintEvent* event) {
  QPainter painter(viewport());
  painter.setPen(Qt::black);
  painter.setFont(current_font_);
  OnPaintText(&painter);

  // paint caret
  if (caret_visible_) {
    QPoint cursor_position = GetPixelPosition(cursor_offset_);
    QFontMetrics fm(current_font_);
    QRect cursor_rect(cursor_position.x(), cursor_position.y(), 2,
                      line_height_);
    painter.fillRect(cursor_rect, Qt::black);
  }
  if (selection_start_offset_ != selection_end_offset_) {
    const int vertical_offset = verticalScrollBar()->value();
    const int horizontal_offset = horizontalScrollBar()->value();
    const size_t begin_offset =
        std::min(selection_start_offset_, selection_end_offset_);
    const size_t end_offset =
        std::max(selection_start_offset_, selection_end_offset_);

    // paint selected text's background
    if (begin_offset != end_offset) {
      painter.setBrush(QColor(0, 120, 215, 128));
      const QPen old_pen_style = painter.pen();
      painter.setPen(Qt::NoPen);
      QFontMetrics fm(current_font_);
      const size_t start_line = std::max(
          (size_t)0, text_document_->LineNumFromCharOffset(begin_offset));
      const size_t end_line =
          std::min(text_document_->GetLineCount() - 1,
                   text_document_->LineNumFromCharOffset(end_offset));
      for (size_t i = start_line; i <= end_line; ++i) {
        size_t line_start_offset = 0;
        int line_y = i * line_height_;
        QString line_text = QString(
            text_document_
                ->IterateLineByLineNumber(i, &line_start_offset, nullptr)
                .GetLine());
        size_t start = 0;
        size_t end = line_text.size();
        if (i == start_line) {
          start = begin_offset - line_start_offset;
        }
        if (i == end_line) {
          end = end_offset - line_start_offset;
          ;
        }
        const int start_x = fm.horizontalAdvance(line_text.left(start));
        const int end_x = fm.horizontalAdvance(line_text.left(end));
        int selection_length = end_x - start_x;

        // highlight the new line charater('\n')
        if (i != end_line) {
          selection_length += fm.horizontalAdvance(QChar(' '));
        }
        painter.drawRect(start_x - horizontal_offset, line_y - vertical_offset,
                         selection_length, line_height_);
      }
      painter.setPen(old_pen_style);
    }
  }
}

void TextView::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) {
    QAbstractScrollArea::mousePressEvent(event);
    return;
  }
  int new_offset = GetCursorOffset(event->pos());

  cursor_offset_ = selection_start_offset_ = selection_end_offset_ = new_offset;
  is_mouse_pressed = true;
  setFocus();
  viewport()->update();
}

void TextView::mouseReleaseEvent(QMouseEvent* event) {
  auto_scroll_timer_->stop();
  // if (is_mouse_pressed) {
  //   is_mouse_pressed = false;
  //   cursor_offset_ = selection_end_offset_ = GetCursorOffset(event->pos());
  //   viewport()->update();
  // }
  QAbstractScrollArea::mouseReleaseEvent(event);
}

void TextView::mouseMoveEvent(QMouseEvent* event) {
  // if (is_mouse_pressed) {
  //   cursor_offset_ = selection_end_offset_ = GetCursorOffset(event->pos());
  //   viewport()->update();
  // }
  last_mouse_pos_ = event->pos();
  QRect viewport_rect = viewport()->rect();

  if (event->buttons() & Qt::LeftButton) {
    if (!viewport_rect.contains(last_mouse_pos_)) {
      if (!auto_scroll_timer_->isActive()) {
        auto_scroll_timer_->start(30);  // 每 30ms 滚动一次
      }
    } else {
      auto_scroll_timer_->stop();
    }

    // 更新选择逻辑（计算当前的偏移并 update）
    UpdateSelectionOnMouseMove(last_mouse_pos_);
  }
}

void TextView::mouseDoubleClickEvent(QMouseEvent* event) {
  const int offset = GetCursorOffset(event->pos());
  size_t line_start_offset = 0;
  auto itor = text_document_->IterateLineByCharOffset(
      offset, nullptr,
      &line_start_offset);  // select all text in current line.
  selection_start_offset_ = line_start_offset;
  selection_end_offset_ = cursor_offset_ =
      line_start_offset + NormalizeLineEndings(itor.GetLine()).size();
}

void TextView::wheelEvent(QWheelEvent* event) {
  // 获取滚动的角度增量
  // angleDelta().y() 正数向上滚，负数向下滚
  int delta = event->angleDelta().y();

  // 自定义步长系数，比如你希望滚动速度是默认的 2 倍
  const int scroll_speed_multiplier = 3;

  // 计算新的滚动值
  int newValue =
      verticalScrollBar()->value() - (delta * scroll_speed_multiplier / 8);

  // 手动设置滚动条值
  verticalScrollBar()->setValue(newValue);

  event->accept();
}

void TextView::contextMenuEvent(QContextMenuEvent* event) {
  QMenu context_menu(tr("Context Menu"), this);

  QAction* copy_action = context_menu.addAction(tr("Copy"));
  QAction* cut_action = context_menu.addAction(tr("Cut"));
  QAction* paste_action = context_menu.addAction(tr("Paste"));
  context_menu.addSeparator();

  QAction* undo_action = context_menu.addAction(tr("Undo"));
  QAction* redo_action = context_menu.addAction(tr("Redo"));

  bool has_selection = (selection_start_offset_ != selection_end_offset_);
  copy_action->setEnabled(has_selection);
  cut_action->setEnabled(has_selection);

  QClipboard* clipboard = QGuiApplication::clipboard();
  paste_action->setEnabled(clipboard->text().length() > 0);

  undo_action->setEnabled(text_document_->CanUndo());
  redo_action->setEnabled(text_document_->CanRedo());

  QAction* selected_action = context_menu.exec(mapToGlobal(event->pos()));

  if (selected_action == copy_action) {
    Copy();
  } else if (selected_action == cut_action) {
    Cut();
  } else if (selected_action == paste_action) {
    Paste();
  } else if (selected_action == undo_action) {
    Undo();
  } else if (selected_action == redo_action) {
    Redo();
  }

  if (selected_action) {
    UpdateScrollBars();
    ScrollToCursor();
  }
}

void TextView::keyPressEvent(QKeyEvent* event) {
  bool controllIsPressed =
      event->modifiers() &
      Qt::ControlModifier;  // todo: 更多的组合键事件, Shift/Alt
  if (controllIsPressed) {
    if (HandleCtrlKey(event)) {
      return;
    }
  }
  bool event_handled = false;
  switch (event->key()) {
    case Qt::Key_Backspace:
      if (selection_start_offset_ != selection_end_offset_) {
        DeleteSelection();
      } else if (cursor_offset_ > 0) {
        if (text_document_->EraseText(cursor_offset_ - 1, 1)) {
          cursor_offset_ -= 1;
          selection_end_offset_ = selection_start_offset_ = cursor_offset_;
          SyncCursor();
        }
      }
      break;
    case Qt::Key_Delete:
      if (selection_start_offset_ != selection_end_offset_) {
        DeleteSelection();
      } else if (cursor_offset_ > 0) {
        if (text_document_->EraseText(cursor_offset_, 1)) {
          SyncCursor();
        }
      }
      break;
    case Qt::Key_Left:
      UnSelect();
      if (cursor_offset_ > 0) {
        cursor_offset_ -= 1;
        SyncCursor();
      }
      break;
    case Qt::Key_Right:
      UnSelect();
      if (cursor_offset_ < text_document_->GetDocLength()) {
        cursor_offset_ += 1;
        SyncCursor();
      }

      break;
    case Qt::Key_Up:
      UnSelect();
      MoveLineUp();
      SyncCursor();
      break;
    case Qt::Key_Down:
      UnSelect();
      MoveLineDown();
      SyncCursor();
      break;
    case Qt::Key_PageUp:
      UnSelect();
      GoPageUp();
      SyncCursor();
      break;
    case Qt::Key_PageDown:
      UnSelect();
      GoPageDown();
      SyncCursor();
      break;
    case Qt::Key_Home:
      UnSelect();
      MoveLineStart();
      SyncCursor();
      break;
    case Qt::Key_End:
      UnSelect();
      MoveLineEnd();
      SyncCursor();
      break;
    default:
      QString text = event->text();
      HandleInputText(text);
      SyncCursor();
  }
  // QAbstractScrollArea::keyPressEvent(event);
}

void TextView::inputMethodEvent(QInputMethodEvent* event) {
  QString preedit = event->preeditString();
  QString commit = event->commitString();

  HandleInputText(commit);
  SyncCursor();
  QInputMethodQueryEvent query(Qt::ImCursorRectangle);
  QCoreApplication::sendEvent(this, &query);
}

QVariant TextView::inputMethodQuery(Qt::InputMethodQuery query) {
  switch (query) {
    case Qt::ImEnabled:
      return QVariant(true);
    // case Qt::ImCursorRectangle:
    //     return cursorRect();
    default:
      return QAbstractScrollArea::inputMethodQuery(query);
  }
}

void TextView::OnResetLineHeight() {
  QFontMetrics fm(current_font_);
  line_height_ = fm.height() + 2;
}
