#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QApplication>
#include "filereader.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFont font("Roboto Mono", -1, QFont::Normal, false);

    ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    ui->plainTextEdit->setFont(font);

    QString source;
    QString path = QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + "resources" + QDir::separator() + "script.js");
    FileReader::readFile(path, source);
    ui->plainTextEdit->setPlainText(source);

    m_worker = new Worker();

    connect(ui->plainTextEdit, &TextEditor::refresh, m_worker, &Worker::process);
    connect(m_worker, &Worker::finished, ui->plainTextEdit, &TextEditor::handle);
}

MainWindow::~MainWindow()
{
    delete ui;
}
