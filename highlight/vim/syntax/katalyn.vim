" Vim syntax file
" Language: Katalyn
" Maintainer: Lartu
" Last Change: 2024-07-18

if exists("b:current_syntax")
  finish
endif

" Separator
" syntax match katalynPunctuation ":"
" highlight link katalynPunctuation Special

" Operators
syntax match katalynOperator "\*"
syntax match katalynOperator "\^"
syntax match katalynOperator "/"
syntax match katalynOperator "%"
syntax match katalynOperator "//"
syntax match katalynOperator "+"
syntax match katalynOperator "&"
syntax match katalynOperator "-"
syntax match katalynOperator "::"
syntax match katalynOperator "!"
syntax match katalynOperator "<"
syntax match katalynOperator ">"
syntax match katalynOperator "<="
syntax match katalynOperator ">="
syntax match katalynOperator "<>"
syntax match katalynOperator "!="
syntax match katalynOperator "="
syntax match katalynOperator "&&"
syntax match katalynOperator "||"
highlight link katalynOperator Special

" Floats
syntax match katalynFloat "\<\d\+\.\d\+\>"
highlight link katalynFloat Constant

" Integers
syntax match katalynInteger "\<\d\+\>"
highlight link katalynInteger Constant

" Keywords
syntax match katalynOtherKeyword "\<[A-Za-z0-9_]\+\>"
highlight link katalynOtherKeyword Statement

" Functions
syntax match katalynFunction "\<[A-Za-z0-9_]\+\>\s*\%#("
highlight link katalynFunction Statement

" Regular Variables
syntax match katalynVariable "\$[A-Za-z0-9_]*"
highlight link katalynVariable Identifier

" Reserved variables
syntax match katalynReservedVariable "\$_[A-Za-z0-9_]*"
highlight link katalynReservedVariable Underlined

" Punctuation
syntax match katalynPunctuation "("
syntax match katalynPunctuation ")"
syntax match katalynPunctuation ";"
syntax match katalynPunctuation "\["
syntax match katalynPunctuation "\]"
highlight link katalynPunctuation Normal

" Strings
syntax region katalynString start=+"+ end=+"+ contains=katalynEscape
highlight link katalynString Constant

" String access within {}
syntax region katalynStringAccess start="{" end="}" contains=katalynEscape
highlight link katalynStringAccess Constant

" Escape sequences within strings
syntax match katalynEscape "\\\\[\"ntr]"
highlight link katalynEscape Special

" Line comments
syntax match katalynLineComment "#.*$"
highlight link katalynLineComment Comment

" Block comments
syntax region katalynBlockComment start="(\*" end="\*)" contains=katalynBlockComment
highlight link katalynBlockComment Comment

let b:current_syntax = "katalyn"