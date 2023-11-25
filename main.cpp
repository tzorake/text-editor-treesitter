#include "mainwindow.h"
#include "treesitter.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<EditorNodeDescriptionList>();

    MainWindow w;
    w.show();

    return a.exec();
}
