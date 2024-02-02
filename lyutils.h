#ifndef LYUTILS_H
#define LYUTILS_H

#include "cpp_pipe.h"
#include "symbols.h"

#define YYSTYPE astree_ptr
#include "yyparse.h"

int yylex();
int yylex_destroy();

extern FILE* yyin;
extern char* yytext;
extern int yyleng;
extern int yy_flex_debug;

int yyparse(YYSTYPE astree_root);
void yyerror(astree_ptr, const char* message);
extern int yydebug;
const char* get_parser_yytname(int symbol);

class lex_util {
private:
    FILE* token_file = nullptr;
    location loc { .filenr = 0, .linenr = 1, .offset = 0 };
    size_t last_yyleng { 0 };

public:
    ~lex_util();
    bool open_tokens(const char* filename);
    bool close_tokens();
    void advance();
    void newline();
    void include();
    int token(int symbol);
    int badtoken(int symbol);
    void badchar(unsigned char bad);
    void lex_fatal_error(const char* msg);
    ostream& lex_error();
};

extern lex_util lexer;

class parse_util {
private:
    FILE* ast_file = nullptr;
    FILE* symbol_file = nullptr;
	FILE* ocil_file = nullptr;
    const char* fname = nullptr;
    cpp_pipe oc_file;
    astree_ptr astree_root = astree::make(TOK_ROOT, {}, "[ROOT]");

public:
	void emit();
	void emit(astree_ptr start);
    void traverse();
    void traverse(const string* function_name, astree_ptr start,
        bool scope, int block_number);
    void delete_symbols(astree_ptr start);
    void write_ast();
    void write_ast(astree_ptr start, int depth = 0);
    bool open_ast();
    bool close_ast();
    bool open_symbols();
    bool close_symbols();
	bool open_ocil();
	bool close_ocil();
    parse_util(const char* oc_filename, bool parse_debug,
        bool lex_debug, string defines);
    ~parse_util() { delete astree_root; }
    void parse();
    astree_ptr root() { return astree_root; }
};

#endif
