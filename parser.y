%{
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#pragma GCC diagnostic ignored "-Wdeprecated"

#include "lyutils.h"
#include "astree.h"
#define ERASE(PTR) (astree::erase(PTR))

astree_ptr create_type_id(astree_ptr type_spec, astree_ptr ident, 
                          astree_ptr vardecl_expr = nullptr) {
   astree_ptr type_id = astree::make(TOK_TYPE_ID, type_spec->loc, "");
   type_id = type_id->adopt(type_spec, ident);
   if (vardecl_expr != nullptr) type_id = type_id->adopt(vardecl_expr);
   return type_id;
}

astree_ptr create_function(astree_ptr ftype, astree_ptr fident, 
                           astree_ptr fparam, astree_ptr fblock) {
   astree_ptr type_id = create_type_id(ftype, fident);
   astree_ptr function = astree::make(TOK_FUNCTION, ftype->loc, "");
   return function->adopt(type_id, fparam, fblock);
}
%}

%parse-param {YYSTYPE astree_root}
%debug
%defines
%define parse.error verbose
%token-table
%verbose

%destructor { if ($$ != astree_root) ERASE($$); } <>

%token TOK_VOID TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULLPTR TOK_ARRAY TOK_ARROW TOK_ALLOC TOK_PTR
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE TOK_NOT
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON
%token TOK_ROOT TOK_BLOCK TOK_CALL TOK_TYPE_ID 
%token TOK_FUNCTION TOK_INDEX TOK_PARAM

%right TOK_IF TOK_ELSE
%right "="
%left TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left "+" "-"
%left "*" "/" "%"
%right POS NEG NOT ELSE
%start start

%%

start         : program             { $$ = $1 = nullptr; }
              ;
    
program       : program structdef  { $$ = $1->adopt($2); }
              | program function   { $$ = $1->adopt($2); }
              | program statement  { $$ = $1->adopt($2); }
              | program error '}'  { ERASE($3); $$ = $1; }
              | program error ';'  { ERASE($3); $$ = $1; }
              | %empty             { $$ = astree_root; }
              ;
    
// PARAM NOTES #4
structdef     : structdefhead '}' ';'{ ERASE($2); ERASE($3); $$ = $1; }
              ;
structdefhead : structdefhead type TOK_IDENT ';' { ERASE($4); 
                                                   $$ = $1->adopt(
                                                      create_type_id(
                                                         $2, $3
                                                      )
                                                   );       
                                                 }
              | TOK_STRUCT TOK_IDENT '{'         { ERASE($3); 
                                                   $$ = $1->adopt($2);
                                                 }
              ;
// PARAM NOTES #4

type          : plaintype { $$ = $1; }
              | TOK_ARRAY TOK_LT plaintype TOK_GT { ERASE($2); 
                                                    ERASE($4); 
                                                    $$ = $1->adopt(
                                                       $3
                                                    );                 
                                                  }
              ;
    
plaintype     : TOK_INT { $$ = $1; }/* looks good */
              | TOK_STRING { $$ = $1; }
              | TOK_PTR TOK_LT TOK_STRUCT TOK_IDENT TOK_GT
                                                   { ERASE($2); 
                                                     ERASE($3); 
                                                     ERASE($5); 
                                                     $$ = $1->adopt(
                                                        $4
                                                     );                 
                                                   } 
                                                      
              ;

// PARAM NOTES #7
function      : plaintype TOK_IDENT parameters block 
                                                { $$ = create_function(
                                                     $1, $2, $3, $4
                                                  );                    
                                                } 
                                                      
              | TOK_VOID TOK_IDENT parameters block  
                                                { $$ = create_function(
                                                     $1, $2, $3, $4
                                                  );                    
                                                }
              ;

parameters    : paramhead ')' { ERASE($2); $$ = $1; }// $$ = $1
              | '(' ')'       { ERASE($2); $1->symbol = TOK_PARAM; 
                                $$ = $1;                           
                              }// $$ = $1
              ;
paramhead     : paramhead ',' type TOK_IDENT { 
                                               ERASE($2); 
                                               $$ = $1->adopt(
                                                  create_type_id(
                                                     $3,
                                                     $4
                                                  )
                                               ); 
                                             }
              | '(' type TOK_IDENT           { $1->symbol = TOK_PARAM; 
                                               $$ = $1->adopt(
                                                  create_type_id(
                                                     $2,$3
                                                  )
                                               ); 
                                             }
              ;
// PARAM NOTES #7

// PARAM NOTES #4
block         : blockhead '}'       { ERASE($2); $$ = $1; }
              | ';'                 { $$ = $1; }
              ;
blockhead     : blockhead statement { $$ = $1->adopt($2);}
              | '{'                 { $1->symbol = TOK_BLOCK; 
                                      $$ = $1; 
                                    }
              ;
// PARAM NOTES #4

statement     : vardecl     { $$ = $1; }
              | block       { $$ = $1; }
              | while       { $$ = $1; }
              | ifelse      { $$ = $1; }
              | return      { $$ = $1; }
              | expr ';'    { ERASE($2); $$ = $1; }
              ;

vardecl       : type TOK_IDENT ';'          { ERASE($3); 
                                              $$ = create_type_id(
                                                 $1, $2
                                              ); 
                                            }
              | type TOK_IDENT '=' expr ';' { ERASE($3); ERASE($5); 
                                              $$ = create_type_id(
                                                 $1, $2, $4
                                              ); 
                                            }
              ;
    
while         : TOK_WHILE '(' expr ')' statement { ERASE($2); 
                                                   ERASE($4); 
                                                   $$ = $1->adopt(
                                                      $3, $5
                                                   ); 
                                                 }
              ;
    
ifelse        :TOK_IF '(' expr ')' statement TOK_ELSE statement
                                                  {
                                                     ERASE($2);
                                                     ERASE($4);
                                                     ERASE($6);
                                                     $1->adopt($3, $5);
                                                     $1->adopt($7);
                                                     $$ = $1;
                                                  }
              | TOK_IF '(' expr ')' statement %prec TOK_ELSE
                                                  {
                                                     ERASE($2);
                                                     ERASE($4);
                                                     $$ = $1->adopt(
                                                        $3, $5
                                                     );
                                                  }
              ;
    
return        : TOK_RETURN ';'      { ERASE($2); $$ = $1; }
              | TOK_RETURN expr ';' { ERASE($3); $$ = $1->adopt($2); }
              ;
    
expr          : expr '=' expr          { $2->adopt($1, $3); $$ = $2; }
              | expr TOK_EQ expr       { $2->adopt($1, $3); $$ = $2; }
              | expr TOK_NE expr       { $2->adopt($1, $3); $$ = $2; }
              | expr TOK_LT expr       { $2->adopt($1, $3); $$ = $2; }
              | expr TOK_LE expr       { $2->adopt($1, $3); $$ = $2; }
              | expr TOK_GT expr       { $2->adopt($1, $3); $$ = $2; }
              | expr TOK_GE expr       { $2->adopt($1, $3); $$ = $2; }
              | expr '+' expr          { $2->adopt($1, $3); $$ = $2; }
              | expr '-' expr          { $2->adopt($1, $3); $$ = $2; }
              | expr '*' expr          { $2->adopt($1, $3); $$ = $2; }
              | expr '/' expr          { $2->adopt($1, $3); $$ = $2; }
              | expr '%' expr          { $2->adopt($1, $3); $$ = $2; }
              | '+' expr %prec POS     { $$ = $1->adopt_as($2, POS); }
              | '-' expr %prec NEG     { $$ = $1->adopt_as($2, NEG); }
              | TOK_NOT expr %prec NOT { $$ = $1->adopt_as($2, NOT); }
              | allocator              { $$ = $1;}
              | call                   { $$ = $1;}
              | '(' expr ')'           { ERASE($1); ERASE($3); 
                                         $$ = $2;
                                       }
              | variable               { $$ = $1; }
              | constant               { $$ = $1; }
              ;
    
allocator     : TOK_ALLOC TOK_LT TOK_STRING TOK_GT '(' expr ')' 
                                                    { 
                                                        ERASE($2); 
                                                        ERASE($4); 
                                                        ERASE($5); 
                                                        ERASE($7);
                                                        $$ = $1->adopt(
                                                           $3, $6
                                                        );
                                                    }
              | TOK_ALLOC TOK_LT TOK_STRUCT TOK_IDENT TOK_GT '(' ')' 
                                                    { 
                                                        ERASE($2); 
                                                        ERASE($3); 
                                                        ERASE($5); 
                                                        ERASE($6);
                                                        ERASE($7);
                                                        $$ = $1->adopt(
                                                           $4
                                                        );
                                                    }
              | TOK_ALLOC TOK_LT TOK_ARRAY TOK_LT 
                plaintype TOK_GT TOK_GT '(' expr ')'
                                                    {
                                                        ERASE($2);
                                                        ERASE($4);
                                                        ERASE($6);
                                                        ERASE($7);
                                                        ERASE($8);
                                                        ERASE($10);
                                                        $3->adopt($5);
                                                        $$ = $1->adopt(
                                                           $3, $9
                                                        );
                                                    }
              ;
    
// PARSER     NOTES #6
call          : callhead ')'        { ERASE($2); $$ = $1; }
              | TOK_IDENT '(' ')'   { ERASE($3); $2->symbol = TOK_CALL; 
                                      $$ = $2->adopt($1); 
                                    }
              ;
    
callhead      : callhead ',' expr   { ERASE($2); $$ = $1->adopt($3); }
              | TOK_IDENT '(' expr  { $2->symbol = TOK_CALL; 
                                      $$ = $2->adopt($1, $3); 
                                    }
              ;
// PARSER     NOTES #6
    
variable      : TOK_IDENT { $$ = $1; }
              | expr '[' expr ']'{ ERASE($4); $2->symbol = TOK_INDEX; 
                                   $$ = $2->adopt($1, $3); 
                                 }
              | expr TOK_ARROW TOK_IDENT { $$ = $2->adopt($1, $3); }
              ;
    
constant      : TOK_INTCON    { $$ = $1; }
              | TOK_CHARCON   { $$ = $1; }
              | TOK_STRINGCON { $$ = $1; }
              | TOK_NULLPTR   { $$ = $1; }
              ; 
            
%%

const char* get_parser_yytname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}
