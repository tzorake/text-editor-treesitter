#ifndef PLAYGROUND_H
#define PLAYGROUND_H

#include <QPlainTextEdit>
#include <QTimer>
#include "worker.h"

class Playground : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit Playground(QWidget *parent = nullptr);
    ~Playground();

    QPlainTextEdit *source();
    void setSource(QPlainTextEdit *textEdit);

signals:
    void updateNodes(QString text);

private slots:
    void onTextChanged();

    void process();
    void handle(Tree tree);

private:
    QPlainTextEdit *m_source = nullptr;

    std::shared_ptr<Worker> m_worker;
    QTimer *m_timer = nullptr;
};

#endif // PLAYGROUND_H
