# General description of the assembly language

## Grammar

```
program = ((directive | label | instruction) comment? '\n' )*

comment = ';' .*

directive = '.' identifier argument*

instruction = identifier argument*

label = identifier ':'

argument = identifier | integer

```