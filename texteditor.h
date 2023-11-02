#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include "worker.h"
#include "syntaxhighlighter.h"
#include <QObject>
#include <QPlainTextEdit>
#include <QTimer>

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit TextEditor(QWidget *parent = nullptr);
    ~TextEditor();

signals:
    void updateNodes(QString text);

private slots:
    void onTextChanged();

    void process();
    void handle(Tree tree);

private:
    SyntaxHighlighter *m_syntaxHighlighter = nullptr;

    std::shared_ptr<Worker> m_worker;
    QTimer *m_timer = nullptr;
};

#endif // TEXTEDITOR_H
