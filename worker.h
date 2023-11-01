#ifndef WORKER_H
#define WORKER_H

#include "workertypes.h"
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

class TSNodeIterable {

public:
    TSNodeIterable(TSNode _source);
    ~TSNodeIterable();

    Child operator()();

private:
    Child node_next_child(TSTreeCursor *cursor, TSNode source);

    TSNode source;
    Child current;
    TSTreeCursor cursor;

};

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
    void finished(Tree tree);

private:
    TSNodeIterable iter_children(TSNode source);
    Range node_range(TSNode source);
    bool child_is_empty(Child child);
    void traverse(TSNode node, uint32_t depth, Tree *tree);

    Tree m_tree;
    std::unique_ptr<QThread> m_thread;
};

#endif // WORKER_H
