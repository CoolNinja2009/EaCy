/*
 * ============================================================================
 *  EaCyP - a more playful, dynamic layer on top of EaCy
 * ============================================================================
 *
 *  Usage:
 *      #include "EaCyP.h"
 *
 *  EaCyP keeps plain C11 compatibility while making small scripts and demos
 *  feel looser:
 *      var a, b, c;
 *      a = b = c = V(12);
 *      say("answer:", a);
 *
 *  C cannot literally change the grammar, so Python syntax such as bare
 *  dynamic `a = b = c = 12` is only possible for normal C variables. For
 *  dynamic variables, wrap values with V(...), or use let()/set()/set_all().
 *
 *  EaCyP also includes tiny module helpers:
 *      module(math);
 *      export int add(int a, int b) { return a + b; }
 * ============================================================================
 */

#ifndef EACYP_H
#define EACYP_H

#include "eacy.h"

typedef enum {
    ECP_NULL,
    ECP_BOOL,
    ECP_INT,
    ECP_LONG,
    ECP_DOUBLE,
    ECP_STRING,
    ECP_PTR
} ecp_type;

typedef struct {
    ecp_type type;
    union {
        bool b;
        int i;
        long l;
        double d;
        const char *s;
        void *p;
    } as;
} ecp_value;

typedef ecp_value var;

/*
 * Simple modules
 * --------------
 * Plain C cannot parse `module math;` as a macro-friendly construct, but
 * `module(math);` gets very close while remaining valid C11. `export` is a
 * header-only public function marker, and `private` is for internal helpers.
 */
#define module(name) \
    typedef struct EC_CONCAT(name, _module_tag) { int unused; } EC_CONCAT(name, _module)

#define module_name(name) #name
#define export EC_INLINE
#define private static

EC_INLINE ecp_value ecp_null(void) {
    ecp_value v;
    v.type = ECP_NULL;
    v.as.p = NULL;
    return v;
}

EC_INLINE ecp_value ecp_bool(bool x) {
    ecp_value v;
    v.type = ECP_BOOL;
    v.as.b = x;
    return v;
}

EC_INLINE ecp_value ecp_int(int x) {
    ecp_value v;
    v.type = ECP_INT;
    v.as.i = x;
    return v;
}

EC_INLINE ecp_value ecp_long(long x) {
    ecp_value v;
    v.type = ECP_LONG;
    v.as.l = x;
    return v;
}

EC_INLINE ecp_value ecp_double(double x) {
    ecp_value v;
    v.type = ECP_DOUBLE;
    v.as.d = x;
    return v;
}

EC_INLINE ecp_value ecp_string(const char *x) {
    ecp_value v;
    v.type = ECP_STRING;
    v.as.s = x ? x : "";
    return v;
}

EC_INLINE ecp_value ecp_ptr(void *x) {
    ecp_value v;
    v.type = ECP_PTR;
    v.as.p = x;
    return v;
}

#define V(x) _Generic((x), \
    bool:        ecp_bool,   \
    char:        ecp_int,    \
    int:         ecp_int,    \
    long:        ecp_long,   \
    float:       ecp_double, \
    double:      ecp_double, \
    char*:       ecp_string, \
    const char*: ecp_string, \
    default:     ecp_ptr     \
)(x)

#define none ecp_null()
#define let(name, value) var name = V(value)
#define set(name, value) ((name) = V(value))

#define ECP_SET_1(value, a)             (*(a) = (value))
#define ECP_SET_2(value, a, b)          ECP_SET_1(value, a); ECP_SET_1(value, b)
#define ECP_SET_3(value, a, b, c)       ECP_SET_2(value, a, b); ECP_SET_1(value, c)
#define ECP_SET_4(value, a, b, c, d)    ECP_SET_3(value, a, b, c); ECP_SET_1(value, d)
#define ECP_SET_5(value, a, b, c, d, e) ECP_SET_4(value, a, b, c, d); ECP_SET_1(value, e)
#define ECP_SET_6(value, a, b, c, d, e, f) \
    ECP_SET_5(value, a, b, c, d, e); ECP_SET_1(value, f)
#define ECP_SET_7(value, a, b, c, d, e, f, g) \
    ECP_SET_6(value, a, b, c, d, e, f); ECP_SET_1(value, g)
#define ECP_SET_8(value, a, b, c, d, e, f, g, h) \
    ECP_SET_7(value, a, b, c, d, e, f, g); ECP_SET_1(value, h)
#define ECP_SET_SELECT_(N) ECP_SET_##N
#define ECP_SET_SELECT(N) ECP_SET_SELECT_(N)

#define set_all(value, ...) \
    do { \
        ecp_value ecp_set_value_ = (value); \
        ECP_SET_SELECT(EC_ARG_COUNT(__VA_ARGS__))(ecp_set_value_, __VA_ARGS__); \
    } while (0)

EC_INLINE const char *type_of(var v) {
    switch (v.type) {
        case ECP_NULL:   return "null";
        case ECP_BOOL:   return "bool";
        case ECP_INT:    return "int";
        case ECP_LONG:   return "long";
        case ECP_DOUBLE: return "double";
        case ECP_STRING: return "string";
        case ECP_PTR:    return "ptr";
        default:         return "unknown";
    }
}

EC_INLINE double num(var v) {
    switch (v.type) {
        case ECP_BOOL:   return v.as.b ? 1.0 : 0.0;
        case ECP_INT:    return (double)v.as.i;
        case ECP_LONG:   return (double)v.as.l;
        case ECP_DOUBLE: return v.as.d;
        case ECP_STRING: return strtod(v.as.s, NULL);
        default:         return 0.0;
    }
}

EC_INLINE long integer(var v) {
    return (long)num(v);
}

EC_INLINE bool truthy(var v) {
    switch (v.type) {
        case ECP_NULL:   return false;
        case ECP_BOOL:   return v.as.b;
        case ECP_INT:    return v.as.i != 0;
        case ECP_LONG:   return v.as.l != 0;
        case ECP_DOUBLE: return v.as.d != 0.0;
        case ECP_STRING: return v.as.s && v.as.s[0] != '\0';
        case ECP_PTR:    return v.as.p != NULL;
        default:         return false;
    }
}

EC_INLINE void ecp_print(var v) {
    switch (v.type) {
        case ECP_NULL:   fputs("null", stdout); break;
        case ECP_BOOL:   fputs(v.as.b ? "true" : "false", stdout); break;
        case ECP_INT:    printf("%d", v.as.i); break;
        case ECP_LONG:   printf("%ld", v.as.l); break;
        case ECP_DOUBLE: printf("%g", v.as.d); break;
        case ECP_STRING: fputs(v.as.s ? v.as.s : "", stdout); break;
        case ECP_PTR:    printf("%p", v.as.p); break;
        default:         fputs("<unknown>", stdout); break;
    }
}

EC_INLINE void ecp_print_ptr_const_(const void *v) {
    printf("%p", v);
}

#define ecp_say_one(x) _Generic((x), \
    ecp_value:       ecp_print,          \
    bool:            ec_print_bool,      \
    char:            ec_print_char,      \
    int:             ec_print_int,       \
    long:            ec_print_long,      \
    unsigned int:    ec_print_uint,      \
    unsigned long:   ec_print_ulong,     \
    unsigned long long: ec_print_ullong, \
    float:           ec_print_float,     \
    double:          ec_print_double,    \
    char*:           ec_print_str,       \
    const char*:     ec_print_cstr,      \
    default:         ecp_print_ptr_const_ \
)(x)

#define say(...) \
    do { EC_FOR_EACH(ecp_say_one, ec_print_sep, __VA_ARGS__); putchar('\n'); } while (0)

EC_INLINE var add(var a, var b) {
    if (a.type == ECP_STRING || b.type == ECP_STRING) {
        return ecp_string("<string concat: use string_builder>");
    }
    return ecp_double(num(a) + num(b));
}

EC_INLINE var sub(var a, var b) { return ecp_double(num(a) - num(b)); }
EC_INLINE var mul(var a, var b) { return ecp_double(num(a) * num(b)); }
EC_INLINE var divv(var a, var b) { return ecp_double(num(a) / num(b)); }

#endif /* EACYP_H */
