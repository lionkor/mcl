# MCL

"My concise language" is a language which is purely stack-based, and looks reminiscent of some assembly dialects.

You can find the **full language reference** here: [Reference.md](./Reference.md)

## Examples

Find more examples in the [examples](./examples) directory.

### Arithmetic

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
```
-> `84`
