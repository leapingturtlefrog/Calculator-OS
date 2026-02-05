#ifndef MATH_H
#define MATH_H

// Parse and evaluate an expression from the input buffer
double evaluate(const char* expr, int len);

// Software sqrt implementation
double math_sqrt(double x);

// Software pow implementation
double math_pow(double base, double exp);

// Absolute value
double math_abs(double x);

// Modulo for doubles
double math_mod(double a, double b);

#endif
