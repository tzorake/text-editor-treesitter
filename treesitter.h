#ifndef TREESITTER_H
#define TREESITTER_H

#include <QList>
#include <QDebug>
#include <tree_sitter/api.h>
#include <gnu_regex/regex.h>
#include <kvec.h>

class CaptureBase;

typedef CaptureBase * predicate_t;
typedef kvec_t(predicate_t) predicate_list_t;
typedef kvec_t(predicate_list_t) list_of_predicate_lists_t;
typedef char * capture_name_t;
typedef kvec_t(capture_name_t) capture_names_t;

class Node;

typedef QPair<Node *, const char *> NodeTuple;
typedef QList<NodeTuple> NodeTupleList;

Q_DECLARE_METATYPE(NodeTupleList);

template<class T>
bool instance_of_capture(CaptureBase *self)
{
    return dynamic_cast<T *>(self) != nullptr;
}

struct Range
{
    uint32_t lnum;
    uint32_t col;
    uint32_t end_lnum;
    uint32_t end_col;
};

struct PlaygroundNodeDescription
{
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
typedef QList<PlaygroundNodeDescription> PlaygroundNodeDescriptionList;

struct EditorNodeDescription
{
    const void *id;
    const char *query_type;
    uint32_t lnum;
    uint32_t col;
    uint32_t end_lnum;
    uint32_t end_col;
};
typedef QList<EditorNodeDescription> EditorNodeDescriptionList;

class Tree
{
public:
    Tree(TSTree *tree, const char *source, int keep_text);

    TSTree *tree;
    const char *source;
};

class Node
{
public:
    Node(TSNode node, Tree *tree);

    const char *text();

    TSNode node;
    Tree *tree;
};

class CaptureBase
{
public:
    virtual ~CaptureBase() {}
};

class CaptureEqCapture : public CaptureBase
{
public:
    CaptureEqCapture(uint32_t capture1_value_id, uint32_t capture2_value_id, int is_positive);

    uint32_t capture1_value_id;
    uint32_t capture2_value_id;
    int is_positive;
};

class CaptureEqString : public CaptureBase
{
public:
    CaptureEqString(uint32_t capture_value_id, const char *string_value, int is_positive);

    uint32_t capture_value_id;
    const char *string_value;
    int is_positive;
};

class CaptureMatchString : public CaptureBase
{
public:
    CaptureMatchString(uint32_t capture_value_id, const char *string_value, int is_positive);
    ~CaptureMatchString();

    uint32_t capture_value_id;
    regex_t regex;
    int is_positive;
};

class QueryCapture
{
public:
    QueryCapture(TSQueryCapture capture);

    TSQueryCapture capture;
};

#define QUERY_NEW_INTERNAL_ERROR(query) \
    {                                   \
        ts_query_delete(query);         \
        return;                         \
    }

class Query
{
public:
    Query(TSLanguage *language, char *source, int length);
    ~Query();

    QList<QPair<Node *, const char *>> captures(
        TSQueryCursor *query_cursor,
        Node *node,
        TSPoint start_point = TSPoint{.row = 0, .column = 0},
        TSPoint end_point = TSPoint{.row = UINT32_MAX, .column = UINT32_MAX},
        uint32_t start_byte = 0,
        uint32_t end_byte = UINT32_MAX
    );

    TSQuery *query;
    capture_names_t capture_names;
    list_of_predicate_lists_t text_predicates;

private:
    Node *node_for_capture_index(uint32_t index, TSQueryMatch match, Tree *tree);
    bool satisfies_text_predicates(TSQueryMatch match, Tree *tree);
};

class Language
{
public:
    Language(TSLanguage *language);

    Query *language_query(char *source, size_t length);

    TSLanguage *language;
};

Q_DECLARE_METATYPE(EditorNodeDescriptionList);

#endif // TREESITTER_H
