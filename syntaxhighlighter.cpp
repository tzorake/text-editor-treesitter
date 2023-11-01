#include "syntaxhighlighter.h"
#include <QTextDocument>
#include <QTextBlock>
#include <QDebug>

SyntaxHighlighter::SyntaxHighlighter(QPlainTextEdit *source, QWidget *parent)
    : QObject(parent)
    , m_source(source)
{

}

void SyntaxHighlighter::highlight(const Tree &tree)
{
    auto doc = m_source->document();
    QTextCursor cursor(doc);

    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.setCharFormat(QTextCharFormat());

    for (auto node : tree) {
        uint32_t startRow = node.lnum;
        uint32_t startCol = node.col;
        uint32_t endRow = node.end_lnum;
        uint32_t endCol = node.end_col;

        if (node.named && strcmp(node.type, "identifier") == 0) {
            int position = translatePosition(startRow, startCol);
            cursor.setPosition(position, QTextCursor::MoveAnchor);

            QTextCharFormat format;
            format.setBackground(QColor(Qt::yellow));

            for (uint32_t row = startRow; row <= endRow; ++row) {
                uint32_t endColForRow = (row == endRow) ? endCol : m_source->document()->findBlockByLineNumber(row).length() - 1;
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, endColForRow - cursor.columnNumber());
                cursor.setCharFormat(format);

                if (row < endRow) {
                    cursor.movePosition(QTextCursor::Down);
                    cursor.movePosition(QTextCursor::StartOfLine);
                }
            }
        }
    }
}

int SyntaxHighlighter::translatePosition(uint32_t row, uint32_t column, bool local)
{
    QTextCursor cursor(m_source->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);

    return local ? cursor.positionInBlock() : cursor.position();
}
