#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QThread>
#include <memory>
#include "treesitter.h"

// https://github.com/tsoding/arena

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

public slots:
    void process(const QString &text);
    NodeDescriptionList result();

private slots:
    void cleanup();

signals:
    void started();
    void finished(const NodeDescriptionList &result);

private:
    Range node_range(Node *source);

    NodeDescriptionList descriptions(const NodeTupleList &list);
    NodeDescriptionList m_result;

    void loadQueries();
    QList<QString> m_queries;

    TSParser *m_parser = nullptr;
    TSLanguage *m_language = nullptr;

    std::unique_ptr<State> m_state;
    std::unique_ptr<QThread> m_thread;
};

#endif // WORKER_H
