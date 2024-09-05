# 1 – Introduction

Katalyn is a powerful and lightweight procedural scripting language.

It’s dynamically typed and runs by interpreting bytecode on the NariVM virtual machine. With automatic and efficient memory management, Katalyn is ideal for rapid prototyping, scripting, configuration automation, and language integration.

Katalyn is free software provided without any guarantees, as detailed in its license. It’s released under the Apache 2.0 License.

This document is intentionally concise. Feedback, corrections, contributions, and suggestions are welcome either through the main Katalyn repository (www.github.com/lartu/Katalyn) or via email to lartu (at) lartu.net.

# 2 – The Language

Katalyn is designed to be extendable while keeping its syntax small and easy to understand. It’s a language you can pick up during your lunch break and master within a week of use. Katalyn makes extensive use of implicitly declared variables, enhancing functionality beyond what most traditional programming languages offer. In this sense, Katalyn takes inspiration from Perl. However, it’s also designed to be simple to use, learn, and read; drawing its data types from Lua.

The preferred file extension for Katalyn source files is .kat, though the interpreter won’t complain if you use another extension.

# 3 – Basic Concepts
## 3.1 – Structure of a Katalyn Source File

Katalyn scripts are plain text files, and commands are executed sequentially, from top to bottom. Functions can be defined anywhere in the file, as long as they are present before being called. Katalyn source files are compiled into Nambly code, which is then executed on the NariVM.

Statements in Katalyn are separated by semicolons (`;`), so you’ll almost always need to end each line with one.

Katalyn is a case-sensitive language.

## 3.2 – Datatypes

Katalyn is an operator-typed language, meaning values themselves don’t have types, but the operations do. This means values are handled differently depending on the operation. That said, you’ll typically work with four primary data types: numbers, texts, tables, and the nil value.

Number: Represents real numbers, stored internally as binary64 double-precision floating-point numbers, as defined by the IEEE 754 standard.

Text: Represents alphanumeric strings. To support a wide range of locales, strings in Katalyn are UTF-8 encoded and have no fixed length limit.

Table: Represents key-value collections. Table keys are always treated as strings, while table values can be of any type. This flexible structure allows users to create complex data types.

Nil: The nil value represents the absence of a value, or a “non-value.” It always evaluates to false in boolean expressions. You can store nil in variables, and in fact, the default value of an unset table position is nil. However, attempting to operate on nil or pass it as a parameter to any function (except `is(...)`) will result in an error. This behavior is designed to prevent nil replication errors.

When assigned to a variable, a table position, or passed to a function, scalar values (numbers, texts and `nil`) are passed by copy, while tables are always passed by reference.

## 3.3 – Variables

Variables in Katalyn must be prefixed with the '`$`' symbol. Variable names can only contain Latin letters (A-Z, a-z), digits (0-9), and underscores (_).

For example, valid variable names include `$foo` and `$bar`.

Although you are free to declare variables that start with an underscore, such as `$_foo` and `$_bar`, it’s recommended to reserve these for implicit, context-sensitive variables used by the language.

## 3.4 – Scoping

Variables in Katalyn can exist in one of two scopes: global or local.

Variables defined outside functions are automatically assigned to the global scope and can be accessed both from outside functions (global scope) and from within them.

Variables defined within functions are automatically assigned to the local function scope, unless otherwise specified. These variables only exist while inside the function that defines them and are inaccessible once the function’s scope is exited.

In general, variables must be defined before being accessed (that is, assigned a value before being read). Some variables, however, are implicitly defined, and there are ways to instruct the compiler that a variable, though not currently defined, will be by the time that part of the code is reached (i.e. `unsafe(...)`).

Control structures like `if`, `unless`, `while`, and `until` do not create their own scopes. Variables defined within these blocks belong to the scope the block is in and will continue to exist after the block is exited, as long as the scope is still active.

## 3.5 – Comments

Comments are used to annotate the code, making it easier to understand for anyone reading it. They do not affect the execution of the program.

Katalyn supports two types of comments: line comments and multiline comments.

Line comments: These are created by placing a `#` symbol at the beginning of the line. Everything after the `#` on that line is ignored by the compiler.

Example:

```
# This is a line comment
```

Multiline comments: These are enclosed between `(*` and `*)`. Anything between these symbols is ignored, allowing for comments that span multiple lines.

Example:

```
(* This is a
multiline comment *)
```

Multiline comments in Katalyn can be nested, meaning you can place one multiline comment inside another.

# 4 – Variable Declaration and Assignment

## 4.1 – Explicit Variable Scoping

## 4.2 – Unsafe Variables

## 4.3 – Magic Variables

## 4.4 – Undeclaring Variables

# 5 – Expressions

## 5.1 – Operators

## 5.2 – Function Calls

# 6 – Function Definition and Calling

## 6.1 – Function Shadowing

## 6.2 – Function Magic Variables

## 6.3 – Built-in Functions

# 7 – Flow Control Structures

## 7.1 – if / elif / else

## 7.2 – unless / else

## 7.3 – while

## 7.4 – until

## 7.5 – for

# 8 – Tables

## 8.1 – Table Functions

## 8.2 – {} Indexes

## 8.3 – Arrays

## 8.4 – Table Prototyping

# 9 – Working with Multiple Files

# 10 – Katalyn Compiler Flags