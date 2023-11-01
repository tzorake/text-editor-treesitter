#ifndef WORKERTYPES_H
#define WORKERTYPES_H

#include <QList>
#include <tree_sitter/api.h>

struct Range {
    uint32_t lnum;
    uint32_t col;
    uint32_t end_lnum;
    uint32_t end_col;
};

struct Node {
    const void *id;
    const char *type;
    const char *field;
    bool named;
    uint32_t depth;
    uint32_t lnum;
    uint32_t col;
    uint32_t end_lnum;
    uint32_t end_col;
};

typedef QList<Node> Tree;
typedef QPair<TSNode, const char *> Child;
typedef QList<Child> Children;

#endif // WORKERTYPES_H
