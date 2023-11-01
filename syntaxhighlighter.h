#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include "workertypes.h"
#include <QObject>
#include <QPlainTextEdit>
#include <QTextDocument>

class SyntaxHighlighter : public QObject
{
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QPlainTextEdit *source, QWidget *parent);

public slots:
    void highlight(const Tree &text);

private:
    int translatePosition(uint32_t row, uint32_t column, bool local = false);

    QPlainTextEdit *m_source;
};

#endif // SYNTAXHIGHLIGHTER_H
