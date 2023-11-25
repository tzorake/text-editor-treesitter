#include "playground.h"
#include <QDebug>
#include <QApplication>
#include <QStringBuilder>

Playground::Playground(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setTextInteractionFlags(Qt::NoTextInteraction);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);

    connect(m_timer, &QTimer::timeout, this, &Playground::process);
}

Playground::~Playground()
{

}

QPlainTextEdit *Playground::source()
{
    return m_source;
}

void Playground::setSource(QPlainTextEdit *textEdit)
{
    if (!textEdit) {
        setVisible(false);

        m_source = textEdit;
    } else {
        setVisible(true);

        if (m_source) {
            disconnect(m_source, &QPlainTextEdit::textChanged, this, &Playground::onTextChanged);
        }

        m_source = textEdit;
        connect(m_source, &QPlainTextEdit::textChanged, this, &Playground::onTextChanged);
        process();
    }
}

void Playground::onTextChanged()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_timer->start(500);
}

void Playground::process()
{
    if (m_source) {
        emit updateNodes(m_source->toPlainText());
    }
}

void Playground::handle(PlaygroundNodeDescriptionList list)
{
    QString result;

    for (auto node : qAsConst(list)) {
        auto type = node.type;
        auto named = node.named;
        auto field = node.field;

        auto text = named
            ? (field
                ? QStringLiteral("%1: (%2)").arg(field).arg(type)
                : QStringLiteral("(%1)").arg(type))
            : QStringLiteral("\"%1\"").arg(type);
        auto formatted = QStringLiteral("%1%2 %3")
            .arg(QStringLiteral("  ").repeated(node.depth))
            .arg(text)
            .arg(QStringLiteral("[%1, %2] - [%3, %4]\n")
                 .arg(node.lnum).arg(node.col).arg(node.end_lnum).arg(node.end_col)
            );

        result = result % formatted;
    }

    for (auto node : list) {
        delete node.type;
        delete node.field;
    }

    clear();
    insertPlainText(result);
}
