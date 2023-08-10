#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <error.h>
#include <math.h>
#include "leptjson.h"

#define EXPECT(c, ch) do { assert( *c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)     ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch) do{ *(char*)lept_context_push(c, sizeof(char)) = (ch); }while(0)
#define STRING_ERROR(ret) do{ c->top = head; return ret; }while(0)

int lept_parse(lept_value* v, const char* json){
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    v->type = LEPT_NULL;
    lept_init(v);
    lept_parse_whitespace(&c);
    // return lept_parse_value(&c, v);
    if((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK){
        lept_parse_whitespace(&c);
        if(*c.json != '\0'){
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

lept_type lept_get_type(const lept_value* v){
    assert(v != NULL);
    return v->type;
}

static void* lept_context_push(lept_context* c, size_t size){
    void* ret;
    assert(size > 0);
    if(c->top + size >= c->size){
        if(c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while(c->top + size >= c->size)
            c->size += c->size >> 1;    /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size){
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

// parse ws
static void lept_parse_whitespace(lept_context* c){
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p; 
}

// parse literals(true\false\null)
static int lept_parse_literal(lept_context*c, lept_value* v, const char* literal, lept_type type){
    size_t i;
    EXPECT(c, literal[0]);
    for(i = 0; literal[i + 1]; i++)
        if(c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

//parse number
static int lept_parse_number(lept_context*c, lept_value* v){
    const char* p = c->json;
    /* \TODO validate number */
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

//value = false/true/null
static int lept_parse_value(lept_context*c, lept_value* v){
    switch (*c->json)
    {
    case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
    case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
    case '"': return lept_parse_string(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    case '[': return lept_parse_array(c, v);
    default: return lept_parse_number(c, v);
    }
}

// string mem freeing
void lept_free(lept_value* v){
    assert( v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    if (v->type == LEPT_ARRAY)
        free(v->u.a.e);
    v->type = LEPT_NULL;
}

//number
void lept_set_number(lept_value* v, double n){
    assert(v != NULL);
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

double lept_get_number(const lept_value* v){
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

//boolean
int lept_get_boolean(const lept_value* v){
    assert(v != NULL && (v->type == LEPT_FALSE || v->type == LEPT_TRUE));
    return v->type ==LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b){
    assert(v != NULL);
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

//string
void lept_set_string(lept_value* v, const char* s, size_t len){
    assert(v != NULL && (s != NULL || len == 0) );
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}

const char* lept_get_string(const lept_value* v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

//array
size_t lept_get_array_size(const lept_value *v){
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index){
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];    
}

// unicode
static const char* lept_parse_hex4(const char* p, unsigned *u){
    int i;
    *u = 0;
    for(i = 0; i < 4; ++i){
        char ch = *p++;
        *u <<= 4;
        if ( ch >= '0' && ch <= '9' ) *u |= ch - '0';
        else if ( ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
        else if ( ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
        else return NULL;  
    }
    return p;

    /*
    strategy 2: using strtol(); 
    but only using the following code, \u 123 will pass, for strtol will skip the blank space
    */
    // char* end;
    // *u = (unsigned)strtol(p, &end, 16);
    // return end == p + 4 ? end : NULL;
}

void lept_encode_utf8(lept_context* c, unsigned u){
    if( u <= 0x7F )
        PUTC(c, u & 0xFF);
    else if ( u <= 0x7FF ){
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF) );
        PUTC(c, 0x80 | (u        & 0x3F));
    }
    else if ( u <= 0xFFFF ){
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF) );
        PUTC(c, 0x80 | ((u >> 6 ) & 0x3F) );
        PUTC(c, 0x80 | (u         & 0x3F) );
    }
    else{
        assert( u <= 0x10FFFF );
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}

static int lept_parse_string(lept_context* c, lept_value* v){
    size_t head = c->top, len;
    const char* p;
    unsigned u, u2;
    EXPECT(c, '\"');
    p = c->json;
    for(;;){
        char ch = *p++;
        switch(ch){
            case '\"':
                len = c->top - head;
                lept_set_string(v, (const char*)lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
                c->top = head;
                return LEPT_PARSE_MISS_QUOTATION_MARK;
            case '\\':
                switch(*p++){
                    case '\"' : PUTC(c, '\"');  break;
                    case '\\' : PUTC(c, '\\');  break;
                    case '/' : PUTC(c, '/');    break;
                    case 'b' : PUTC(c, '\b');  break;
                    case 'f' : PUTC(c, '\f');  break;
                    case 'n' : PUTC(c, '\n');  break;
                    case 'r' : PUTC(c, '\r');  break;
                    case 't' : PUTC(c, '\t');  break;

                    case 'u':
                        if( !(p = lept_parse_hex4(p, &u)) )
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        if( u>= 0xD800 && u <= 0xDBFF ){
                            if(*p++ != '\\')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if(*p++ != 'u')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if( !(p = lept_parse_hex4(p, &u2)) )
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                            if( u2 < 0xDC00 || u2 > 0xDFFF )
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        lept_encode_utf8(c, u);
                        break;

                    default:
                        // c->top = head;
                        // return LEPT_PARSE_INVALID_STRING_ESCAPE;
                        STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
            }
            break;
            default:
                if( (unsigned char)ch < 0x20 ){
                    c->top = head;
                    return LEPT_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c, ch);
        }
    }
}

//parse array
static int lept_parse_array(lept_context*c, lept_value* v){
    size_t size = 0;
    int ret;
    EXPECT(c, '[');
    lept_parse_whitespace(c);
    if(*c->json == ']'){
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
    }
    for(;;){
        lept_value e;
        lept_init(&e);
        if( (ret = lept_parse_value(c, &e) != LEPT_PARSE_OK) )
            return ret;
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        size++;
        lept_parse_whitespace(c);
        if(*c->json == ',')
            c->json++;
        else if(*c->json == ']'){
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            size *= sizeof(lept_value);
            memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
            return LEPT_PARSE_OK;
        }
        else
            return LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    }
}

// //parse null
// static int lept_parse_null(lept_context*c, lept_value* v){
//     EXPECT(c, 'n');
//     if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 3;
//     v->type = LEPT_NULL;
//     return LEPT_PARSE_OK;
// }

// //parse false
// static int lept_parse_false(lept_context*c, lept_value* v){
//     EXPECT(c, 'f');
//     if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 4;
//     v->type = LEPT_FALSE;
//     return LEPT_PARSE_OK;
// }

// //parse true
// static int lept_parse_true(lept_context*c, lept_value* v){
//     EXPECT(c, 't');
//     if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 3;
//     v->type = LEPT_TRUE;
//     return LEPT_PARSE_OK;
// }

