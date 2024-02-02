#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <bitset>
#include <unordered_map>
#include <vector>

#include "astree.h"

struct symbol;

using symbol_ptr = struct symbol*;
using symbol_table = unordered_map<const string*, symbol_ptr>;
using symbol_table_ptr = symbol_table*;

extern unordered_map<string, vector<astree_ptr>> local_emits;

struct symbol {
    attr_bitset attributes;
    size_t sequence;
    symbol_table* fields;
    const string* struct_entry;
    location lloc;
    size_t block_nr;
    vector<symbol_ptr>* parameters;
    symbol(astree_ptr n) {
        attributes = n->attributes;
        sequence = n->sequence;
        fields = nullptr;
        struct_entry = n->struct_entry;
        block_nr = n->block_number;
        parameters = nullptr;
    }
};

void delete_symbol_node(symbol_ptr s);
void delete_symbol_table(symbol_table st);
void write_symbol(FILE* symbol_file, astree_ptr type_id_name);
void write_line(FILE* symbol_file);
void assign_on_type(astree_ptr type_id_node);
void assign_on_type_struct(
    astree_ptr type_id_name, astree_ptr struct_name);
void type_check_type_id(
    astree_ptr node, bool is_local, FILE* symbol_file, const string* function_name);
void type_check_assign(astree_ptr node);
void type_check_return(astree_ptr node, const string* function_name);
void type_check_eq(astree_ptr node);
void type_check_binop(astree_ptr node);
void type_check_unop(astree_ptr node);
void type_check_alloc(astree_ptr node);
void type_check_call(astree_ptr node);
void type_check_ident(astree_ptr node);
void type_check_index(astree_ptr node);
void type_check_arrow(astree_ptr node);
void type_check_intcon(astree_ptr node);
void type_check_stringcon(astree_ptr node);
void type_check_nullptr(astree_ptr node);
void check_int(astree_ptr node);

#endif