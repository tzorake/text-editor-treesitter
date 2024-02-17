#include "syntaxhighlighter.h"
#include "filereader.h"
#include <QTextDocument>
#include <QTextBlock>
#include <QRegularExpression>
#include <QDir>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SyntaxHighlighter::SyntaxHighlighter(QPlainTextEdit *parent)
    : QObject(parent)
    , m_source(parent)
{
    loadFormats();
}

void SyntaxHighlighter::highlight(const NodeDescriptionList &nodes)
{
    m_nodes = nodes;
    rehighlight();
}

void SyntaxHighlighter::rehighlight()
{
    auto doc = m_source->document();
    const QSignalBlocker blocker(doc);

    QTextCursor cursor(doc);

//    cursor.setPosition(0, QTextCursor::MoveAnchor);
//    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
//    cursor.setCharFormat(QTextCharFormat());

    uint32_t rendered = 0;

    for (auto node : m_nodes) {
        auto query_type = node.query_type;
        auto startRow = node.lnum;
        auto startCol = node.col;
        auto endRow = node.end_lnum;
        auto endCol = node.end_col;

        auto startPosition = translatePosition(startRow, startCol);
        auto endPosition = translatePosition(endRow, endCol);

        if (!(withinViewport(m_source, startPosition) || withinViewport(m_source, endPosition))) {
            continue;
        }

        auto position = translatePosition(startRow, startCol);
        cursor.setPosition(position, QTextCursor::MoveAnchor);

        auto format = m_formats[query_type];
        for (auto row = startRow; row <= endRow; ++row) {
            auto endColForRow = (row == endRow) ? endCol : m_source->document()->findBlockByLineNumber(row).length() - 1;
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, endColForRow - cursor.columnNumber());
            cursor.setCharFormat(format);

            if (row < endRow) {
                cursor.movePosition(QTextCursor::Down);
                cursor.movePosition(QTextCursor::StartOfLine);
            }
        }

        ++rendered;
    }

    qDebug() << rendered << m_nodes.size();
}

int SyntaxHighlighter::translatePosition(uint32_t row, uint32_t column, bool local)
{
    QTextCursor cursor(m_source->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);

    return local ? cursor.positionInBlock() : cursor.position();
}

bool SyntaxHighlighter::withinViewport(const QPlainTextEdit *textEdit, int position)
{
    QTextCursor cursor(textEdit->document());
    cursor.setPosition(position);

    auto rect = textEdit->cursorRect(cursor);
    auto viewport = textEdit->viewport()->rect();

    return viewport.intersects(rect);
}

void SyntaxHighlighter::loadFormats()
{
    QRegularExpression comment("//.*");

    QString source;
    QString path = QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + "resources" + QDir::separator() + "format.json");
    FileReader::readFile(path, source);

    source.replace(comment, "");

    QMap<QString, QTextCharFormat> formats;
    QJsonDocument document = QJsonDocument::fromJson(source.toUtf8());
    if(!document.isNull()) {
        if(document.isObject()) {
            QJsonObject object = document.object();

            for (const auto &key : object.keys()) {
                auto formatRef = object[key];
                if (!formatRef.isObject()) continue;
                auto formatObject = formatRef.toObject();

                QTextCharFormat format;

                auto backgroundRef = formatObject["background-color"];
                auto backgroundObject = backgroundRef.isString() ? backgroundRef.toString() : "#00000000";
                format.setBackground(QColor(backgroundObject));

                auto colorRef = formatObject["color"];
                auto colorObject = colorRef.isString() ? colorRef.toString() : "#ff000000";
                format.setForeground(QColor(colorObject));

                auto italicRef = formatObject["italic"];
                auto italicObject = italicRef.isBool() ? italicRef.toBool() : false;
                format.setFontItalic(italicObject);

                auto boldRef = formatObject["bold"];
                auto boldObject = boldRef.isBool() ? boldRef.toBool() : false;
                format.setFontWeight(boldObject);

                formats[key] = format;
            }

            m_formats = formats;
        } else {
            qWarning() << qPrintable(tr("SyntaxHighlighter::loadFormats(): document is not an object!"));
        }
    } else {
        qWarning() << qPrintable(tr("SyntaxHighlighter::loadFormats(): invalid document!"));
    }
}
