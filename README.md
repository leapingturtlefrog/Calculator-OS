# Calculator OS v0.2

A minimal OS that boots to a calculator with Level 1 & 2 math support.

## Math Operations
- Basic: `+`, `-`, `*`, `/`, `%`
- Power: `2^10`
- Functions: `sqrt(16)`, `abs(-5)`, `root(3,27)`
- Parentheses: `(3+4)*2`
- Decimals: `3.14*2`
- Negatives: `-5+3`

## Extras
- `iching` - I Ching fortune
- `moji` - Random asciimoji  
- `lasagna` - ASCII art

## Build & Run
```
make        # build
make run    # run in QEMU (GUI)
make test   # run with curses + serial debug output
```

Requires: gcc (32-bit), nasm, qemu-system-i386
