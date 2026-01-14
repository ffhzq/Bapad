#include "BapadQt.h"

#include "FormatConversionV2.h"
#include "QFileDialog"
#include "QMessageBox"
#include "qfile.h"
#include "ui_BapadQt.h"

BapadQt::BapadQt(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::BapadQt), current_file_(), textview_() {
  ui->setupUi(this);
  QWidget* p = &textview_;
  setCentralWidget(p);
}

BapadQt::~BapadQt() { delete ui; }

void BapadQt::on_NewFile_triggered() {
  textview_.Clear();
  current_file_ = QString();
}

void BapadQt::on_OpenFile_triggered() {
  QString file_name = QFileDialog::getOpenFileName(this, "Open the file");
  if (file_name.isEmpty()) return;
  current_file_ = file_name;

  QFile file(file_name);

  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Can't open file: " << file_name << file.errorString();
    return;
  }

  QByteArray data = file.readAll();

  file.close();

  std::vector<char> raw_data(data.constBegin(), data.constEnd());
  int header_size = 0;
  auto data_type = DetectFileFormat(raw_data, header_size);
  std::vector<char16_t> utf16_data = RawToUtf16(raw_data, data_type);
  textview_.NewFile(utf16_data);
}
