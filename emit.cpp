#include <bitset>
#include <unordered_map>
#include <vector>

#include "emit.h"
#include "lyutils.h"
#include "symbols.h"

size_t if_n = 0;
size_t wh_n = 0;

int t_n_i = 0;
int t_n_p = 0;

string typesize(astree_ptr n)
{
    attr_bitset int_bitset;
    int_bitset.set(attr_INT);

    attr_bitset void_ptr_bitset;
    void_ptr_bitset.set(attr_NULLPTR_T);
    void_ptr_bitset.set(attr_STRING);
    void_ptr_bitset.set(attr_ARRAY);

    attr_bitset struct_bitset;
    struct_bitset.set(attr_STRUCT);

    if ((n->attributes & int_bitset).any()) {
        return "int " + *n->lexinfo;
    }
    if ((n->attributes & void_ptr_bitset).any()) {
        return "void* " + *n->lexinfo;
    }
    if ((n->attributes & struct_bitset).any()) {
        return "void* " + *n->lexinfo;
    }

    return "WEIRD TYPESIZE " + *n->lexinfo;
}

void emit_structdef(astree_ptr n, FILE* fp)
{
    astree_ptr struct_ident = n->children[0];
    fprintf(fp, ".struct   %s\n", struct_ident->lexinfo->c_str());

    astree_ptr field_name = nullptr;
    for (size_t i = 1; i < n->children.size(); i++) {
        field_name = n->children[i]->children[1];
        fprintf(fp, "\t\t.field %s\n", typesize(field_name).c_str());
    }

    fprintf(fp, ".end\n");
}

void parse_util::emit()
{
    for (auto child : astree_root->children) {
        switch (child->symbol) {
        case TOK_STRUCT:
            emit_structdef(child, ocil_file);
            break;
        case TOK_FUNCTION: {
            astree_ptr type_id = child->children[0];
            astree_ptr params = child->children[1];
            astree_ptr block = child->children[2];
            astree_ptr type_id_name = type_id->children[1];

            // EMIT FUNCTION NAME
            fprintf(ocil_file, ".function %s (",
                type_id_name->lexinfo->c_str());

            // EMIT PARAMS
            for (size_t i = 0; i < params->children.size(); i++) {
                astree_ptr param_type_id_name
                    = params->children[i]->children[1];
                if (i == 0) {
                    fprintf(ocil_file, "%s",
                        typesize(param_type_id_name).c_str());
                } else {
                    fprintf(ocil_file, ", %s",
                        typesize(param_type_id_name).c_str());
                }
            }
            fprintf(ocil_file, ")\n");

            // EMIT LOCAL
            for (auto local_child :
                local_emits[*type_id_name->lexinfo]) {
                fprintf(ocil_file, "\t\t.local %s\n",
                    typesize(local_child).c_str());
            }

            t_n_i = 0;
            t_n_p = 0;
            if_n = 0;
            wh_n = 0;
            // EMIT BLOCK
            emit(block);

            // EMIT END
            fprintf(ocil_file, "\t\treturn\n");
            fprintf(ocil_file, ".end\n");
            break;
        }
        case TOK_TYPE_ID:
            // FIXED! global definition or string def
            bubble_up_expr(child);
            break;
        default:
            break;
        }
    }
}

void parse_util::emit(astree_ptr n) // IN FUNCTION
{
    for (auto start : n->children) {
        switch (start->symbol) {
        case TOK_WHILE: {
            astree_ptr expr = start->children[0];
            astree_ptr while_block = start->children[1];
            size_t curr_wh_no = wh_n++;

            fprintf(ocil_file, ".wh%zu:\n", curr_wh_no);

            bubble_up_expr(expr);
            for (size_t i = 0; i < expr->vreg_statement.size(); i++) {
                fprintf(ocil_file, "\t\t%s\n",
                    expr->vreg_statement[i].c_str());
            }

            fprintf(ocil_file, "\t\tgoto .do%zu if ! %s\n", curr_wh_no,
                expr->vreg.c_str());
            fprintf(ocil_file, ".do%zu:\n", curr_wh_no);
            if (while_block->symbol != TOK_BLOCK) {
                bubble_up_expr(while_block);
            }
            emit(while_block);
            fprintf(ocil_file, "\t\tgoto .od%zu\n", curr_wh_no);
            fprintf(ocil_file, ".od%zu:\n", curr_wh_no);
            break;
        }
        case TOK_IF: {
            if (start == n->children[0]) {
                fprintf(ocil_file, "\n");
            }
            astree_ptr expr = start->children[0];
            astree_ptr if_block = start->children[1];
            astree_ptr else_block = nullptr;
            string alt_to_print = "";
            size_t curr_if_no = if_n++;
            if (start->children.size() == 3) {
                else_block = start->children[2];
                alt_to_print = "el" + to_string(curr_if_no) + ":";
            } else {
                alt_to_print = "fi" + to_string(curr_if_no) + ":";
            }

            fprintf(ocil_file, ".if%zu:\n", curr_if_no);

            bubble_up_expr(expr);
            for (size_t i = 0; i < expr->vreg_statement.size(); i++) {
                fprintf(ocil_file, "\t\t%s\n",
                    expr->vreg_statement[i].c_str());
            }
            fprintf(ocil_file, "\t\tgoto %s if ! %s\n",
                alt_to_print.c_str(), expr->vreg.c_str());
            fprintf(ocil_file, ".th%zu:\n", curr_if_no);
            if (if_block->symbol != TOK_BLOCK) {
                bubble_up_expr(if_block);
            }
            emit(if_block);
            fprintf(ocil_file, "\t\tgoto .fi%zu\n", curr_if_no);
            if (start->children.size() == 3) {
                fprintf(ocil_file, ".el%zu:\n", curr_if_no);
                if (else_block->symbol != TOK_BLOCK) {
                    bubble_up_expr(else_block);
                }
                emit(else_block);
            }
            fprintf(ocil_file, ".fi%zu:\n", curr_if_no);
            break;
        }
        case TOK_TYPE_ID:
        case '=':
        case '-':
        case '*':
        case '/':
        case '%':
        case TOK_EQ:
        case TOK_NE:
        case TOK_LT:
        case TOK_LE:
        case TOK_GT:
        case TOK_GE:
        case NOT:
        case POS:
        case NEG:
        case TOK_INTCON:
        case TOK_CHARCON:
        case TOK_STRINGCON:
        case TOK_NULLPTR:
        case TOK_IDENT:
        case TOK_ALLOC:
        case TOK_ARROW:
        case TOK_INDEX:
        case TOK_CALL:
        case TOK_RETURN: {
            bubble_up_expr(start);
            for (size_t i = 0; i < start->vreg_statement.size(); i++) {
                fprintf(ocil_file, "\t\t%s\n",
                    start->vreg_statement[i].c_str());
            }
            fprintf(ocil_file, "\t\t%s\n", start->vreg.c_str());
            break;
        }
        default: {
            break;
        }
        }
    }
}

void bubble_up_expr(astree_ptr start)
{
    for (auto child : start->children) {
        bubble_up_expr(child);
    }
    switch (start->symbol) {
    case TOK_TYPE_ID: {
        bubble_up_typeid(start);
        break;
    }
    case '=': {
        bubble_up_assign(start);
        break;
    }
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case TOK_EQ:
    case TOK_NE:
    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE: {
        bubble_up_binop(start);
        break;
    }
    case NOT:
    case POS:
    case NEG: {
        bubble_up_unop(start);
        break;
    }
    case TOK_INTCON:
    case TOK_CHARCON:
    case TOK_STRINGCON:
    case TOK_NULLPTR: {
        bubble_up_const(start);
        break;
    }
    case TOK_IDENT: {
        start->vreg = *start->lexinfo;
        break;
    }
    case TOK_ALLOC: {
        bubble_up_alloc(start);
        break;
    }
    case TOK_ARROW: {
        bubble_up_arrow(start);
        break;
    }
    case TOK_INDEX: {
        bubble_up_index(start);
        break;
    }
    case TOK_CALL: {
        bubble_up_call(start);
        break;
    }
    case TOK_RETURN: {
        bubble_up_return(start);
        break;
    }
    default: {
        break;
    }
    }
}

void bubble_up_typeid(astree_ptr start)
{
    if (start->children.size() == 3) {
        astree_ptr ident = start->children[1];
        astree_ptr assignment = start->children[2];
        start->vreg = *ident->lexinfo + " = " + assignment->vreg;
        start->vreg_statement.insert(start->vreg_statement.end(),
            assignment->vreg_statement.begin(),
            assignment->vreg_statement.end());
    }
}

void bubble_up_assign(astree_ptr start)
{
    astree_ptr lhs = start->children[0];
    astree_ptr rhs = start->children[1];
    start->vreg = lhs->vreg + " = " + rhs->vreg;
    start->vreg_statement.insert(start->vreg_statement.end(),
        lhs->vreg_statement.begin(), lhs->vreg_statement.end());
    start->vreg_statement.insert(start->vreg_statement.end(),
        rhs->vreg_statement.begin(), rhs->vreg_statement.end());
}

void bubble_up_return(astree_ptr start)
{
    astree_ptr expr = start->children[0];
    start->vreg_statement.insert(start->vreg_statement.end(),
        expr->vreg_statement.begin(), expr->vreg_statement.end());
    start->vreg = "return " + expr->vreg;
}

void bubble_up_binop(astree_ptr start)
{
    astree_ptr lhs = start->children[0];
    astree_ptr rhs = start->children[1];
    start->vreg = "$t" + to_string(t_n_i++) + ":i";
    start->vreg_statement.insert(start->vreg_statement.end(),
        lhs->vreg_statement.begin(), lhs->vreg_statement.end());
    start->vreg_statement.insert(start->vreg_statement.end(),
        rhs->vreg_statement.begin(), rhs->vreg_statement.end());
    start->vreg_statement.push_back(start->vreg + " = " + lhs->vreg
        + " " + *start->lexinfo + " " + rhs->vreg);
}

void bubble_up_unop(astree_ptr start)
{
    astree_ptr unop_child = start->children[0];
    start->vreg = "$t" + to_string(t_n_i++) + ":i";
    start->vreg_statement.insert(start->vreg_statement.end(),
        unop_child->vreg_statement.begin(),
        unop_child->vreg_statement.end());
    start->vreg_statement.push_back(
        start->vreg + " = " + *start->lexinfo + unop_child->vreg);
}

void bubble_up_const(astree_ptr start)
{
    switch (start->symbol) {
    case TOK_INTCON:
    case TOK_CHARCON:
    case TOK_STRINGCON: {
        start->vreg = *start->lexinfo;
        break;
    }
    case TOK_NULLPTR: {
        start->vreg = "0:p";
        break;
    }
    default: {
        break;
    }
    }
}

void bubble_up_alloc(astree_ptr start)
{
    astree_ptr child1 = start->children[0];
    switch (child1->symbol) {
    // alloc
    // |   ident
    case TOK_IDENT: {
        start->vreg
            = "malloc (sizeof (struct " + *child1->lexinfo + "))";
        break;
    }
    // alloc
    // |   string
    // |   expr
    case TOK_STRING: {
        astree_ptr expr = start->children[1];
        start->vreg = "malloc(" + expr->vreg + ")";
        break;
    }
    // alloc
    // |   array
    // |   |   plaintype
    // |   expr
    case TOK_ARRAY: {
        // ALIAS
        astree_ptr expr = start->children[1];
        astree_ptr plaintype = child1->children[0];

        string malloc_expr = "";
        string malloc_reg = "$t" + to_string(t_n_i++) + ":i";
        if (plaintype->symbol == TOK_INT) {
            malloc_expr
                = malloc_reg + " = " + expr->vreg + " * sizeof int";
        } else {
            malloc_expr
                = malloc_reg + " = " + expr->vreg + " * sizeof ptr";
        }

        start->vreg = "malloc(" + malloc_reg + ")";
        start->vreg_statement.insert(start->vreg_statement.end(),
            expr->vreg_statement.begin(), expr->vreg_statement.end());
        start->vreg_statement.push_back(malloc_expr);

        break;
    }
    default: {
        break;
    }
    }
}

void bubble_up_arrow(astree_ptr start)
{
    astree_ptr lhs = start->children[0];
    astree_ptr rhs = start->children[1];
    start->vreg
        = lhs->vreg + "->" + *lhs->struct_entry + "::" + rhs->vreg;
    start->vreg_statement.insert(start->vreg_statement.end(),
        lhs->vreg_statement.begin(), lhs->vreg_statement.end());
}

void bubble_up_index(astree_ptr start)
{
    astree_ptr lhs = start->children[0];
    astree_ptr rhs = start->children[1];
    string stride = "";
    if (lhs->attributes.test(attr_INT)) {
        stride = ":i";
    } else if (lhs->attributes.test(attr_STRING)) {
        stride = ":c";
    } else {
        stride = ":p";
    }
    start->vreg = lhs->vreg + "[" + rhs->vreg + " * " + stride + "]";
    start->vreg_statement.insert(start->vreg_statement.end(),
        lhs->vreg_statement.begin(), lhs->vreg_statement.end());
    start->vreg_statement.insert(start->vreg_statement.end(),
        rhs->vreg_statement.begin(), rhs->vreg_statement.end());
}

void bubble_up_call(astree_ptr start)
{
    astree_ptr call_ident = start->children[0];
    start->vreg = *call_ident->lexinfo + "(";
    for (size_t i = 1; i < start->children.size(); i++) {
        if (i == 1) {
            start->vreg += start->children[i]->vreg;
        } else {
            start->vreg += ", " + start->children[i]->vreg;
        }
        start->vreg_statement.insert(start->vreg_statement.end(),
            start->children[i]->vreg_statement.begin(),
            start->children[i]->vreg_statement.end());
    }
    start->vreg += ")";
}
