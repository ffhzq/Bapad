#ifndef BAPADQT_H
#define BAPADQT_H

#include <QApplication>
#include <QMainWindow>
#include "Textview.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class BapadQt;
}
QT_END_NAMESPACE

class BapadQt : public QMainWindow
{
    Q_OBJECT

public:
    explicit BapadQt(QWidget *parent = nullptr);
    ~BapadQt();

private slots:

    void on_NewFile_triggered();

    void on_OpenFile_triggered();

private:
    Ui::BapadQt *ui;
    TextView textview_;
    QString current_file_;
};
#endif // BAPADQT_H
