Support operations in **layers**, from basic → “this can replace WolframAlpha-lite”.

---

## **Level 1 — Core Arithmetic (minimum viable calculator)**

These are non-negotiable.

* Addition: `+`
* Subtraction: `-`
* Multiplication: `*`
* Division: `/`
* Parentheses for order: `( )`
* Decimals: `3.14`
* Negative numbers: `-5`

**Example**

```
(3 + 4) * 2 - 5 / 10
```

---

## **Level 2 — Powers & Structure**

Makes it feel like a *real* calculator.

* Exponents: `^` → `2^10`
* Roots

  * Square root: `sqrt(x)`
  * General root: `root(n, x)` or `x^(1/n)`
* Absolute value: `abs(x)`
* Modulo: `mod` or `%` → `10 % 3`

---

## **Level 3 — Scientific Functions**

Now you're in scientific calculator territory.

### Trig

* `sin(x)`
* `cos(x)`
* `tan(x)`
* Inverses: `asin`, `acos`, `atan`

### Logs

* Natural log: `ln(x)`
* Log base 10: `log(x)`
* Log base n: `log(base, x)`

### Constants

* `pi`
* `e`

---

## **Level 4 — Advanced Math**

Useful for students, engineers, finance.

* Factorial: `n!`
* Permutations: `nPr(n, r)`
* Combinations: `nCr(n, r)`
* Floor/Ceil: `floor(x)`, `ceil(x)`
* Round: `round(x, n)`
* Min/Max: `min(a, b)`, `max(a, b)`

---

## **Level 5 — Algebra Support (HUGE upgrade)**

Now you're not just computing numbers — you're manipulating math.

* Variables: `x = 5`
* Expressions: `2x + 3`
* Substitution: `f(x) = x^2`, `f(3)`
* Solve equations:
  `solve(2x + 3 = 7, x)`

---

## **Level 6 — Linear Algebra (power users)**

* Vectors: `[1, 2, 3]`
* Dot product: `dot(a, b)`
* Cross product: `cross(a, b)`
* Matrices:

  * Multiply: `A * B`
  * Determinant: `det(A)`
  * Inverse: `inv(A)`

---

## **Level 7 — Calculus (elite tier)**

If you support this, it's basically a CAS.

* Derivatives: `d/dx(x^2)`
* Integrals: `integral(x^2, x)`
* Limits: `limit(x→0, sin(x)/x)`
* Summation: `sum(i=1 to 10, i^2)`

---

## **Level 8 — Programmer Mode**

Appeals to CS people.

* Bitwise ops: `& | ^ ~ << >>`
* Binary/hex: `0b1010`, `0xFF`
* Conversions: `bin(x)`, `hex(x)`

---

## **Level 9 — Financial (high real-world value)**

* Compound interest
  `FV(P, r, n, t)`
* Present value
* Payment calculators
* Growth rates

---

# 🔥 Practical Build Strategy

| Stage          | What to Ship |
| -------------- | ------------ |
| **V1**         | Levels 1–2   |
| **V2**         | + Level 3    |
| **V3**         | + Level 4    |
| **Power Mode** | Levels 5–7   |

Most calculators never go past Level 3.
Level 5+ is where you start competing with symbolic math tools.

---

If you want, next I can break down **how to parse expressions (parser design)** — that’s the real challenge, not the math.
