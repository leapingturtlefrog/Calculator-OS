// Math module for Calculator OS
// Supports Level 1: +, -, *, /, (), decimals, negatives
// Supports Level 2: ^, sqrt(), abs(), %

#include "math.h"

// External serial debug functions from kernel.c
extern void serial_puts(const char* s);
extern void serial_putdouble(double num);
extern void serial_putc(char c);

static const char* expr_ptr;
static const char* expr_end;

// Forward declarations
static double parse_expr(void);
static double parse_term(void);
static double parse_power(void);
static double parse_unary(void);
static double parse_primary(void);

static void skip_spaces(void) {
    while (expr_ptr < expr_end && *expr_ptr == ' ') expr_ptr++;
}

static int match(const char* s) {
    skip_spaces();
    const char* p = expr_ptr;
    while (*s && p < expr_end && *p == *s) { p++; s++; }
    if (*s == '\0') { expr_ptr = p; return 1; }
    return 0;
}

double math_sqrt(double x) {
    if (x < 0) return 0;
    if (x == 0) return 0;
    double guess = x / 2;
    for (int i = 0; i < 20; i++) {
        guess = (guess + x / guess) / 2;
    }
    return guess;
}

double math_pow(double base, double exp) {
    if (exp == 0) return 1;
    if (base == 0) return 0;
    
    // Handle negative exponents
    int neg = 0;
    if (exp < 0) { neg = 1; exp = -exp; }
    
    // Integer part
    int int_exp = (int)exp;
    double result = 1;
    for (int i = 0; i < int_exp; i++) {
        result *= base;
    }
    
    // Fractional part using sqrt approximation
    double frac = exp - int_exp;
    if (frac > 0 && base > 0) {
        // x^0.5 = sqrt(x), approximate other fractions
        double ln_base = 0;
        double term = (base - 1) / (base + 1);
        double term_sq = term * term;
        double current = term;
        for (int i = 0; i < 10; i++) {
            ln_base += current / (2 * i + 1);
            current *= term_sq;
        }
        ln_base *= 2;
        
        double exp_val = frac * ln_base;
        double frac_result = 1;
        double term2 = 1;
        for (int i = 1; i < 15; i++) {
            term2 *= exp_val / i;
            frac_result += term2;
        }
        result *= frac_result;
    }
    
    return neg ? 1 / result : result;
}

double math_abs(double x) {
    return x < 0 ? -x : x;
}

double math_mod(double a, double b) {
    if (b == 0) return 0;
    int q = (int)(a / b);
    return a - q * b;
}

// Parse a number (integer or decimal)
static double parse_number(void) {
    skip_spaces();
    double num = 0;
    int has_decimal = 0;
    double decimal_place = 0.1;
    
    while (expr_ptr < expr_end) {
        char c = *expr_ptr;
        if (c >= '0' && c <= '9') {
            if (!has_decimal) {
                num = num * 10 + (c - '0');
            } else {
                num += (c - '0') * decimal_place;
                decimal_place *= 0.1;
            }
            expr_ptr++;
        } else if (c == '.' && !has_decimal) {
            has_decimal = 1;
            expr_ptr++;
        } else {
            break;
        }
    }
    return num;
}

// Parse primary: numbers, parentheses, functions
static double parse_primary(void) {
    skip_spaces();
    
    // Check for functions
    if (match("sqrt(")) {
        double val = parse_expr();
        match(")");
        return math_sqrt(val);
    }
    if (match("abs(")) {
        double val = parse_expr();
        match(")");
        return math_abs(val);
    }
    if (match("root(")) {
        double n = parse_expr();
        match(",");
        double x = parse_expr();
        match(")");
        return math_pow(x, 1.0 / n);
    }
    
    // Parentheses
    if (match("(")) {
        double val = parse_expr();
        match(")");
        return val;
    }
    
    // Number
    return parse_number();
}

// Parse unary: -x, +x
static double parse_unary(void) {
    skip_spaces();
    if (match("-")) {
        return -parse_unary();
    }
    if (match("+")) {
        return parse_unary();
    }
    return parse_primary();
}

// Parse power: x^y (right associative)
static double parse_power(void) {
    double left = parse_unary();
    skip_spaces();
    if (match("^")) {
        double right = parse_power();  // Right associative
        return math_pow(left, right);
    }
    return left;
}

// Parse term: *, /, %
static double parse_term(void) {
    double left = parse_power();
    
    while (1) {
        skip_spaces();
        if (match("*")) {
            left *= parse_power();
        } else if (match("/")) {
            double right = parse_power();
            left = (right != 0) ? left / right : 0;
        } else if (match("%") || match("mod")) {
            left = math_mod(left, parse_power());
        } else {
            break;
        }
    }
    return left;
}

// Parse expression: +, -
static double parse_expr(void) {
    double left = parse_term();
    
    while (1) {
        skip_spaces();
        if (match("+")) {
            left += parse_term();
        } else if (match("-")) {
            left -= parse_term();
        } else {
            break;
        }
    }
    return left;
}

double evaluate(const char* expr, int len) {
    serial_puts("[MATH] evaluate() called, len=");
    serial_putdouble((double)len);
    serial_puts("\n");
    
    expr_ptr = expr;
    expr_end = expr + len;
    
    serial_puts("[MATH] calling parse_expr()...\n");
    double result = parse_expr();
    serial_puts("[MATH] parse_expr() returned: ");
    serial_putdouble(result);
    serial_puts("\n");
    
    return result;
}
