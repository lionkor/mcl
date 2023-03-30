# MCL

"My concise language" is a language which is purely stack-based, and looks reminiscent of some assembly dialects.
It comes with a compiler and interpreter, and generates executables in a binary format.

You can find the **full language reference** here: [Reference.md](./Reference.md)


## Examples

Find more examples in the [examples](./examples) directory.

Adding 123 and 456:
```nasm
push 123
push 456
add
print
halt
```
-> `579`

Adding 5 and 6, then multiplying by 1 (operator precedence):
```nasm
push 1
push 5
push 6
add
mul
print
halt
```
-> `11`

Multiplying 42 by itself by use of `dup`:
```nasm
push 42
dup
mul
print
halt
```
-> `84`


## Performance

So far benchmarks consist of compiling and seperately running examples/primes.mcl in a profiler, 
and examining the cost (cycle %, branch misprediction, instr fetch) of different parts of the code.

In Debug builds, performance suffers greatly due to very expensive correctness checks (each stack and each program counter 
increment is bounds-checked), and debug printouts of each instruction and the current stack. This is a development aide.

In Release builds, most checks are omitted (only `push` has a check to ensure it doesn't run out of bounds, which is the only 
issue in "correct" programs), and the performance is considerable.

### Primes example

It manages to iterate through (compute the modulo, compare the result) all numbers up to 100002493 in order to compute that it's a prime in about 4.5s on my Ryzen 5 4500U laptop processor.
This algorithm is hilariously inefficient, but easy to implement in any language, and thus it's what I use to "benchmark" progress in MCL's performance. 

The algorithm in python:
```py
p = 100002493

for i in range(2, p):
    if p % i == 0:
        print("0")
        exit()
print("1")
```
Takes about 7.76 seconds on the above machine.

The same algorithm in MCL:
```nasm
:start
# prime `p` to check
push 100002493
# start `i` at 1+1=2
push 1

:loop
push 1
# compute next number in iteration
add
# duplicate top two stack numbers with double-over
over
over
# is prime once we iterated through all numbers (p == i)
je :prime
over
over
# p % i == 0 ? not prime : try i+1
mod
push 0
je :not_prime
jmp :loop

:not_prime
push 0
print
halt

:prime
push 1
print
halt
```
Takes about 1.28 seconds (since v1.1.0, about 2x to 2.5x more time before) on the above machine.

In C, however:
```c
#include <stdio.h>
int main() {
    size_t p = 100002493;
    for (size_t i = 2; i < p; ++i) {
        if (p % i == 0) {
            printf("0");
            return 0;
        }
    }
    printf("1");
    return 0;
}
```
Takes about 0.37 seconds.

### Future Performance Optimizations

Possible future performance optimizations are:

- splitting the opcodes into more sub-sections, this may reduce the number of branch misses/mispredictions in the switch(opcode) switch statement.
- jit (obviously) to x86_64 (for example)
- making "as if" optimizations, so steering away from a "correct" and direct implementation of a stack-based interpreter, only keeping the stack semantics.


## Motivation

I enjoy writing parsers, interpreters, and solving other such problems. I enjoy designing languages, especially with the goal of simplicity.
MCL is supposed to be a research toy, an educational project, but also a real-world usable runtime.

The plan for the future is to add more functionality (variables, functions, types, calling functions from other binaries, etc.) and improving performance further.

