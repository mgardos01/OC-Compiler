
#include "lyutils.h"
#include "symbols.h"

string typesize(astree_ptr n);
// void parse_util::emit();
// void parse_util::emit(astree_ptr start);
void emit_structdef(astree_ptr n, FILE* fp);
void emit_function(astree_ptr n, FILE* fp);

void emit_global(astree_ptr n, FILE* fp);

void emit_typesize(astree_ptr n, FILE* fp);
void emit_stringdef(astree_ptr n, FILE* fp);
void emit_label(astree_ptr n, FILE* fp);
void emit_globaldef(astree_ptr n, FILE* fp);

void bubble_up_expr(astree_ptr start);
void bubble_up_typeid(astree_ptr start);
void bubble_up_assign(astree_ptr start);
void bubble_up_return(astree_ptr start);
void bubble_up_binop(astree_ptr start);
void bubble_up_unop(astree_ptr start);
void bubble_up_const(astree_ptr start);
void bubble_up_alloc(astree_ptr start);
void bubble_up_arrow(astree_ptr start);
void bubble_up_index(astree_ptr start);
void bubble_up_call(astree_ptr start);

void emit_fntype(astree_ptr n, FILE* fp);
void emit_params(astree_ptr n, FILE* fp);
void emit_instruction(astree_ptr n, FILE* fp);
void emit_condition(astree_ptr n, FILE* fp);
void emit_fncall(astree_ptr n, FILE* fp);
void emit_expression(astree_ptr n, FILE* fp);
void emit_selection(astree_ptr n, FILE* fp);
void emit_operand(astree_ptr n, FILE* fp);
void emit_binop(astree_ptr n, FILE* fp);
void emit_relop(astree_ptr n, FILE* fp);
void emit_unop(astree_ptr n, FILE* fp);
void emit_stride(astree_ptr n, FILE* fp);