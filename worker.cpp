#include "worker.h"
#include "filereader.h"
#include <string>
#include <QStringBuilder>
#include <QTimer>
#include <QApplication>
#include <QRegularExpression>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Worker::Worker(QObject *parent) : QObject(parent)
{
    loadQueries();

    m_thread.reset(new QThread);
    moveToThread(m_thread.get());
    m_thread->start();
}

Worker::~Worker()
{
    QMetaObject::invokeMethod(this, "cleanup");
    m_thread->wait();
}

void Worker::process(QString text)
{
    auto parser = ts_parser_new();
    auto lang = tree_sitter_javascript();
    ts_parser_set_language(parser, lang);

    auto string = text.toUtf8();
    auto source_code = string.constData();
    auto tree = ts_parser_parse_string(
        parser,
        NULL,
        source_code,
        strlen(source_code)
    );

    auto root = ts_tree_root_node(tree);
    EditorNodeDescriptionList list;

    for (const auto &query : m_queries) {
        auto string = query.toStdString();
        auto c_str = string.c_str();

        auto parser = ts_parser_new();
        auto javascript = tree_sitter_javascript();
        ts_parser_set_language(parser, javascript);

        char *query_source = new char[strlen(c_str) + 1];
        memcpy(query_source, c_str, strlen(c_str) + 1);

        auto language = new Language(javascript);
        auto language_query = language->language_query(
            query_source,
            strlen(c_str)
        );
        auto query_cursor = ts_query_cursor_new();
        auto tree_object = new Tree(tree, source_code, true);
        auto root_node = new Node(root, tree_object);

        auto captures = language_query->captures(query_cursor, root_node);
        auto descriptions = editorNodeDescriptions(captures);

        list.append(descriptions);
    }

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    emit finished(list);
}

void Worker::cleanup()
{
    m_thread->quit();
}

EditorNodeDescriptionList Worker::editorNodeDescriptions(NodeTupleList list)
{
    EditorNodeDescriptionList nodes;

    std::transform(list.begin(), list.end(), std::back_inserter(nodes), [this](const NodeTuple &tuple) {
        auto node = tuple.first;
        auto query_type = tuple.second;
        auto range = node_range(node);
        uint32_t lnum = range.lnum, col = range.col, end_lnum = range.end_lnum, end_col = range.end_col;

        return EditorNodeDescription{
            .id = node->node.id,
            .query_type = query_type,
            .lnum = lnum,
            .col = col,
            .end_lnum = end_lnum,
            .end_col = end_col,
        };
    });

    return nodes;
}

Range Worker::node_range(Node *source)
{
    auto start = ts_node_start_point(source->node);
    auto end = ts_node_end_point(source->node);

    return Range{
        start.row, start.column, end.row, end.column
    };
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
            qDebug() << "SyntaxHighlighter::loadQueries(): document is not an array!";
        }
    } else {
        qDebug() << "SyntaxHighlighter::loadQueries(): invalid document!";
    }
}
