#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "worker.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Worker *m_worker;

    Ui::MainWindow *ui;

protected:
    void keyPressEvent(QKeyEvent *event);
};
#endif // MAINWINDOW_H
