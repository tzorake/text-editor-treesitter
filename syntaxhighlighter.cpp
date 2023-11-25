#include "syntaxhighlighter.h"
#include "filereader.h"
#include <QTextDocument>
#include <QTextBlock>
#include <QDebug>
#include <QApplication>
#include <QRegularExpression>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

SyntaxHighlighter::SyntaxHighlighter(QPlainTextEdit *source, QWidget *parent)
    : QObject(parent)
    , m_source(source)
{
    loadFormats();
}

void SyntaxHighlighter::highlight(const EditorNodeDescriptionList &list)
{
    auto doc = m_source->document();
    QTextCursor cursor(doc);

    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.setCharFormat(QTextCharFormat());

    for (auto node : list) {
        auto query_type = node.query_type;
        auto startRow = node.lnum;
        auto startCol = node.col;
        auto endRow = node.end_lnum;
        auto endCol = node.end_col;

        int position = translatePosition(startRow, startCol);
        cursor.setPosition(position, QTextCursor::MoveAnchor);

        QTextCharFormat format = m_formats[query_type];
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

void SyntaxHighlighter::loadFormats()
{
    QRegularExpression comment("//.*");

    QString source;
    QString path = QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + "format.json");
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

            setFormats(formats);
        } else {
            qDebug() << "SyntaxHighlighter::loadFormats(): document is not an object!";
        }
    } else {
        qDebug() << "SyntaxHighlighter::loadFormats(): invalid document!";
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

void SyntaxHighlighter::setFormats(QMap<QString, QTextCharFormat> formats)
{
    m_formats = formats;
}
