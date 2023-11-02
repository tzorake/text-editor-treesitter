#include "texteditor.h"
#include <memory>

TextEditor::TextEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_worker = std::make_shared<Worker>();

    m_syntaxHighlighter = new SyntaxHighlighter(this, this);

    connect(this->document(), &QTextDocument::contentsChanged, this, &TextEditor::onTextChanged);
    connect(m_timer, &QTimer::timeout, this, &TextEditor::process);
    connect(this, &TextEditor::updateNodes, m_worker.get(), &Worker::process);
    connect(m_worker.get(), &Worker::finished, this, &TextEditor::handle);
}

TextEditor::~TextEditor()
{

}

void TextEditor::onTextChanged()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_timer->start(500);
}

void TextEditor::process()
{
    emit updateNodes(toPlainText());
}

void TextEditor::handle(Tree tree)
{
    document()->blockSignals(true);
    m_syntaxHighlighter->highlight(tree);
    document()->blockSignals(false);
}
