#include "syntaxhighlighterworker.h"

#include <iostream>

SyntaxHighlighterWorker::SyntaxHighlighterWorker(QObject *parent) : QObject(parent)
{
    m_thread.reset(new QThread);
    moveToThread(m_thread.get());
    m_thread->start();
}

SyntaxHighlighterWorker::~SyntaxHighlighterWorker()
{
    QMetaObject::invokeMethod(this, "cleanup");
    m_thread->wait();
}

const QList<SyntaxHighlighterWorker::TSNodeInfo> &SyntaxHighlighterWorker::nodes()
{
    return m_nodes;
}

void SyntaxHighlighterWorker::highlight(const QString &text)
{
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_javascript());

    auto string = text.toStdString();
    const char *sourceCode = string.c_str();
    TSTree *tree = ts_parser_parse_string(
      parser,
      NULL,
      sourceCode,
      strlen(sourceCode)
    );

    TSNode root_node = ts_tree_root_node(tree);

    traverseNode(&root_node);

    emit success();
}

void SyntaxHighlighterWorker::cleanup()
{
    m_thread->quit();
}

void SyntaxHighlighterWorker::traverseNode(const TSNode *node, uint32_t level)
{
    if (ts_node_is_null(*node)) {
        return;
    }

    const char *stringType = ts_node_type(*node);
    auto type = nodeType(stringType);
    auto start = ts_node_start_point(*node);
    auto end = ts_node_end_point(*node);

    TSNodeInfo info = { type, { start.row, start.column }, { end.row, end.column }, level };

    m_nodes.append(info);

    for (uint32_t i = 0; i < ts_node_named_child_count(*node); ++i) {
        auto child = ts_node_named_child(*node, i);
        traverseNode(&child, level + 1);
    }
}

SyntaxHighlighterWorker::NodeType SyntaxHighlighterWorker::nodeType(const char *type)
{
    if (!strcmp(type, "string")) {
        return NodeType::String;
    }

    if (!strcmp(type, "identifier")) {
        return NodeType::Identifier;
    }

    if (!strcmp(type, "comment")) {
        return NodeType::Comment;
    }

    return NodeType::Unknown;
}
