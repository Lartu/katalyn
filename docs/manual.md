# 1 – Introduction

The Katalyn programming language is a powerful and lightweight procedural scripting language.

Katalyn is dynamically typed and runs by interpreting bytecode on the NariVM virtual machine. It has automatic and efficient memory management. It's ideal for rapid prototypes, scripts, configuration automation and language glueing.

The Katalyn programming language is free software, and is provided with no guarantees, as stated in its license. It's released under the Apache 2.0 License.

This document is dry. Feedback, corrections, contributions and suggestions are welcome, both on the main Katalyn repository (www.github.com/lartu/Katalyn) or by e-mail to lartu (at) lartu.net.

# 2 – The Language

The Katalyn programming language (just Katalyn from now on) has been designed to be extendable, but at the same time keep its syntax small and comprehensible. It's a language that you can learn during your lunch break and surely master in a week of use. It makes heavy use of implicitly declared variables that extended the functionality of what regular programming languages generally offer. In this regard, Katalyn is inspired by Perl. On the other hand, however, it's also designed to be easy to use, learn and read. In that regard, Katalyn borrows from the data-types of Lua.

The preferred file extension for Katalyn source files is '.kat', although the interpreter will not complain if any other extension is used.

# 3 – Basic Concepts
## 3.1 – Structure of a Katalyn Source File

Katalyn scripts are plain text files. Commands written in a Katalyn source file are executed from top to bottom. Functions can be defined anywhere in a file, but they must exist somewhere. Katalyn sourcefiles are compiled to Nambly code to be executed on the NariVM.

Statements in Katalyn are separated by a semicolon (';'). This means that you almost always have to write a semicolon at the end of every line.

Katalyn is a case-sensitive language.

## 3.2 – Datatypes

Katalyn is an operator typed language. In Katalyn, values do not have types, operations do. As such, values are treated differently depending on the operation. Generally, though, you'll be dealing with four data types: numbers, texts, tables and the nil value.

The number type represents real numbers. They are represented internally as binary64 double-precision floating-point numbers as defined by the IEEE 754.

The text type represents alphanumeric strings. In the interest of supporting as many locales as possible, strings in Katalyn are UTF-8 encoded and have no length bound.

The table type represents key-value collections. Table keys are always treated as strings. Table values can be of any type. This is a powerful structure that allows the user to create complex data types.

When assigned to a variable or passed to a function as a parameter, scalar values (numbers and texts) are passed by copy, while tables are always passed by reference.

## 3.3 – Variables

Variables in Katalyn must be prefixed with the glyph '$'. Variable names must consist of only latin letters (A-z), digits (0-9) and underscores.

Example variable names may be $foo and $bar.

While you are free to declare variables that start with an underscore, such as $_foo and $_bar, these variables should be reserved to be used by the language for implicit, context-sensitive variables.

## 3.4 – Scoping

Variables in Katalyn can exist in one of two scopes: the global scope or the local scope.

Variables defined outside functions will automatically be assigned to the global scope, and can be accessed from both the global scope (code outside the body of a function) and from within functions.

Variables defined within functions will automatically be assigned to the local scope (unless otherwise specified by the programmer). These variables cease to exist once the scope of the function is left and can only be accessed from the body of the function that defined them.

Variables must (generally) be defined before accessing them. Some variables are implicitly defined, and there are ways to tell the compiler that a variable that is not defined now will be defined by the time that part of the code is reached.

If, unless, while, until, and other code blocks do not define their own scopes. Variables defined within them will belong to the scope the block is in, and will continue to exist after leaving the block (but not the scope the block is in).

## 3.5 – Comments

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

# 9 – Working with Multiple Files

# 10 – Katalyn Compiler Flags