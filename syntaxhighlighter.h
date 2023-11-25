#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include "treesitter.h"
#include <QObject>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QMap>

class SyntaxHighlighter : public QObject
{
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QPlainTextEdit *source, QWidget *parent);

    void setFormats(QMap<QString, QTextCharFormat> formats);

public slots:
    void highlight(const EditorNodeDescriptionList &list);

private:
    void loadFormats();
    int translatePosition(uint32_t row, uint32_t column, bool local = false);

    QMap<QString, QTextCharFormat> m_formats;

    QPlainTextEdit *m_source;
};

#endif // SYNTAXHIGHLIGHTER_H
