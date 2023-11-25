#ifndef WORKER_H
#define WORKER_H

#include "treesitter.h"
#include <QObject>
#include <QDebug>
#include <QThread>
#include <QTextEdit>
#include <QPair>
#include <QStack>
#include <memory>

extern "C" {
    TSLanguage *tree_sitter_javascript();
}

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

public slots:
    void process(QString text);

private slots:
    void cleanup();

signals:
    void started();
    void finished(EditorNodeDescriptionList list);

private:
    EditorNodeDescriptionList editorNodeDescriptions(NodeTupleList list);

    Range node_range(Node *source);

    void loadQueries();

    QList<QString> m_queries;

    std::unique_ptr<QThread> m_thread;
};

#endif // WORKER_H
