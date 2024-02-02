#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

using namespace std;

#include "auxlib.h"
#include "lyutils.h"

lex_util lexer;

void yyerror(astree_ptr, const char* message)
{
    lexer.lex_error() << ": " << message << endl;
}

lex_util::~lex_util() { yylex_destroy(); }

bool lex_util::open_tokens(const char* filename)
{
    filesystem::path p(filename);
    if (p.extension().string() == "") {
        cerr << "ERROR in open_tokens: No extension.\n";
        return false;
    }
    if (p.extension().string() != ".oc") {
        cerr << "ERROR in open_tokens: Extension is not .oc\n";
        return false;
    }
    string fname = p.stem().string() + ".tokens";
    token_file = fopen(fname.c_str(), "w");
    if (!(token_file)) {
        cerr << "ERROR in open_tokens: Unable to open token file.\n";
    }
    return token_file ? true : false;
}

bool lex_util::close_tokens()
{
    if (token_file != nullptr) {
        int status = fclose(token_file);
        if (status != 0) {
            cerr << "ERROR in close_tokens: Unable to close token "
                    "file.\n";
        }
        return status == 0;
    }
    cerr << "ERROR in close_tokens: calling close on a null token "
            "file.\n";
    return false;
}

void lex_util::advance()
{
    if (loc.offset == 0) {
        // cout << ";" << setw(3) << loc.filenr << ":"
        //      << setw(3) << loc.linenr << ": ";
    }
    // cout << yytext;
    loc.offset += last_yyleng; // messes with offset of .h files
    last_yyleng = yyleng;
}

void lex_util::newline()
{
    ++loc.linenr;
    loc.offset = 0;
    last_yyleng = 1;
}

void lex_util::include()
{
    size_t linenr;
    size_t filename_len = strlen(yytext) + 1;
    unique_ptr<char[]> filename = make_unique<char[]>(filename_len);
    int scan_rc
        = sscanf(yytext, "# %zu \"%[^\"]\"", &linenr, filename.get());
    if (scan_rc != 2) {
        lex_error() << "invalid directive, ignored: " << yytext << endl;
    } else {
        if (yy_flex_debug) {
            cerr << "--included # " << linenr << " \"" << filename.get()
                 << "\"" << endl;
        }
        loc.linenr = linenr - 1;
        loc.filenr = astree::filenames.size();
        astree::filenames.push_back(filename.get());
        fprintf(token_file, "# %zu \"%s\"\n", linenr, filename.get());
    }
}

int lex_util::token(int symbol)
{
    yylval = astree::make(symbol, loc, yytext);
    fprintf(token_file, "%zu %2zu.%03zu %-3d %-13s %s\n",
        yylval->loc.filenr, yylval->loc.linenr, yylval->loc.offset,
        yylval->symbol, get_parser_yytname(yylval->symbol),
        (*(yylval->lexinfo)).c_str());
    // We clean up the node afterwards because
    // we just care about generating the .tokens file
    // astree::erase(yylval); NOT ANYMORE WE WANNA KEEP IT
    return symbol;
}

int lex_util::badtoken(int symbol)
{
    lex_error() << ": invalid token (" << yytext << ")" << endl;
    return token(symbol);
}

void lex_util::badchar(unsigned char bad)
{
    ostringstream buffer;
    if (isgraph(bad))
        buffer << bad;
    else
        buffer << "\\" << setfill('0') << setw(3) << unsigned(bad);
    lex_error() << ": invalid source character (" << buffer.str() << ")"
                << endl;
}

void lex_util::lex_fatal_error(const char* message)
{
    throw ::fatal_error(message);
}

ostream& lex_util::lex_error() { return exec::error() << loc << ": "; }

parse_util::parse_util(const char* oc_filename, bool parse_debug,
    bool lex_debug, string defines)
    : oc_file(oc_filename, lex_debug, defines)
{
    fname = oc_filename;
    yydebug = parse_debug;
    yy_flex_debug = lex_debug;
    yyin = oc_file.get_pipe();
}

void parse_util::parse()
{
    int not_ok = yyparse(astree_root);
    if (not_ok)
        exec::error() << "parse failed.";
    if (yydebug or yy_flex_debug) {
        astree_root->dump_tree();
        astree::dump_filenames();
    }
    open_symbols();
    traverse();
    close_symbols();

    open_ast();
    write_ast();
    close_ast();

    open_ocil();
    emit();
    close_ocil();
}

bool parse_util::open_ast()
{
    filesystem::path fpath(fname);
    if (fpath.extension().string() == "") {
        cerr << "ERROR in open_ast: No extension.\n";
        return false;
    }
    if (fpath.extension().string() != ".oc") {
        cerr << "ERROR in open_ast: Extension is not .oc\n";
        return false;
    }
    string ast_fname = fpath.stem().string() + ".ast";
    ast_file = fopen(ast_fname.c_str(), "w");
    if (!(ast_file)) {
        cerr << "ERROR in open_ast: Unable to open AST file.\n";
    }
    return ast_file ? true : false;
}

bool parse_util::close_ast()
{
    if (ast_file != nullptr) {
        int status = fclose(ast_file);
        if (status != 0) {
            cerr << "ERROR in close_ast: Unable to close AST file.\n";
        }
        return status == 0;
    }
    cerr << "ERROR in close_ast: calling close on a null AST file.\n";
    return false;
}

bool parse_util::open_symbols()
{
    filesystem::path fpath(fname);
    if (fpath.extension().string() == "") {
        cerr << "ERROR in open_symbols: No extension.\n";
        return false;
    }
    if (fpath.extension().string() != ".oc") {
        cerr << "ERROR in open_symbols: Extension is not .oc\n";
        return false;
    }
    string symbols_fname = fpath.stem().string() + ".symbols";
    symbol_file = fopen(symbols_fname.c_str(), "w");
    if (!(symbol_file)) {
        cerr
            << "ERROR in open_symbols: Unable to open .symbols file.\n";
    }
    return symbol_file ? true : false;
}

bool parse_util::close_symbols()
{
    if (symbol_file != nullptr) {
        int status = fclose(symbol_file);
        if (status != 0) {
            cerr << "ERROR in close_symbols: Unable to close .symbols "
                    "file.\n";
        }
        return status == 0;
    }
    cerr << "ERROR in close_symbols: calling close on a null .symbols "
            "file.\n";
    return false;
}

bool parse_util::open_ocil()
{
    filesystem::path fpath(fname);
    if (fpath.extension().string() == "") {
        cerr << "ERROR in open_ocil: No extension.\n";
        return false;
    }
    if (fpath.extension().string() != ".oc") {
        cerr << "ERROR in open_ocil: Extension is not .oc\n";
        return false;
    }
    string ocil_fname = fpath.stem().string() + ".ocil";
    ocil_file = fopen(ocil_fname.c_str(), "w");
    if (!(ocil_file)) {
        cerr
            << "ERROR in open_ocil: Unable to open .ocil file.\n";
    }
    return ocil_file ? true : false;
}

bool parse_util::close_ocil()
{
    if (ocil_file != nullptr) {
        int status = fclose(ocil_file);
        if (status != 0) {
            cerr << "ERROR in close_ocil: Unable to close .ocil "
                    "file.\n";
        }
        return status == 0;
    }
    cerr << "ERROR in close_ocil: calling close on a null .ocil "
            "file.\n";
    return false;
}
