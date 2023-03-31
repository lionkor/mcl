# MCL Reference

# Language

## "Registers"

These are internal to the VM's "processor", and are modified by instructions. For example, `push` modifies `st`, and `jmp :somewhere` modifies `pc`.

- `pc`: Program counter
- `st`: Stack top (one above the top stack element)

## Instructions

Quick guide to reading the instructions:

- `SOMETHING` = instruction, usually typed lowercase
- `<IMM>` = immediate value, like `42`
- `<ADDR>` = address
- `<LBL>` = label (for example `:something`)

Normal instructions:

```
push <IMM>	pushes value on the stack.
pop		pops the top value from the stack.
print		prints the top value of the stack and pops it.
		side effect: prints the value to stdout, followed by a 
		newline.
add, sub, mul	adds/subtracts/multiplies the two top values on the stack, 
		and pops them off the stack. pushes the result of the operation
		on the stack.
div, mod	divides or performs modulo on the two top values on the stack, 
		and pops them off the stack. pushes the result of the operation
		on the stack. if the second argument is zero, it will HALT the
		program and show an error.
		side effect: possibly HALTs.
dup		duplicates the value on the top of the stack.
swap		swaps the top 2 values on the stack with each other.
over		duplicates the value at 1-depth on the stack.
jmp <ADDR/LBL>jumps to the specified address or label.
je,jn,jl,jg,	jumps to the specified address or label IF the condition is satisfied.
jle,jge	e = equal, n = not equal, l = less than, g = greater than. all others are 
<ADDR/LBL>	combinations of these.
```

Advanced instructions:

*These are part of the core language, but they are equivalent to some combinations of "normal" instructions. Therefore, you should only use these if it makes your program more readable or easier to understand. The optimizer will use these to replace equivalent constructs, so you can write either.*

Equivalent "normal" instructions are shown behind `<=>`.

```
dup2		duplicates the top two stack values, <=> `over over`.
jz, jnz 	jumps to the specified address or label, if the top stack value is (not)
<ADDR/LBL>	zero. <=> `push 0 je <ADDR/LABEL>` or with `jn`.
inc		increments the stack top value by 1. <=> `push 1 add`.
dec		decrements the stack top value by 1. <=> `push 1 sub`.
``` 

## Labels

Label syntax:
```nasm
:my_label
push 1
print
```

Each label starts with `:` followed by the identifier.

Jump to label syntax (`jmp` here is replacable by any other jump instruction (`j*`)):
```nasm
jmp :my_label
```

If the jump is conditional, for example with `je` (jump if equal), the following behavior will occur:

- If the condition is true, jump to the address.
- If the condition is false, go to the next instruction.

## Jumps to raw addresses

**DO NOT** jump to raw addresses. There is no reason to do so. If you do, turn off optimizations with `--dont-optimize`, as currently the optimizer may remove or change instructions at a raw address jump target (optimization happens before absolute addresses can be calculated).

You *may* jump without labels, by simply using addresses. Consider the following (complete) program:

```nasm
push 1
push 2
add
dup
print
jmp 1
```

This will run infinitely and simply keep printing values for a long time. Here, no label was used, and instead a raw jump to address 1 (the `push 2` instruction) was used. 
Each instruction is 1 (unit) in size, so calculating the address of jumps is trivial. However, its recommended to use labels, as these are resolved at compile-time and the 
resulting bytecode is *the same*.

# Bytecode

MCL is represented by a very simple bytecode.

An instruction looks as follows:

```
[ 00 0000000000000000 ]
  op      value
```

That is, one byte instruction, and 7 bytes value.
Each argument, e.g. the IMM of the `push` instruction, is thus at most a 56 bit integer.
The stack itself operates on 64 bit integers.
