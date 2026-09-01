" Vim syntax file
" Language: LSPL
" Maintainer: Lartu
" Last Change: 2024-07-18

if exists("b:current_syntax")
  finish
endif

" Separator
" syntax match lsplPunctuation ":"
" highlight link lsplPunctuation Special

" Operators
syntax match lsplOperator "\*"
syntax match lsplOperator "\^"
syntax match lsplOperator "/"
syntax match lsplOperator "%"
syntax match lsplOperator "//"
syntax match lsplOperator "+"
syntax match lsplOperator "&"
syntax match lsplOperator "-"
syntax match lsplOperator "::"
syntax match lsplOperator "!"
syntax match lsplOperator "<"
syntax match lsplOperator ">"
syntax match lsplOperator "<="
syntax match lsplOperator ">="
syntax match lsplOperator "<>"
syntax match lsplOperator "!="
syntax match lsplOperator "="
syntax match lsplOperator "&&"
syntax match lsplOperator "||"
highlight link lsplOperator Special

" Floats
syntax match lsplFloat "\<\d\+\.\d\+\>"
highlight link lsplFloat Constant

" Integers
syntax match lsplInteger "\<\d\+\>"
highlight link lsplInteger Constant

" Keywords
syntax match lsplOtherKeyword "\<[A-Za-z0-9_]\+\>"
highlight link lsplOtherKeyword Statement

" Functions
syntax match lsplFunction "\<[A-Za-z0-9_]\+\>\s*\%#("
highlight link lsplFunction Statement

" Regular Variables
syntax match lsplVariable "\$[A-Za-z0-9_]*"
highlight link lsplVariable Identifier

" Reserved variables
syntax match lsplReservedVariable "\$_[A-Za-z0-9_]*"
highlight link lsplReservedVariable Underlined

" Punctuation
syntax match lsplPunctuation "("
syntax match lsplPunctuation ")"
syntax match lsplPunctuation ";"
syntax match lsplPunctuation "\["
syntax match lsplPunctuation "\]"
highlight link lsplPunctuation Normal

" Strings
syntax region lsplString start=+"+ end=+"+ contains=lsplEscape
highlight link lsplString Constant

" String access within {}
syntax region lsplStringAccess start="{" end="}" contains=lsplEscape
highlight link lsplStringAccess Constant

" Escape sequences within strings
syntax match lsplEscape "\\\\[\"ntr]"
highlight link lsplEscape Special

" Line comments
syntax match lsplLineComment "#.*$"
highlight link lsplLineComment Comment

" Block comments
syntax region lsplBlockComment start="(\*" end="\*)" contains=lsplBlockComment
highlight link lsplBlockComment Comment

let b:current_syntax = "lspl"