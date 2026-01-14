#include "BapadQt.h"

#include <QApplication>

#include <QAbstractScrollArea>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BapadQt w;
    w.show();
    return a.exec();
}
