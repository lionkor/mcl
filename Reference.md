# MCL Reference

# Language

## "Registers"

- `pc`: Program counter
- `st`: Stack top (one above the top stack element)

## Instructions

Quick guide to reading the instructions:

- `SOMETHING` = instruction, usually typed lowercase
- `<IMM>` = immediate value, like `42`
- `<ADDR>` = address
- `<LBL>` = label (for example `:something`)
- `++abc`, `abc+2`, ... = increments register `abc`
- `--abc`, `abc-2`, ... = decrements register `abc`

All instructions:

```
push <IMM>	pushes value on the stack.
		++pc
		++st
pop		pops the top value from the stack.
		++pc
		--st
print		prints the top value of the stack and pops it.
		side effect: prints the value to stdout, followed by a 
		newline.
		++pc
		--st
add, sub, mul	adds/subtracts/multiplies the two top values on the stack, 
		and pops them off the stack. pushes the result of the operation
		on the stack.
		++pc
		--st
div, mod	divides or performs modulo on the two top values on the stack, 
		and pops them off the stack. pushes the result of the operation
		on the stack. if the second argument is zero, it will HALT the
		program and show an error.
		side effect: possibly HALTs.
		++pc
		--st
dup		duplicates the value on the top of the stack.
		++pc
		++st
swap		swaps the top 2 values on the stack with each other.
		++pc
over		duplicates the value at 1-depth on the stack.
		++pc
		++st
jmp <ADDR/LBL>	jumps to the specified address or label.
		pc = <ADDR/LBL>
je,jn,jl,jg,	jumps to the specified address or label IF the condition is satisfied.
jle,jge		e = equal, n = not equal, l = less than, g = greater than. all others are 
<ADDR/LBL>	combinations of these.
		pc=<ADDR/LBL>  OR  pc+1
		st-2
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

You *may* jump without labels, by simply using addresses. Consider the following (complete) program:

```nasm
push 1
push 2
add
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
