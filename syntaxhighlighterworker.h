#ifndef SYNTAXHIGHLIGHTERWORKER_H
#define SYNTAXHIGHLIGHTERWORKER_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QPoint>
#include <memory>
#include <tree_sitter/api.h>

extern "C" {
    TSLanguage *tree_sitter_javascript();
}

class SyntaxHighlighterWorker : public QObject
{
    Q_OBJECT
public:
    enum NodeType {
        Unknown,
        String,
        Identifier,
        Comment,
    };
    Q_ENUM(NodeType)

    struct Point {
        uint32_t row;
        uint32_t column;
    };

    struct TSNodeInfo {
        NodeType type;
        Point start;
        Point end;
        uint32_t level;
    };

    explicit SyntaxHighlighterWorker(QObject *parent = nullptr);
    ~SyntaxHighlighterWorker();

    const QList<TSNodeInfo> &nodes();

public slots:
    void highlight(const QString &text);

private slots:
    void cleanup();

signals:
    void success();
    void error();
    void interrupted();

private:
    void traverseNode(const TSNode *node, uint32_t level = 0);
    NodeType nodeType(const char *type);

    QList<TSNodeInfo> m_nodes;
    std::unique_ptr<QThread> m_thread;
};

#endif // SYNTAXHIGHLIGHTERWORKER_H
