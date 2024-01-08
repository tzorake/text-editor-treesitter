#include "texteditor.h"
#include <QScrollBar>

TextEditor::TextEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);

    m_scrollBarTimer = new QTimer(this);
    m_scrollBarTimer->setSingleShot(true);

    m_syntaxHighlighter = new SyntaxHighlighter(this);

    connect(document(), &QTextDocument::contentsChanged, this, &TextEditor::onTextChanged);
    connect(m_timer, &QTimer::timeout, this, &TextEditor::updateRequired);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TextEditor::onScrollBarValueChanged); // TextEditor::cursorPositionChanged
    connect(m_scrollBarTimer, &QTimer::timeout, m_syntaxHighlighter, &SyntaxHighlighter::rehighlight);
}

void TextEditor::updateRequired()
{
    m_content = toPlainText();
    emit refresh(m_content);
}

void TextEditor::handle(const NodeDescriptionList &list)
{
    m_syntaxHighlighter->highlight(list);
}

void TextEditor::onTextChanged()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_timer->start(500);
}

void TextEditor::onScrollBarValueChanged()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_timer->start(100);
}
