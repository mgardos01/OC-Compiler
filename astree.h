#ifndef ASTREE_H
#define ASTREE_H

#include <bitset>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

#include "auxlib.h"

enum attr {
    attr_VOID,
    attr_INT,
    attr_NULLPTR_T,
    attr_STRING,
    attr_STRUCT,
    attr_ARRAY,
    attr_FUNCTION,
    attr_VARIABLE,
    attr_FIELD,
    attr_PROTOTYPE,
    attr_PARAM,
    attr_LOCAL,
    attr_LVAL,
    attr_CONST,
    attr_VREG,
    attr_VADDR,
    attr_BITSET_SIZE
};

using attr_bitset = bitset<(attr_BITSET_SIZE)>;

using astree_ptr = struct astree*;

using symbol_ptr = struct symbol*;

struct location {
    size_t filenr = 0;
    size_t linenr = 0;
    size_t offset = 0;
};

ostream& operator<<(ostream&, const location&);

class astree {
private:
    astree(int symbol, const location&, const char* lexinfo);
    static unordered_set<string> lextable;

public:
    attr_bitset attributes;
    size_t block_number;
    size_t sequence;
    const string* struct_entry;
    
    string vreg;
	vector<string> vreg_statement;

    int symbol; // token code
    location loc; // source location
    const string* lexinfo; // pointer to lexical information
    vector<astree_ptr> children; // children of this n-way node
    static vector<string> filenames;

    astree(const astree&) = delete;
    astree& operator=(const astree&) = delete;
    ~astree();

    static void dump_filenames();
    static void print_symbol_value(astree_ptr);
    static astree_ptr make(
        int symbol, const location&, const char* lexinfo);
    static void erase(astree_ptr&);

    astree_ptr adopt(astree_ptr child1, astree_ptr child2 = nullptr,
        astree_ptr child3 = nullptr);
    astree_ptr adopt_as(astree_ptr child, int symbol);
    void dump_tree(int depth = 0);
    void write_tree(FILE* out, int depth = 0);
    void print_tree(ostream& out, int depth = 0);
};

ostream& operator<<(ostream&, const astree&);

#endif
