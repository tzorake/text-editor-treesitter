#include "treesitter.h"

State::State()
    : m_arena{0, 0}
{

}

void *State::allocate(size_t size_bytes)
{
    return arena_alloc(&m_arena, size_bytes);
}

template<typename T>
T *State::allocate()
{
    auto item = (ArenaObject *)allocate(sizeof(T));
    m_registry.append(item);

    return reinterpret_cast<T *>(item);
}

void State::dealloc()
{
    while (m_registry.size() > 0) {
        auto item = m_registry.takeAt(0);
        if (item) {
            item->destroy();
        }
    }

    arena_free(&m_arena);
}

CaptureMatchString::CaptureMatchString(State *state, uint32_t capture_value_id, const char *string_value, int is_positive)
{
    this->state = state;
    this->capture_value_id = capture_value_id;
    this->is_positive = is_positive;

    QRegularExpression regex = std::move(QRegularExpression(string_value));
    if (regex.isValid()) {
        this->regex = regex;
    }
}

CaptureMatchString *CaptureMatchString::create(State *state, uint32_t capture_value_id, const char *string_value, int is_positive)
{
    auto self = state->allocate<CaptureMatchString>();
    return new (self)CaptureMatchString(state, capture_value_id, string_value, is_positive);
}

CaptureEqString::CaptureEqString(State *state, uint32_t capture_value_id, const char *string_value, int is_positive)
{
    this->state = state;
    this->capture_value_id = capture_value_id;
    this->string_value = string_value;
    this->is_positive = is_positive;
}

CaptureEqString *CaptureEqString::create(State *state, uint32_t capture_value_id, const char *string_value, int is_positive)
{
    auto self = state->allocate<CaptureEqString>();
    return new (self)CaptureEqString(state, capture_value_id, string_value, is_positive);
}

CaptureEqCapture::CaptureEqCapture(State *state, uint32_t capture1_value_id, uint32_t capture2_value_id, int is_positive)
{
    this->state = state;
    this->capture1_value_id = capture1_value_id;
    this->capture2_value_id = capture2_value_id;
    this->is_positive = is_positive;
}

CaptureEqCapture *CaptureEqCapture::create(State *state, uint32_t capture1_value_id, uint32_t capture2_value_id, int is_positive)
{
    auto self = state->allocate<CaptureEqCapture>();
    return new (self)CaptureEqCapture(state, capture1_value_id, capture2_value_id, is_positive);
}

#define QUERY_NEW_INTERNAL_ERROR(query) \
    {                                   \
        ts_query_delete(query);         \
        return;                         \
    }

Node *node_for_capture_index(State *state, uint32_t index, TSQueryMatch match, Tree *tree)
{
    for (uint16_t i = 0; i < match.capture_count; i++) {
        TSQueryCapture capture = match.captures[i];
        if (capture.index == index) {
            auto capture_node = Node::create(state, capture.node, tree);
            return capture_node;
        }
    }
    return NULL;
}

bool satisfies_text_predicates(State *state, Query *query, TSQueryMatch match, Tree *tree)
{
    PredicateList *pattern_text_predicates = query->text_predicates[match.pattern_index];

    Node *node1 = NULL;
    Node *node2 = NULL;
    const char *node1_text = NULL;
    const char *node2_text = NULL;
    for (uint32_t j = 0; j < (uint32_t)pattern_text_predicates->size(); ++j) {
        Predicate text_predicate = pattern_text_predicates->at(j);
        int is_satisfied;
        if (instance_of_capture<CaptureEqCapture>(text_predicate)) {
            uint32_t capture1_value_id = dynamic_cast<CaptureEqCapture *>(text_predicate)->capture1_value_id;
            uint32_t capture2_value_id = dynamic_cast<CaptureEqCapture *>(text_predicate)->capture2_value_id;
            node1 = node_for_capture_index(state, capture1_value_id, match, tree);
            node2 = node_for_capture_index(state, capture2_value_id, match, tree);
            if (ts_node_is_null(node1->node) || ts_node_is_null(node2->node)) {
                goto error;
            }
            node1_text = node1->text();
            node2_text = node2->text();
            if (node1_text == NULL || node2_text == NULL) {
                goto error;
            }
            is_satisfied = (strcmp(node1_text, node2_text) == 0) ==
                           dynamic_cast<CaptureEqCapture *>(text_predicate)->is_positive;
            if (!is_satisfied) {
                return false;
            }
        } else if (instance_of_capture<CaptureEqString>(text_predicate)) {
            uint32_t capture_value_id = dynamic_cast<CaptureEqString *>(text_predicate)->capture_value_id;
            node1 = node_for_capture_index(state, capture_value_id, match, tree);
            if (node1 == NULL) {
                goto error;
            }
            node1_text = node1->text();
            if (node1_text == NULL) {
                goto error;
            }
            const char *string_value = dynamic_cast<CaptureEqString *>(text_predicate)->string_value;
            is_satisfied = (strcmp(node1_text, string_value) == 0) ==
                           dynamic_cast<CaptureEqString *>(text_predicate)->is_positive;
            if (!is_satisfied) {
                return false;
            }
        } else if (instance_of_capture<CaptureMatchString>(text_predicate)) {
            uint32_t capture_value_id = dynamic_cast<CaptureMatchString *>(text_predicate)->capture_value_id;
            node1 = node_for_capture_index(state, capture_value_id, match, tree);
            if (node1 == NULL) {
                goto error;
            }
            node1_text = node1->text();
            if (node1_text == NULL) {
                goto error;
            }

            QRegularExpression regex = dynamic_cast<CaptureMatchString *>(text_predicate)->regex;
            QRegularExpressionMatch match = regex.match(node1_text);

            is_satisfied = (regex.isValid() && match.hasMatch()) ==
                           dynamic_cast<CaptureMatchString *>(text_predicate)->is_positive;
            if (!is_satisfied) {
                return false;
            }
        }
    }
    return true;

error:
    return false;
}

Query *Query::create(State *state, TSLanguage *language, char *source, int length)
{
    auto self = state->allocate<Query>();
    return new (self)Query(state, language, source, length);
}

void Query::destroy()
{
    this->~Query();
}

Query::Query(State *state, TSLanguage *language, char *source, int length)
{
    this->state = state;

    auto pattern_text_predicates = new QList<Predicate>();

    uint32_t error_offset;
    TSQueryError error_type;
    this->query = ts_query_new(language, source, length, &error_offset, &error_type);
    if (!this->query) {
        char *word_start = &source[error_offset];
        char *word_end = word_start;
        while (word_end < &source[length] &&
               (iswalnum(*word_end) || *word_end == '-' || *word_end == '_' || *word_end == '?' || *word_end == '.')) {
            word_end++;
        }
        char c = *word_end;
        *word_end = 0;
        switch (error_type) {
            case TSQueryErrorNodeType: {
                qWarning() << qPrintable(QObject::tr("Invalid node type %1").arg(&source[error_offset]));
            } break;

            case TSQueryErrorField: {
                qWarning() << qPrintable(QObject::tr("Invalid field name %1").arg(&source[error_offset]));
            } break;

            case TSQueryErrorCapture: {
                qWarning() << qPrintable(QObject::tr("Invalid capture name %1").arg(&source[error_offset]));
            } break;

            default: {
                qWarning() << qPrintable(QObject::tr("Invalid syntax at offset %1").arg(error_offset));
            }
        }
        *word_end = c;
        QUERY_NEW_INTERNAL_ERROR(this->query);
    }

    uint32_t n = ts_query_capture_count(this->query);
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t length;
        const char *capture_name = ts_query_capture_name_for_id(this->query, i, &length);
        char *slice = (char *)state->allocate(length + 1);
        void *result = memcpy(slice, capture_name, length + 1);
        if (result == NULL) {
            QUERY_NEW_INTERNAL_ERROR(this->query);
        }

        this->capture_names.append(slice);
    }

    uint32_t pattern_count = ts_query_pattern_count(this->query);

    for (uint32_t i = 0; i < pattern_count; ++i) {
        uint32_t length;
        const TSQueryPredicateStep *predicate_step =
            ts_query_predicates_for_pattern(this->query, i, &length);

        for (uint32_t j = 0; j < length; j++) {
            uint32_t predicate_len = 0;
            while ((predicate_step + predicate_len)->type != TSQueryPredicateStepTypeDone) {
                predicate_len++;
            }

            if (predicate_step->type != TSQueryPredicateStepTypeString) {
                qWarning() << qPrintable(
                    QObject::tr("Capture predicate must start with a string i=%1/pattern_count=%2 "
                       "j=%3/length=%4 predicate_step->type=%5 TSQueryPredicateStepTypeDone=%6 "
                       "TSQueryPredicateStepTypeCapture=%7 TSQueryPredicateStepTypeString=%8")
                    .arg(i)
                    .arg(pattern_count)
                    .arg(j)
                    .arg(length)
                    .arg(predicate_step[0].type)
                    .arg(TSQueryPredicateStepTypeDone)
                    .arg(TSQueryPredicateStepTypeCapture)
                    .arg(TSQueryPredicateStepTypeString));
                QUERY_NEW_INTERNAL_ERROR(this->query);
            }

            // Build a predicate for each of the supported predicate function names
            uint32_t length;
            const char *operator_name = ts_query_string_value_for_id(this->query, predicate_step[0].value_id, &length);
            if (strcmp(operator_name, "eq?") == 0 || strcmp(operator_name, "not-eq?") == 0) {
                if (predicate_len != 3) {
                    qWarning() << qPrintable(QObject::tr("Wrong number of arguments to #eq? or #not-eq? predicate"));
                    QUERY_NEW_INTERNAL_ERROR(this->query);
                }
                if (predicate_step[1].type != TSQueryPredicateStepTypeCapture) {
                    qWarning() << qPrintable(QObject::tr("First argument to #eq? or #not-eq? must be a capture name"));
                    QUERY_NEW_INTERNAL_ERROR(this->query);
                }
                int is_positive = strcmp(operator_name, "eq?") == 0;
                switch (predicate_step[2].type) {
                    case TSQueryPredicateStepTypeCapture: {
                        CaptureEqCapture *capture_eq_capture_predicate =
                            CaptureEqCapture::create(state, predicate_step[1].value_id, predicate_step[2].value_id, is_positive);
                        if (capture_eq_capture_predicate == NULL) {
                            QUERY_NEW_INTERNAL_ERROR(this->query);
                        }
                        pattern_text_predicates->append(capture_eq_capture_predicate);
                    } break;

                    case TSQueryPredicateStepTypeString: {
                        const char *string_value = ts_query_string_value_for_id(this->query, predicate_step[2].value_id, &length);
                        CaptureEqString *capture_eq_string_predicate =
                            CaptureEqString::create(state, predicate_step[1].value_id, string_value, is_positive);
                        if (capture_eq_string_predicate == NULL) {
                            QUERY_NEW_INTERNAL_ERROR(this->query);
                        }
                        pattern_text_predicates->append(capture_eq_string_predicate);
                    } break;

                    default: {
                        qWarning() << qPrintable(QObject::tr("Second argument to #eq? or #not-eq? must be a capture name or a string literal"));
                        QUERY_NEW_INTERNAL_ERROR(this->query);
                    }
                }
            } else if (strcmp(operator_name, "match?") == 0 || strcmp(operator_name, "not-match?") == 0) {
                if (predicate_len != 3) {
                    qWarning() << qPrintable(QObject::tr("Wrong number of arguments to #match? or #not-match? predicate"));
                    QUERY_NEW_INTERNAL_ERROR(this->query);
                }
                if (predicate_step[1].type != TSQueryPredicateStepTypeCapture) {
                    qWarning() << qPrintable(QObject::tr("First argument to #match? or #not-match? must be a capture name"));
                    QUERY_NEW_INTERNAL_ERROR(this->query);
                }
                if (predicate_step[2].type != TSQueryPredicateStepTypeString) {
                    qWarning() << qPrintable(QObject::tr("Second argument to #match? or #not-match? must be a regex string"));
                    QUERY_NEW_INTERNAL_ERROR(this->query);
                }
                const char *string_value = ts_query_string_value_for_id(this->query, predicate_step[2].value_id, &length);
                int is_positive = strcmp(operator_name, "match?") == 0;
                CaptureMatchString *capture_match_string_predicate =
                    CaptureMatchString::create(state, predicate_step[1].value_id, string_value, is_positive);
                if (capture_match_string_predicate == NULL) {
                    QUERY_NEW_INTERNAL_ERROR(this->query);
                }
                pattern_text_predicates->append(capture_match_string_predicate);
            }
            predicate_step += predicate_len + 1;
            j += predicate_len;
        }
        this->text_predicates.append(pattern_text_predicates);
    }
}

Query::~Query()
{
    if (query) {
        ts_query_delete(query);
    }

    while (text_predicates.size() > 0) {
        delete text_predicates.takeLast();
    }
}

QList<QPair<Node *, const char *> > Query::captures(TSQueryCursor *query_cursor, Node *node, TSPoint start_point, TSPoint end_point, uint32_t start_byte, uint32_t end_byte)
{
    ts_query_cursor_set_byte_range(query_cursor, start_byte, end_byte);
    ts_query_cursor_set_point_range(query_cursor, start_point, end_point);
    ts_query_cursor_exec(query_cursor, query, node->node);

    QList<QPair<Node *, const char *>> result;

    uint32_t capture_index;
    TSQueryMatch match;
    while (ts_query_cursor_next_capture(query_cursor, &match, &capture_index)) {
        auto capture = QueryCapture::create(state, match.captures[capture_index]);
        if (satisfies_text_predicates(state, this, match, node->tree)) {
            auto capture_name = capture_names[capture->capture.index];
            auto capture_node = Node::create(state, capture->capture.node, node->tree);
            QPair<Node *, const char *> item = { capture_node, capture_name };
            result.append(item);
        }
    }
    return result;
}

Node::Node(State *state, TSNode node, Tree *tree)
{
    this->state = state;
    this->node = node;
    this->tree = tree;
}

Node *Node::create(State *state, TSNode node, Tree *tree)
{
    auto self = state->allocate<Node>();
    return new (self)Node(state, node, tree);
}

const char *Node::text()
{
    if (tree == NULL || tree->source == NULL) {
        return NULL;
    }

    auto start_byte = ts_node_start_byte(node);
    if (start_byte == 0) {
        return NULL;
    }
    auto end_byte = ts_node_end_byte(node);
    if (end_byte == 0) {
        return NULL;
    }

    auto len = end_byte - start_byte + 1U;
    char *slice = (char *)state->allocate(len);
    if (slice == NULL) {
        return NULL;
    }
    void *result = memcpy(slice, tree->source, len);
    if (result != NULL) {
        return NULL;
    }

    return slice;
}

Language::Language(State *state, TSLanguage *language)
{
    this->state = state;
    this->language = language;
}

Language *Language::create(State *state, TSLanguage *language)
{
    auto self = state->allocate<Language>();
    return new (self)Language(state, language);
}

Query *Language::query(char *source, size_t length)
{
    if (language == NULL) {
        return NULL;
    }

    return Query::create(state, language, source, length);
}

QueryCapture::QueryCapture(State *state, TSQueryCapture capture)
{
    this->state = state;
    this->capture = capture;
}

QueryCapture *QueryCapture::create(State *state, TSQueryCapture capture)
{
    auto self = state->allocate<QueryCapture>();
    return new (self)QueryCapture(state, capture);
}

Tree::Tree(State *state, TSTree *tree, const char *source, int keep_text)
{
    this->state = state;
    this->tree = tree;
    this->source = keep_text ? source : nullptr;
}

Tree *Tree::create(State *state, TSTree *tree, const char *source, int keep_text)
{
    auto self = state->allocate<Tree>();
    return new (self)Tree(state, tree, source, keep_text);
}
