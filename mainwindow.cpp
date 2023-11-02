#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filereader.h"
#include "playground.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QTextDocument>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFont font("Roboto Mono", -1, QFont::Normal, false);

    ui->textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    ui->textEdit->setFont(font);

    ui->playground->setLineWrapMode(QPlainTextEdit::NoWrap);
    ui->playground->setFont(font);
    ui->playground->setSource(ui->textEdit);

    QString source;
    FileReader::readFile(QApplication::applicationDirPath() + QDir::separator() + "script.js", source);
    ui->textEdit->setPlainText(source);

    m_worker = new Worker();

    connect(ui->textEdit, &TextEditor::updateNodes, m_worker, &Worker::process);
    connect(m_worker, &Worker::finished, ui->textEdit, &TextEditor::handle);

    connect(ui->playground, &Playground::updateNodes, m_worker, &Worker::process);
    connect(m_worker, &Worker::finished, ui->playground, &Playground::handle);
}

MainWindow::~MainWindow()
{
    delete m_worker;
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    auto key = event->key();
    switch (key) {
        case Qt::Key_P: {
            ui->playground->setSource(ui->playground->source() ? nullptr : ui->textEdit);
        } break;

        default: {

        } break;
    }
}
