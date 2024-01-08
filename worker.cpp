#include "worker.h"
#include "filereader.h"
#include "treesitter.h"
#include <QRegularExpression>
#include <QDir>
#include <QApplication>
#include <QJsonDocument>

extern "C" {
    TSLanguage *tree_sitter_javascript();
}

Worker::Worker(QObject *parent)
    : QObject(parent)
{
    loadQueries();

    m_thread.reset(new QThread);
    m_state.reset(new State);
    moveToThread(m_thread.get());
    m_thread->start();
}

Worker::~Worker()
{
    QMetaObject::invokeMethod(this, &Worker::cleanup);
    m_thread->wait();
}

void Worker::process(const QString &text)
{
    if (m_parser == nullptr) {
        m_parser = ts_parser_new();
    }

    if (m_language == nullptr) {
        m_language = tree_sitter_javascript();
    }

    if (m_parser != nullptr && m_language != nullptr) {
        ts_parser_set_language(m_parser, m_language);
    }

    m_result.clear();
    m_state->dealloc();

    auto string = text.toUtf8();
    auto source_code = string.constData();
    auto new_tree = ts_parser_parse_string(
        m_parser,
        NULL,
        source_code,
        strlen(source_code)
    );
    auto root = ts_tree_root_node(new_tree);

    auto language = Language::create(m_state.get(), m_language);
    auto tree_object = Tree::create(m_state.get(), new_tree, source_code, true);
    auto root_node = Node::create(m_state.get(), root, tree_object);

    for (const auto &query : m_queries) {
        auto string = query.toUtf8();
        auto c_str = string.constData();

        auto query_source = (char *)m_state->allocate(strlen(c_str) + 1);
        memcpy(query_source, c_str, strlen(c_str) + 1);

        auto language_query = language->query(
            query_source,
            strlen(query_source)
        );
        auto query_cursor = ts_query_cursor_new();

        auto captures = language_query->captures(query_cursor, root_node);
        auto descs = descriptions(captures);

        m_result.append(descs);
    }

    ts_tree_delete(new_tree);

    emit finished(m_result);
}

NodeDescriptionList Worker::result()
{
    return m_result;
}

void Worker::cleanup()
{
    if (m_parser) {
        ts_parser_delete(m_parser);
    }

    m_thread->quit();
}

Range Worker::node_range(Node *source)
{
    auto start = ts_node_start_point(source->node);
    auto end = ts_node_end_point(source->node);

    return Range{
        start.row, start.column, end.row, end.column
    };
}

NodeDescriptionList Worker::descriptions(const NodeTupleList &list)
{
    NodeDescriptionList nodes;

    std::transform(list.begin(), list.end(), std::back_inserter(nodes), [this](const NodeTuple &tuple) {
        auto query_type = tuple.second;
        auto node = tuple.first;
        auto range = node_range(node);
        uint32_t lnum = range.lnum, col = range.col, end_lnum = range.end_lnum, end_col = range.end_col;

        return NodeDescription{
            .query_type = query_type,
            .lnum = lnum,
            .col = col,
            .end_lnum = end_lnum,
            .end_col = end_col,
        };
    });

    return nodes;
}

void Worker::loadQueries()
{
    QRegularExpression comment("//.*");

    QString source;
    QString path = QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + "queries.json");
    FileReader::readFile(path, source);

    source = source.replace(comment, "");

    QJsonDocument document = QJsonDocument::fromJson(source.toUtf8());
    if(!document.isNull()) {
        if(document.isArray()) {
            m_queries = document.toVariant().toStringList();
        } else {
            qWarning() << qPrintable(tr("SyntaxHighlighter::loadQueries(): document is not an array!"));
        }
    } else {
        qWarning() << qPrintable(tr("SyntaxHighlighter::loadQueries(): invalid document!"));
    }
}

