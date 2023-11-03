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

TSTreeIterator Worker::iter_children(TSNode source)
{
    return TSTreeIterator(source);
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
    return ts_node_is_null(child.first) && strcmp(child.second, "");
}

void Worker::traverse(TSNode node, uint32_t depth, Tree *tree)
{
    auto it = iter_children(node);

    for (auto pair : it) {
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

TSTreeIterator::Iterator::Iterator(TSNode source, Child current, TSTreeCursor *cursor)
    : source(source)
    , current(current)
    , cursor(cursor)
{

}

TSTreeIterator::Iterator &TSTreeIterator::Iterator::operator++()
{
    current = node_next_child(cursor, source);
    return *this;
}

TSTreeIterator::Iterator TSTreeIterator::Iterator::operator++(int)
{
    Iterator retval = *this;
    ++(*this);
    return retval;
}

bool TSTreeIterator::Iterator::operator==(TSTreeIterator::Iterator other) const
{
    return ts_node_eq(current.first, other.current.first) && !strcmp(current.second, other.current.second);
}

bool TSTreeIterator::Iterator::operator!=(TSTreeIterator::Iterator other) const
{
    return !(*this == other);
}

Child TSTreeIterator::Iterator::operator*() const
{
    return current;
}

Child TSTreeIterator::Iterator::node_next_child(TSTreeCursor *cursor, TSNode source)
{
    if (!cursor) {
        return { TSNode{}, "" };
    }

    if (ts_node_is_null(source)) {
        return { TSNode{}, "" };
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

        return { node, field != NULL ? field : "" };
    }

end:
    return { TSNode{}, "" };
}

TSTreeIterator::TSTreeIterator(TSNode source)
    : source(source)
    , cursor(ts_tree_cursor_new(source))
{

}

TSTreeIterator::~TSTreeIterator()
{
    ts_tree_cursor_delete(&cursor);
}

TSTreeIterator::Iterator TSTreeIterator::begin()
{
    return Iterator(source, Iterator::node_next_child(&cursor, source), &cursor);
}

TSTreeIterator::Iterator TSTreeIterator::end()
{
    return Iterator(source, Child{ TSNode{}, "" }, &cursor);
}
