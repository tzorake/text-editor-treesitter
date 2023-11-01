#include "worker.h"
#include <string>
#include <QStringBuilder>
#include <QTimer>
#include <QApplication>

Worker::Worker(QObject *parent) : QObject(parent)
{
    m_thread.reset(new QThread);
    moveToThread(m_thread.get());
    m_thread->start();

    qRegisterMetaType<Tree>("Tree");
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
    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        source_code,
        strlen(source_code)
    );

    auto root = ts_tree_root_node(tree);
    auto nodes = Tree();
    traverse(root, 0, &nodes);

    Tree named;
    for (auto node : qAsConst(nodes)) {
        if (node.named) {
            named.append(node);
        }
    }

    m_tree = named;

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    emit finished(m_tree);
}

void Worker::cleanup()
{
    m_thread->quit();
}

TSNodeIterable Worker::iter_children(TSNode source)
{
    return TSNodeIterable(source);
}

Range Worker::node_range(TSNode source)
{
    auto start = ts_node_start_point(source);
    auto end = ts_node_end_point(source);

    return {
        start.row, start.column, end.row, end.column
    };
}

bool Worker::child_is_empty(Child child)
{
    return ts_node_is_null(child.first) && child.second == NULL;
}

void Worker::traverse(TSNode node, uint32_t depth, Tree *tree)
{
    TSNodeIterable next = iter_children(node);

    Child pair;
    while (!child_is_empty(pair = next())) {
        auto child = pair.first;
        auto field = pair.second;
        auto type = ts_node_type(child);

        auto range = node_range(child);
        uint32_t lnum = range.lnum, col = range.col, end_lnum = range.end_lnum, end_col = range.end_col;
        auto named = ts_node_is_named(child);

        tree->append({
         .id = child.id,
         .type = type,
         .field = field,
         .named = named,
         .depth = depth,
         .lnum = lnum,
         .col = col,
         .end_lnum = end_lnum,
         .end_col = end_col,
        });

        traverse(child, depth + 1, tree);
    }
}

TSNodeIterable::TSNodeIterable(TSNode _source)
    : source(_source)
    , cursor(ts_tree_cursor_new(source))
{

}

TSNodeIterable::~TSNodeIterable()
{
    ts_tree_cursor_delete(&cursor);
}

Child TSNodeIterable::operator()()
{
    return (current = node_next_child(&cursor, source));
}

Child TSNodeIterable::node_next_child(TSTreeCursor *cursor, TSNode source)
{
    if (!cursor) {
        return { TSNode{}, NULL };
    }

    if (ts_node_is_null(source)) {
        return { TSNode{}, NULL };
    }

    if (ts_node_eq(source, ts_tree_cursor_current_node(cursor))) {
        if (ts_tree_cursor_goto_first_child(cursor)) {
            goto push;
        } else {
            goto end;
        }
    }

    if (ts_tree_cursor_goto_next_sibling(cursor)) {
    push:
        auto node = ts_tree_cursor_current_node(cursor);
        const char *field = ts_tree_cursor_current_field_name(cursor);

        return { node, field };
    }

end:
    return { TSNode{}, NULL };
}
