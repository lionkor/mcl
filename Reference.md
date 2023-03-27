# MCL Reference

# Language

## "Registers"

- `pc`: Program counter
- `st`: Stack top (one above the top stack element)

## Instructions

Quick guide to reading the instructions:

- `SOMETHING` = instruction, usually typed lowercase
- `<IMM>` = immediate value, like `42`
- `<ADDR>` = address (unused)
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
```

# Bytecode

MCL is represented by a very simple bytecode.

An instruction looks as follows:

```
[ 00 0000000000000000 ]
  op      value
```

That is, one byte instruction, and 7 bytes value.
Each argument, e.g. the IMM of the `push` instruction, is thus at most a 56 bit integer.
