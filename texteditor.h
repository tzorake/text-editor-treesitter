#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

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

public slots:
    void process();
    void handle(EditorNodeDescriptionList list);

private:
    SyntaxHighlighter *m_syntaxHighlighter = nullptr;

    QTimer *m_timer = nullptr;
};

#endif // TEXTEDITOR_H
