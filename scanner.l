/* $Id: scanner.l,v 1.3 2021-12-08 12:23:51-08 - - $ */

%{
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "lyutils.h"

#define YY_USER_ACTION  { lexer.advance(); }
#define YY_FATAL_ERROR(msg) { lexer.lex_fatal_error (msg); }
%}

%option 8bit
%option debug
%option nobackup
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn

WORD                    [[:alpha:][:digit:]_]

IDENT                   ([[:alpha:]_]({WORD})*)
DECINTEGER              ([1-9][[:digit:]]*)
OCTINTEGER              (0[0-7]*) 
HEXINTEGER              (0[xX][[:xdigit:]]+)
INTEGER                 {DECINTEGER}|{OCTINTEGER}|{HEXINTEGER}
CHARCON                 ('([^\\'\n]|\\[\\'"0nt])')
STRINGCON               (\"([^\\"\n]|\\[\\'"0nt])*\")

IDENT_INVALID           ([[:digit:]]({WORD})+)

/* INVALID ESCAPE CHARCON
'\' or '\{x}' where {x} is an invalid character */
IEC_INVALID  ('\\[^\\'"0nt]?')

/* OPEN CHARCON
' followed by multiple characters that aren't 
the ' or a newline, followed by a newline. */
OC_INVALID   ('[^'\n]*\n)

/* TOO LONG CHARCON
' followed by multiple matching 
(valid or not!) characters, followed by a ' */
TLC_INVALID   ('([^\\'\n]|\\[\\'"0nt]|\\[^\\'"0nt]){2,}')

CHARCON_INVALID {IEC_INVALID}|{OC_INVALID}|{TLC_INVALID}

/* BAD ESCAPE STRINGCON
string with an invalid escaped character somewhere */
BES_INVALID (\"([^\\"\n])*(\\[^\\'"0nt]?)+([^\\"\n])*\")

/* OPEN STRINGCON
" followed by multiple characters that aren't the " 
or a newline, followed by a newline. */
OS_INVALID           (\"[^\"\n]*\n)

STRINGCON_INVALID       {BES_INVALID}|{OS_INVALID}

%%

^"#".*\n                { lexer.include(); lexer.newline(); }
[ \t]+                  { }
\n                      { lexer.newline(); }

"+"                     {return lexer.token('+'); }
"-"                     {return lexer.token('-'); }
"*"                     {return lexer.token('*'); }        
"/"                     {return lexer.token('/'); }
"%"                     {return lexer.token('%'); }
"="                     {return lexer.token('='); }
","                     {return lexer.token(','); }
";"                     {return lexer.token(';'); }
"("                     {return lexer.token('('); }
")"                     {return lexer.token(')'); }
"["                     {return lexer.token('['); }
"]"                     {return lexer.token(']'); }
"{"                     {return lexer.token('{'); }
"}"                     {return lexer.token('}'); }

"=="                    {return lexer.token(TOK_EQ); }
"!="                    {return lexer.token(TOK_NE); }
"<"                     {return lexer.token(TOK_LT); }
"<="                    {return lexer.token(TOK_LE); }
">"                     {return lexer.token(TOK_GT); }
">="                    {return lexer.token(TOK_GE); }
"->"                    {return lexer.token(TOK_ARROW); }

"if"                    {return lexer.token(TOK_IF); }
"else"                  {return lexer.token(TOK_ELSE); }
"while"                 {return lexer.token(TOK_WHILE); }
"return"                {return lexer.token(TOK_RETURN); }
"alloc"                 {return lexer.token(TOK_ALLOC); }
"nullptr"               {return lexer.token(TOK_NULLPTR); }
"not"                   {return lexer.token(TOK_NOT); }
"int"                   {return lexer.token(TOK_INT); }
"string"                {return lexer.token(TOK_STRING); }
"struct"                {return lexer.token(TOK_STRUCT); }
"array"                 {return lexer.token(TOK_ARRAY); }
"ptr"                   {return lexer.token(TOK_PTR); }
"void"                  {return lexer.token(TOK_VOID); }

{IDENT}                 {return lexer.token(TOK_IDENT); }
{INTEGER}               {return lexer.token(TOK_INTCON); }
{CHARCON}               {return lexer.token(TOK_CHARCON); }
{STRINGCON}             {return lexer.token(TOK_STRINGCON); }

{IDENT_INVALID}         {return lexer.badtoken(TOK_IDENT); }
{CHARCON_INVALID}       {return lexer.badtoken(TOK_CHARCON); }
{STRINGCON_INVALID}     {return lexer.badtoken(TOK_STRINGCON); }
.                       { lexer.badchar(*yytext); }

%%

