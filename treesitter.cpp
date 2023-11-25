#include "treesitter.h"

Tree::Tree(TSTree *tree, const char *source, int keep_text)
    : tree(tree)
    , source(keep_text ? source : NULL)
{

}

Node::Node(TSNode node, Tree *tree)
    : node(node)
    , tree(tree)
{

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
    char *slice = new char[len];
    if (slice == NULL) {
        delete[] slice;
        return NULL;
    }
    void *result = memcpy(slice, tree->source, len);
    if (result != NULL) {
        delete[] slice;
        return NULL;
    }

    return slice;
}

CaptureEqCapture::CaptureEqCapture(uint32_t capture1_value_id, uint32_t capture2_value_id, int is_positive)
    : capture1_value_id(capture1_value_id)
    , capture2_value_id(capture2_value_id)
    , is_positive(is_positive)
{

}

CaptureEqString::CaptureEqString(uint32_t capture_value_id, const char *string_value, int is_positive)
    : capture_value_id(capture_value_id)
    , string_value(string_value)
    , is_positive(is_positive)
{

}

CaptureMatchString::CaptureMatchString(uint32_t capture_value_id, const char *string_value, int is_positive)
    : capture_value_id(capture_value_id)
    , is_positive(is_positive)
{
    int result;
    regex_t rx;
    result = regcomp(&rx, string_value, 0);
    if (result) {
        regex = rx;
    }
}

CaptureMatchString::~CaptureMatchString()
{
    regfree(&regex);
}

Query::Query(TSLanguage *language, char *source, int length)
{
    predicate_list_t pattern_text_predicates;
    uint32_t error_offset;
    TSQueryError error_type;
    query = ts_query_new(language, source, length, &error_offset, &error_type);
    if (!query) {
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
                qDebug() << QStringLiteral("Invalid node type %1").arg(&source[error_offset]);
            } break;

            case TSQueryErrorField: {
                qDebug() << QStringLiteral("Invalid field name %1").arg(&source[error_offset]);
            } break;

            case TSQueryErrorCapture: {
                qDebug() << QStringLiteral("Invalid capture name %1").arg(&source[error_offset]);
            } break;

            default: {
                qDebug() << QStringLiteral("Invalid syntax at offset %1").arg(error_offset);
            }
        }
        *word_end = c;
        QUERY_NEW_INTERNAL_ERROR(query);
    }

    uint32_t n = ts_query_capture_count(query);
    kv_init(capture_names);
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t length;
        const char *capture_name = ts_query_capture_name_for_id(query, i, &length);
        char *slice = new char[length + 1];
        void *result = memcpy(slice, capture_name, length + 1);
        if (result == NULL) {
            QUERY_NEW_INTERNAL_ERROR(query);
        }

        kv_push(capture_name_t, capture_names, slice);
    }

    uint32_t pattern_count = ts_query_pattern_count(query);
    kv_init(text_predicates);

    for (uint32_t i = 0; i < pattern_count; ++i) {
        uint32_t length;
        const TSQueryPredicateStep *predicate_step =
            ts_query_predicates_for_pattern(query, i, &length);
        kv_init(pattern_text_predicates);

        for (uint32_t j = 0; j < length; j++) {
            uint32_t predicate_len = 0;
            while ((predicate_step + predicate_len)->type != TSQueryPredicateStepTypeDone) {
                predicate_len++;
            }

            if (predicate_step->type != TSQueryPredicateStepTypeString) {
                qDebug() << QStringLiteral(
                    "Capture predicate must start with a string i=%d/pattern_count=%d "
                    "j=%d/length=%d predicate_step->type=%d TSQueryPredicateStepTypeDone=%d "
                    "TSQueryPredicateStepTypeCapture=%d TSQueryPredicateStepTypeString=%d"
                )
                .arg(i)
                .arg(pattern_count)
                .arg(j)
                .arg(length)
                .arg(predicate_step->type)
                .arg(TSQueryPredicateStepTypeDone)
                .arg(TSQueryPredicateStepTypeCapture)
                .arg(TSQueryPredicateStepTypeString);
                QUERY_NEW_INTERNAL_ERROR(query);
            }

            // Build a predicate for each of the supported predicate function names
            uint32_t length;
            const char *operator_name =
                ts_query_string_value_for_id(query, predicate_step[0].value_id, &length);
            if (strcmp(operator_name, "eq?") == 0 || strcmp(operator_name, "not-eq?") == 0) {
                if (predicate_len != 3) {
                    qDebug() << QStringLiteral("Wrong number of arguments to #eq? or #not-eq? predicate");
                    QUERY_NEW_INTERNAL_ERROR(query);
                }
                if (predicate_step[1].type != TSQueryPredicateStepTypeCapture) {
                    qDebug() << QStringLiteral("First argument to #eq? or #not-eq? must be a capture name");
                    QUERY_NEW_INTERNAL_ERROR(query);
                }
                int is_positive = strcmp(operator_name, "eq?") == 0;
                switch (predicate_step[2].type) {
                    case TSQueryPredicateStepTypeCapture: {
                        CaptureEqCapture *capture_eq_capture_predicate =
                            new CaptureEqCapture(
                                predicate_step[1].value_id, predicate_step[2].value_id,
                                is_positive);
                        if (capture_eq_capture_predicate == NULL) {
                            QUERY_NEW_INTERNAL_ERROR(query);
                        }
                        kv_push(predicate_t, pattern_text_predicates, (CaptureBase *)capture_eq_capture_predicate);
                    } break;

                    case TSQueryPredicateStepTypeString: {
                        const char *string_value = ts_query_string_value_for_id(
                            query, predicate_step[2].value_id, &length);
                        CaptureEqString *capture_eq_string_predicate =
                            new CaptureEqString(
                                predicate_step[1].value_id, string_value, is_positive);
                        if (capture_eq_string_predicate == NULL) {
                            QUERY_NEW_INTERNAL_ERROR(query);
                        }
                        kv_push(predicate_t, pattern_text_predicates, (CaptureBase *)capture_eq_string_predicate);
                    } break;

                    default: {
                        qDebug() << QStringLiteral("Second argument to #eq? or #not-eq? must be a capture name or a string literal");
                        QUERY_NEW_INTERNAL_ERROR(query);
                    }
                }
            } else if (strcmp(operator_name, "match?") == 0 ||
                       strcmp(operator_name, "not-match?") == 0) {
                if (predicate_len != 3) {
                    qDebug() << QStringLiteral("Wrong number of arguments to #match? or #not-match? predicate");
                    QUERY_NEW_INTERNAL_ERROR(query);
                }
                if (predicate_step[1].type != TSQueryPredicateStepTypeCapture) {
                    qDebug() << QStringLiteral("First argument to #match? or #not-match? must be a capture name");
                    QUERY_NEW_INTERNAL_ERROR(query);
                }
                if (predicate_step[2].type != TSQueryPredicateStepTypeString) {
                    qDebug() << QStringLiteral("Second argument to #match? or #not-match? must be a regex string");
                    QUERY_NEW_INTERNAL_ERROR(query);
                }
                const char *string_value =
                    ts_query_string_value_for_id(query, predicate_step[2].value_id, &length);
                int is_positive = strcmp(operator_name, "match?") == 0;
                CaptureMatchString *capture_match_string_predicate =
                    new CaptureMatchString(
                        predicate_step[1].value_id, string_value, is_positive);
                if (capture_match_string_predicate == NULL) {
                    QUERY_NEW_INTERNAL_ERROR(query);
                }
                kv_push(predicate_t, pattern_text_predicates, (CaptureBase *)capture_match_string_predicate);
            }
            predicate_step += predicate_len + 1;
            j += predicate_len;
        }
        kv_push(predicate_list_t, text_predicates, pattern_text_predicates);
    }
}

Query::~Query()
{
    if (query) {
        ts_query_delete(query);
    }
}

QList<QPair<Node *, const char *>> Query::captures(TSQueryCursor *query_cursor, Node *node, TSPoint start_point, TSPoint end_point, uint32_t start_byte, uint32_t end_byte)
{
    ts_query_cursor_set_byte_range(query_cursor, start_byte, end_byte);
    ts_query_cursor_set_point_range(query_cursor, start_point, end_point);
    ts_query_cursor_exec(query_cursor, query, node->node);

    QList<QPair<Node *, const char *>> result;

    uint32_t capture_index;
    TSQueryMatch match;
    while (ts_query_cursor_next_capture(query_cursor, &match, &capture_index)) {
        auto capture = new QueryCapture(match.captures[capture_index]);
        if (satisfies_text_predicates(match, node->tree)) {
            auto capture_name = kv_A(capture_names, capture->capture.index);
            auto capture_node = new Node(capture->capture.node, node->tree);
            QPair<Node *, const char *> item = { capture_node, capture_name };
            result.append(item);
        }
    }
    return result;
}

Node *Query::node_for_capture_index(uint32_t index, TSQueryMatch match, Tree *tree)
{
    for (uint16_t i = 0; i < match.capture_count; i++) {
        TSQueryCapture capture = match.captures[i];
        if (capture.index == index) {
            auto capture_node = new Node(capture.node, tree);
            return capture_node;
        }
    }
    return NULL;
}

bool Query::satisfies_text_predicates(TSQueryMatch match, Tree *tree)
{
    predicate_list_t pattern_text_predicates = kv_A(text_predicates, match.pattern_index);

    Node *node1 = NULL;
    Node *node2 = NULL;
    const char *node1_text = NULL;
    const char *node2_text = NULL;
    for (uint32_t j = 0; j < kv_size(pattern_text_predicates); ++j) {
        predicate_t text_predicate = kv_A(pattern_text_predicates, j);
        int is_satisfied;
        if (instance_of_capture<CaptureEqCapture>(text_predicate)) {
            uint32_t capture1_value_id = ((CaptureEqCapture *)text_predicate)->capture1_value_id;
            uint32_t capture2_value_id = ((CaptureEqCapture *)text_predicate)->capture2_value_id;
            node1 = node_for_capture_index(capture1_value_id, match, tree);
            node2 = node_for_capture_index(capture2_value_id, match, tree);
            if (ts_node_is_null(node1->node) || ts_node_is_null(node2->node)) {
                goto error;
            }
            node1_text = node1->text();
            node2_text = node2->text();
            if (node1_text == NULL || node2_text == NULL) {
                goto error;
            }
            is_satisfied = (strcmp(node1_text, node2_text) == 0) ==
                           ((CaptureEqCapture *)text_predicate)->is_positive;
            if (!is_satisfied) {
                return false;
            }
        } else if (instance_of_capture<CaptureEqString>(text_predicate)) {
            uint32_t capture_value_id = ((CaptureEqString *)text_predicate)->capture_value_id;
            node1 = node_for_capture_index(capture_value_id, match, tree);
            if (node1 == NULL) {
                goto error;
            }
            node1_text = node1->text();
            if (node1_text == NULL) {
                goto error;
            }
            const char *string_value = ((CaptureEqString *)text_predicate)->string_value;
            is_satisfied = (strcmp(node1_text, string_value) == 0) ==
                           ((CaptureEqString *)text_predicate)->is_positive;
            if (!is_satisfied) {
                return false;
            }
        } else if (instance_of_capture<CaptureMatchString>(text_predicate)) {
            uint32_t capture_value_id = ((CaptureMatchString *)text_predicate)->capture_value_id;
            node1 = node_for_capture_index(capture_value_id, match, tree);
            if (node1 == NULL) {
                goto error;
            }
            node1_text = node1->text();
            if (node1_text == NULL) {
                goto error;
            }

            int search_result;
            regex_t regex = ((CaptureMatchString *)text_predicate)->regex;
            search_result = regexec(&regex, node1_text, 0, NULL, 0);

            is_satisfied = (search_result != REG_NOERROR && search_result != REG_NOMATCH ) ==
                           ((CaptureMatchString *)text_predicate)->is_positive;
            if (!is_satisfied) {
                return false;
            }
        }
    }
    return true;

error:
    free(node1);
    free(node2);
    free((char *)node1_text);
    free((char *)node2_text);
    return false;
}

QueryCapture::QueryCapture(TSQueryCapture capture)
    : capture(capture)
{

}

Language::Language(TSLanguage *language)
    : language(language)
{

}

Query *Language::language_query(char *source, size_t length)
{
    if (language == NULL) {
        return NULL;
    }
    return new Query(language, source, length);
}
