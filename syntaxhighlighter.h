#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include "treesitter.h"
#include <QObject>
#include <QPlainTextEdit>
#include <QMap>

class SyntaxHighlighter : public QObject
{
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QPlainTextEdit *parent = nullptr);

public slots:
    void highlight(const NodeDescriptionList &nodes);
    void rehighlight();

private:
    NodeDescriptionList m_nodes;

    int translatePosition(uint32_t row, uint32_t column, bool local = false);
    bool withinViewport(const QPlainTextEdit *textEdit, int position);

    void loadFormats();

    QMap<QString, QTextCharFormat> m_formats;
    QPlainTextEdit *m_source;
};

#endif // SYNTAXHIGHLIGHTER_H
