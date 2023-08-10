#ifndef LEPTJSON_H__
#define LEPTJSON_H__

// ALL data type in json
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, 
LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,

    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,

    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_INVALID_UNICODE_HEX,

    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET
};  // the return value of the first api

// mem efficient way, v->n change to v->u.n or v->u.s/v->u.len
typedef struct lept_value lept_value;
struct lept_value{
    union{
        struct{ lept_value* e; size_t size }a;
        struct{ char* s; size_t len; }s;
        double n;
    }u;
    lept_type type;
} ;

typedef struct{
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;

#define lept_init(v) do{ (v)->type = LEPT_NULL; }while(0)
#define lept_set_null(v) lept_free(v)

// twp api func to parse json and access data
void lept_free(lept_value* v);

int lept_parse(lept_value* v, const char* json);  // char[] json and parse to tree
lept_type lept_get_type(const lept_value* v);

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

static const char* lept_parse_hex4(const char* p, unsigned *u);
static void lept_encode_utf8(lept_context* c, unsigned u);

size_t lept_get_array_size(const lept_value *v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif /*lept parse stack init size*/

static void* lept_context_push(lept_context* c, size_t size);
static void* lept_context_pop(lept_context* c, size_t size);

static int lept_parse_string(lept_context* c, lept_value* v);

static void lept_parse_whitespace(lept_context* c);
// static int lept_parse_null(lept_context*c, lept_value* v);
// static int lept_parse_false(lept_context*c, lept_value* v);
// static int lept_parse_true(lept_context*c, lept_value* v);
static int lept_parse_value(lept_context*c, lept_value* v);
static int lept_parse_number(lept_context*c, lept_value* v);
static int lept_parse_literal(lept_context*c, lept_value* v, const char* literal, lept_type type);
static int lept_parse_array(lept_context*c, lept_value* v);

// parse json data to tree, so we nedd a node
// typedef struct 
// {
//     char* s;
//     size_t lne;
//     double n;
//     lept_type type;
// }lept_value;

#endif /* LEPTJSON_H__ */