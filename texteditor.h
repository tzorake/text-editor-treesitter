#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include "syntaxhighlighter.h"
#include <QPlainTextEdit>
#include <QTimer>

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit TextEditor(QWidget *parent = nullptr);

signals:
    void refresh(const QString &text);

public slots:
    void updateRequired();
    void handle(const NodeDescriptionList &list);

private slots:
    void onTextChanged();
    void onScrollBarValueChanged();

private:
    QString m_content;

    SyntaxHighlighter *m_syntaxHighlighter = nullptr;
    QTimer *m_timer = nullptr;
    QTimer *m_scrollBarTimer = nullptr;
};

#endif // TEXTEDITOR_H
