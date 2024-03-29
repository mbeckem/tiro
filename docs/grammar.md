# Tiro Grammar

## Notation

Unless otherwise noted, elements may be separated by arbitrary whitespace.

-   Quoted strings (`"x"` or `'x'`) represent terminal values as literal strings.
-   _ItalicNames_ represent named grammar rules.
-   "r1 r2" means that the two rules must match in sequence.
-   "r1 | r2" means that the input must match r1 or r2.
-   "r1 - r2" means that the input must match r1 but not r2.
-   "~r1" is the negation of r1
-   "r1"<sup>\*</sup> means that the rule may be repeated any number of times.
-   "r1"<sup>+</sup> means that the rule must match at least once.
-   "r1"<sup>?</sup> means that the rule must match 0 or 1 time.

## Top level items

TODO Syntax of Comment (`//` and `/** */`)

> _Item_ := `"export"`<sup>?</sup> (_ImportDecl_ `";"` | _VarDecl_ `";"` | _FuncDecl_)

> _ImportDecl_ := `"import"` _ImportPath_ (`"as"` _Identifier_)<sup>?</sup>  
> _ImportPath_ := _Identifier_ (`"."` _Identifier_ )<sup>\*</sup>

> _VarDecl_ := (`"const"` | `"var"`) _Binding_ (`","` _Binding_)<sup>\*</sup>  
> _Binding_: := _BindingPattern_ (`"="` _Expr_)<sup>?</sup>  
> _BindingPattern_ := _Identifier_ | `"("` _Identifier_ (`","` _Identifier_)<sup>\*</sup> `")"`

> _FuncDecl_ := `"func"` _Identifier_<sup>?</sup> `"("` _ParamList_<sup>?</sup> `")"` _FuncBody_  
> _ParamList_ := _Identifier_ (`","` _Identifier_)<sup>\*</sup>  
> _FuncBody_ := (`"="` _Expr_) | _BlockExpr_

## Statements

> _Stmt_ :=  
> &nbsp;&nbsp;&nbsp;&nbsp; _AssertStmt_ `";"` | _VarDecl_ `";"`  
> &nbsp;&nbsp;&nbsp;&nbsp; | _ForEachStmt_ | _ForStmt_ | _WhileStmt_  
> &nbsp;&nbsp;&nbsp;&nbsp; | _IfExpr_ | _DeferStmt_ `";"`  
> &nbsp;&nbsp;&nbsp;&nbsp; | _BlockExpr_ | _Expr_ `";"`  
> &nbsp;&nbsp;&nbsp;&nbsp; | `";"`

> _AssertStmt_ := `"assert"` `"("` _Expr_ (`","` _StringExpr_)<sup>?</sup> `")"`

> _ForEachStmt_ := `"for"` _BindingPattern_ `"in"` _Expr_ _BlockExpr_

> _ForStmt_ := `"for"` _VarDecl_<sup>?</sup> `";"` _Expr_<sup>?</sup> `";"` _Expr_<sup>?</sup> _BlockExpr_

> _WhileStmt_ := `"while"` _Expr_ _BlockExpr_

> _DeferStmt_ := `"defer"` _Expr_

## Expressions

> _Expr_ :=  
> &nbsp;&nbsp;&nbsp;&nbsp; _Literal_ | _FieldExpr_ | _TupleFieldExpr_ | _ElementExpr_  
> &nbsp;&nbsp;&nbsp;&nbsp; | _CallExpr_ | _UnaryExpr_ | _BinaryExpr_  
> &nbsp;&nbsp;&nbsp;&nbsp; | _AssignExpr_ | _ContinueExpr_ | _BreakExpr_  
> &nbsp;&nbsp;&nbsp;&nbsp; | _ReturnExpr_ | _GroupedExpr_ | _IfExpr_  
> &nbsp;&nbsp;&nbsp;&nbsp; | _FuncExpr_ | _BlockExpr_

> _FieldExpr_ := _Expr_ (`"."`| `"?."`) _Identifier_

> _TupleFieldExpr_ := _Expr_ (`"."` | `"?."`) _NonNegativeInt_

> _ElementExpr_ := _Expr_ (`"["` | `"?["`) _Expr_ `"]"`

> _CallExpr_ := _Expr_ (`"("` | `"?("`) _CallArguments_<sup>?</sup> `")"`  
> _CallArguments_ := _Expr_ (`","` _Expr_)<sup>\*</sup>

> _UnaryExpr_ := _UnaryOp_ _Expr_  
> _UnaryOp_ := `"+"` | `"-"` | `"!"` `"~"`

> _BinaryExpr_ := _Expr_ _BinaryOp_ _Expr_  
> _BinaryOp_ :=  
> &nbsp;&nbsp;&nbsp;&nbsp; `"+"` | `"-"` | `"*"` | `"/"` | `"%"`  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"**"` | `"<<"` | `">>"` | `"&"` | `"|"` | `"^"`  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"<"` | `">"` | `"<="` | `">="` | `"=="` | `"!="`  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"??"` | `"&&"` | `"||"`

> _AssignExpr_ := _Expr_ _AssignOp_ _Expr_  
> _AssignOp_ := `"="` | `"+="` | `"-="` | `"*="` | `"**="` | `"/="` | `"%="`

> _ContinueExpr_ := `"continue"`

> _BreakExpr_ := `"break"`

> _ReturnExpr_ := `"return"` _Expr_<sup>?</sup>

> _GroupedExpr_ := `"("` _Expr_ `")"`

> _IfExpr_ := `"if"` _Expr_ _BlockExpr_ (`"else"` _BlockExpr_)<sup>?</sup>

> _FuncExpr_ := _FuncDecl_

> _BlockExpr_ := `"{"` _Stmt_<sup>\*</sup> `"}"`

## Literals

> _Literal_ :=  
> &nbsp;&nbsp;&nbsp;&nbsp; `"true"` | `"false"` | `"null"`  
> &nbsp;&nbsp;&nbsp;&nbsp; | _Int_ | _Float_ | _String_  
> &nbsp;&nbsp;&nbsp;&nbsp; | _Symbol_ | _Identifier_ | _TupleLiteral_ | _RecordLiteral_  
> &nbsp;&nbsp;&nbsp;&nbsp; | _ArrayLiteral_ | _MapLiteral_ | _SetLiteral_

> _TupleLiteral_ := `"("` _TupleElements_<sup>?</sup> `")"`  
> _TupleElements_ := (_Expr_ `","`)<sup>+</sup> _Expr_<sup>?</sup>

> _RecordLiteral_ := `"("` (_RecordElements_ | `":"` ) `")"`  
> _RecordElements_ := _RecordEntry_ (`","` _RecordEntry_)<sup>\*</sup> `","`<sup>?</sup>  
> _RecordEntry_ := _Identifier_ `":"` _Expr_

> _ArrayLiteral_ := `"["` _ArrayElements_<sup>?</sup> `"]"`  
> _ArrayElements_ := _Expr_ (`","` _Expr_)<sup>\*</sup> `","`<sup>?</sup>

> _MapLiteral_ := `"map{"` _MapElements_ <sup>?</sup> `"}"`  
> _MapElements_ := _MapEntry_ (`","` _MapEntry_)<sup>\*</sup> `","`<sup>?</sup>  
> _MapEntry_ := _Expr_ `":"` _Expr_

> _SetLiteral_ := `"set{"` _SetElements_ <sup>?</sup> `"}"`  
> _SetElements_ := _Expr_ (`","` _Expr_)<sup>\*</sup> `","`<sup>?</sup>

## Atoms

Elements within this section _must not_ be separated by white space.

> _Int_ :=  
> &nbsp;&nbsp;&nbsp;&nbsp; _DecimalDigit_<sup>+</sup>  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"0b"` _BinaryDigit_<sup>\*</sup>  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"0o"` _OctalDigit_<sup>\*</sup>  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"0x"` _HexadecimalDigit_<sup>\*</sup>  
> _DecimalDigit_ := [`"0"` - `"9"`] | `"_"`  
> _BinaryDigit_ := `"0"` | `"1"` | `"_"`  
> _OctalDigit_ := [`"0"` - `"7"`] | `"_"`  
> _HexadecimalDigit_ := [`"0"` - `"9"`] &nbsp; | &nbsp; [`"a"` - `"f"`] &nbsp; | &nbsp; [`"A"` - `"F"`] &nbsp; | &nbsp; `"_"`

> _NonNegativeInt_ := `"0"` &nbsp; | &nbsp; [`"1"` - `"9"`] &nbsp; [`"0"` - `"9"`]<sup>\*</sup>

> _Float_ :=  
> &nbsp;&nbsp;&nbsp;&nbsp; _DecimalDigit_<sup>+</sup> `"."` _DecimalDigit_<sup>\*</sup>  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"0b"` _BinaryDigit_<sup>\*</sup> `"."` _BinaryDigit_<sup>\*</sup>  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"0o"` _OctalDigit_<sup>\*</sup> `"."` _OctalDigit_<sup>\*</sup>  
> &nbsp;&nbsp;&nbsp;&nbsp; | `"0x"` _HexadecimalDigit_<sup>\*</sup> `"."` _HexadecimalDigit_<sup>\*</sup>

> _Identifier_ := (_UnicodeLetter_ | `"_"`) (_UnicodeLetter_ | _UnicodeNumber_ | `"_"`)<sup>\*</sup>

> _Symbol_ := `"#"` _Identifier_

TODO: Document string escape rules.  
TODO: Document format mini language. E.g. `"hello ${name}"` or `"hello $name"`.

> _String_ := `'"'` (_StringContent_ - `'"'`) `'"'` | `"'"` (_StringContent_ - `"'"`) `"'"`

## Comments

> _LineComment_ := `"//"` ~(`"\n"`)<sup>\*</sup> (`"\n"` | EOF)

> _BlockComment_ := `"/*"` (_BlockComment_ | ~`"*/"`)_ `"_/"`
