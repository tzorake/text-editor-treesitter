#ifndef TREESITTER_H
#define TREESITTER_H

#include "arena.h"
#include <tree_sitter/api.h>
#include <QList>
#include <QRegularExpression>
#include <QTranslator>
#include <QDebug>

class ArenaObject
{
public:
    friend class CaptureBase;
    friend class Tree;
    friend class Node;
    friend class QueryCapture;
    friend class Query;
    friend class Language;

    virtual void destroy() { }

private:
    virtual ~ArenaObject() = default;
};

class State {
public:
    explicit State();
    ~State() = default;

    void *allocate(size_t size_bytes);
    template <typename T>
    T* allocate();

    void dealloc();

private:
    QList<ArenaObject *> m_registry;

    Arena m_arena;
};

class CaptureBase : public ArenaObject
{
public:
    friend class CaptureEqCapture;
    friend class CaptureEqString;
    friend class CaptureMatchString;

private:
    virtual ~CaptureBase() = default;
};

typedef CaptureBase *Predicate;
typedef QList<Predicate> PredicateList;
typedef QList<PredicateList *> ListOfPredicateLists;

typedef const char *CaptureName;
typedef QList<CaptureName> CaptureNames;

template<class T>
bool instance_of_capture(CaptureBase *self)
{
    return dynamic_cast<T *>(self);
}

typedef struct {
    uint32_t lnum;
    uint32_t col;
    uint32_t end_lnum;
    uint32_t end_col;
} Range;

typedef struct {
    const char *query_type;
    uint32_t lnum;
    uint32_t col;
    uint32_t end_lnum;
    uint32_t end_col;
} NodeDescription;

typedef QList<NodeDescription> NodeDescriptionList;
Q_DECLARE_METATYPE(NodeDescriptionList);

class Tree : public ArenaObject
{
public:
    static Tree *create(State *state, TSTree *tree, const char *source, int keep_text);

    State *state;
    TSTree *tree;
    const char *source;

private:
    Tree(State *state, TSTree *tree, const char *source, int keep_text);
};

class Node : public ArenaObject
{
public:
    static Node *create(State *state, TSNode node, Tree *tree);

    const char *text();

    State *state;
    TSNode node;
    Tree *tree;

private:
    Node(State *state, TSNode node, Tree *tree);
};

typedef QPair<Node *, const char *> NodeTuple;
typedef QList<NodeTuple> NodeTupleList;
Q_DECLARE_METATYPE(NodeTupleList);

class CaptureEqCapture : public CaptureBase
{
public:
    static CaptureEqCapture *create(State *state, uint32_t capture1_value_id, uint32_t capture2_value_id, int is_positive);

    State *state;
    uint32_t capture1_value_id;
    uint32_t capture2_value_id;
    int is_positive;

private:
    CaptureEqCapture(State *state, uint32_t capture1_value_id, uint32_t capture2_value_id, int is_positive);
};

class CaptureEqString : public CaptureBase
{
public:
    static CaptureEqString *create(State *state, uint32_t capture_value_id, const char *string_value, int is_positive);

    State *state;
    uint32_t capture_value_id;
    const char *string_value;
    int is_positive;

private:
    CaptureEqString(State *state, uint32_t capture_value_id, const char *string_value, int is_positive);
};

class CaptureMatchString : public CaptureBase
{
public:
    static CaptureMatchString *create(State *state, uint32_t capture_value_id, const char *string_value, int is_positive);

    State *state;
    uint32_t capture_value_id;
    QRegularExpression regex;
    int is_positive;

private:
    CaptureMatchString(State *state, uint32_t capture_value_id, const char *string_value, int is_positive);
};

class QueryCapture : public ArenaObject
{
public:
    static QueryCapture *create(State *state, TSQueryCapture capture);

    State *state;
    TSQueryCapture capture;

private:
    QueryCapture(State *state, TSQueryCapture capture);
};

class Query : public ArenaObject {
public:
    static Query *create(State *state, TSLanguage *language, char *source, int length);
    void destroy() override;

    QList<QPair<Node *, const char *>> captures(
        TSQueryCursor *query_cursor,
        Node *node,
        TSPoint start_point = TSPoint{.row = 0, .column = 0},
        TSPoint end_point = TSPoint{.row = UINT32_MAX, .column = UINT32_MAX},
        uint32_t start_byte = 0,
        uint32_t end_byte = UINT32_MAX
    );

    State *state;
    TSQuery *query;
    CaptureNames capture_names;
    ListOfPredicateLists text_predicates;

private:
    Query(State *state, TSLanguage *language, char *source, int length);
    ~Query();
};

class Language : public ArenaObject {
public:
    static Language *create(State *state, TSLanguage *language);

    Query *query(char *source, size_t length);

    State *state;
    TSLanguage *language;

private:
    Language(State *state, TSLanguage *language);
};

#endif // TREESITTER_H
