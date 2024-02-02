#include <bitset>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "lyutils.h"
#include "symbols.h"

#define LOCAL true
#define GLOBAL false

void print_symbol_table(symbol_table st)
{
    cout << "/st---print---\n";
    for (auto i = st.begin(); i != st.end(); i++) {
        auto s = i->first;
        cout << "\t" << *s << "->blocknr = " << st[s]->block_nr << "\n";
    }
    cout << "\\st---print---\n";
}

symbol_table type_name_table;
symbol_table global;
symbol_table local;
symbol_table struct_table;
size_t next_block = 1;
size_t local_sequence = 0;
unordered_map<string, vector<astree_ptr>> local_emits;
void delete_symbol_node(symbol_ptr s)
{
    if (s->parameters) {
        for (auto param : *s->parameters) {
            // cout << "\tdeleting param...\n";
            delete param;
        }
        // cout << "deleting parameters table...\n";
        delete s->parameters;
    }
    if (s->fields) {
        // cout << "deleting fields...\n";
        delete_symbol_table(*s->fields);
        delete s->fields;
    }
    if (s) {
        delete s;
    }
}

void delete_symbol_table(symbol_table st)
{
    for (auto i = st.begin(); i != st.end(); i++) {
        auto s = i->second;
        // cout << "deleting symbol node associated with " << *i->first
        // << "...\n";
        delete_symbol_node(s);
    }
}

// Assign type on vardecl or function TYPE_ID
void assign_on_type(astree_ptr type_id_node)
{
    astree_ptr type_id_type = type_id_node->children[0];
    astree_ptr type_id_name = type_id_node->children[1];
    switch (type_id_type->symbol) {
    case TOK_VOID: {
        type_id_name->attributes.set(attr_VOID);
        break;
    }
    case TOK_INT: {
        type_id_name->attributes.set(attr_INT);
        break;
    }
    case TOK_NULLPTR: {
        type_id_name->attributes.set(attr_NULLPTR_T);
        break;
    }
    case TOK_STRING: {
        type_id_name->attributes.set(attr_STRING);
        break;
    }
    case TOK_PTR: {
        astree_ptr struct_name = type_id_type->children[0];
        assign_on_type_struct(type_id_name, struct_name);
        break;
    }
    case TOK_ARRAY: {
        type_id_name->attributes.set(attr_ARRAY);
        astree_ptr plaintype = type_id_type->children[0];
        switch (plaintype->symbol) {
        case TOK_INT: {
            type_id_name->attributes.set(attr_INT);
            break;
        }
        case TOK_STRING: {
            type_id_name->attributes.set(attr_STRING);
            break;
        }
        case TOK_PTR: {
            astree_ptr struct_name = plaintype->children[0];
            assign_on_type_struct(type_id_name, struct_name);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void assign_on_type_struct(
    astree_ptr type_id_name, astree_ptr struct_name)
{
    type_id_name->attributes.set(attr_STRUCT);
    type_id_name->struct_entry = struct_name->lexinfo;
    if (struct_table.count(struct_name->lexinfo) == 0) {
        cerr << struct_name->loc.linenr << "."
             << struct_name->loc.offset
             << " error: reference to incomplete struct \""
             << *(struct_name->lexinfo) << "\"\n";
        struct_table[struct_name->lexinfo] = new symbol(struct_name);
        // struct_table.emplace(
        //     struct_name->lexinfo, new symbol(struct_name));
    }
}

void parse_util::traverse()
{
    for (astree_ptr child : astree_root->children) {
        if (child->symbol == TOK_FUNCTION) {
            // ALIAS
            astree_ptr type_id = child->children[0];
            astree_ptr params = child->children[1];
            astree_ptr block = child->children[2];
            astree_ptr type_id_name = type_id->children[1];
            // ATTRIBUTES
            type_id_name->attributes.set(attr_FUNCTION);
            if (block->symbol == ';') {
                type_id_name->attributes.set(attr_PROTOTYPE);
            }
            assign_on_type(type_id);
            // BLOCK NUMBER
            type_id_name->block_number = next_block++;
            // SEQUENCE
            type_id_name->sequence = 0;
            // WRITE SYMBOL
            write_symbol(symbol_file, type_id_name);
            // ASSIGNMENT
            if (!global.count(type_id_name->lexinfo)) {
                global[type_id_name->lexinfo]
                    = new symbol(type_id_name);
                // global.emplace(type_id_name->lexinfo, new
                // symbol(type_id_name));
                global[type_id_name->lexinfo]->parameters
                    = new vector<symbol_ptr>;
            }
            for (size_t i = 0; i < params->children.size();
                 i++) { // loop over each param
                astree_ptr param_type_id = params->children[i];
                astree_ptr param_type_id_name
                    = param_type_id->children[1];
                param_type_id_name->attributes.set(attr_PARAM);
                param_type_id_name->sequence = i;
                param_type_id_name->block_number
                    = type_id_name->block_number;
                assign_on_type(params->children[i]);
                write_symbol(symbol_file, param_type_id_name);
                global[type_id_name->lexinfo]->parameters->push_back(
                    new symbol(param_type_id_name));
                local[param_type_id_name->lexinfo]
                    = new symbol(param_type_id_name);
                // local.emplace(param_type_id_name->lexinfo, new
                // symbol(param_type_id_name));
            }
            // TRAVERSE
            traverse(type_id_name->lexinfo, block, LOCAL,
                type_id_name->block_number);
            // NEW LINE
            write_line(symbol_file);
            // CLEAR LOCAL
            // delete_symbol_table(local);

            delete_symbol_table(local);
            local.clear();
            local_sequence = 0;
        } else if (child->symbol == TOK_STRUCT) {
            // ALIAS
            astree_ptr struct_name = child->children[0];
            // ATTRIBUTES
            struct_name->attributes.set(attr_STRUCT);
            // BLOCK NUMBER
            struct_name->block_number = 0;
            // SEQUENCE
            struct_name->sequence = 0;
            // STRUCT ENTRY
            struct_name->struct_entry = struct_name->lexinfo;
            // WRITE SYMBOL
            write_symbol(symbol_file, struct_name);
            if (struct_table.count(struct_name->lexinfo)
                && struct_table[struct_name->lexinfo]->fields) {
                cerr
                    << "PREVIOUSLY DEFINED STRUCT WITH FIELDS\n";
                struct_table.erase(struct_name->lexinfo);
            } else {
                struct_table.emplace(
                    struct_name->lexinfo, new symbol(struct_name));
            }
            // ASSIGNMENT
            auto fields = struct_table[struct_name->lexinfo]->fields
                = new symbol_table();
            for (size_t i = 1; i < child->children.size(); i++) {
                astree_ptr type_id_name
                    = child->children[i]->children[1];
                type_id_name->attributes.set(attr_FIELD);
                assign_on_type(child->children[i]);
                write_symbol(symbol_file, type_id_name);
                fields->emplace(
                    type_id_name->lexinfo, new symbol(type_id_name));
            }
        } else {
            // BLOCK NUMBER
            child->block_number = 0;
            // SEQUENCE
            child->sequence = 0;
            string dummy = "";
            traverse(&dummy, child, GLOBAL, 0);
        }
    }
    delete_symbol_table(struct_table);
    delete_symbol_table(global);
}

void parse_util::traverse(const string* name, astree_ptr start,
    bool is_local, int block_number)
{
    for (astree_ptr child : start->children) {
        traverse(name, child, is_local, block_number);
    }
    start->block_number = block_number;
    switch (start->symbol) {
    case ';': // this function is a prototype, so there is nothing more
              // to see.
    {
        return;
    }
    case TOK_TYPE_ID: // variable declaration
    {
        type_check_type_id(start, is_local, symbol_file, name);
        break;
    }
    case '=':
        type_check_assign(start);
        break;
    case TOK_RETURN: {
        type_check_return(start, name);
        break;
    }
    case TOK_EQ:
    case TOK_NE: {
        type_check_eq(start);
        break;
    }
    case '<':
    case TOK_LE:
    case '>':
    case TOK_GE:
    case '+':
    case '-':
    case '*':
    case '/':
    case '%': {
        type_check_binop(start);
        break;
    }
    case TOK_NOT:
    case POS:
    case NEG: {
        type_check_unop(start);
        break;
    }

    case TOK_ALLOC: {
        type_check_alloc(start);
        break;
    }
    case TOK_CALL: {
        type_check_call(start);
        break;
    }
    case TOK_IDENT: { 
        type_check_ident(start);
        break;
    }
    case TOK_INDEX: {
        type_check_index(start);
        break;
    }
    case TOK_ARROW: {
        type_check_arrow(start);
        break;
    } 
    case TOK_INTCON:
    case TOK_CHARCON: {
        type_check_intcon(start);
        break;
    }
    case TOK_STRINGCON: {
        type_check_stringcon(start);
        break;
    }
    case TOK_NULLPTR: {
        type_check_nullptr(start);
        break;
    }
    }
}

void type_check_type_id(astree_ptr node, bool is_local,
    FILE* symbol_file, const string* fn_name)
{
    // ALIAS
    astree_ptr type_id_name = node->children[1];
    // astree_ptr type_id_rhs = nullptr;
    // if (node->children.size() == 3) {
    //     type_id_rhs = node->children[2];
    // }
    // ASSIGNMENT
    assign_on_type(node);
    if (is_local) {
        type_id_name->attributes.set(attr_LOCAL);
    }
    // TYPE CHECKING
    // IF THERE IS AN ASSIGNMENT '='
    if (node->children.size() == 3) {
        // FIXMENOPE NEVER GONNA LOL!!! check_compatible(type_id_name,
        // type_id_rhs);
    }
    if (is_local) {
        if (local.count(type_id_name->lexinfo)) {
            cerr << type_id_name->loc.linenr << "."
                 << type_id_name->loc.offset << " error: "
                 << "\"" << *type_id_name->lexinfo << "\""
                 << " has already been locally declared.\n";
        } else {
            local_emits[*fn_name].push_back(type_id_name);
            local.emplace(
                type_id_name->lexinfo, new symbol(type_id_name));
        }
    } else {
        if (global.count(type_id_name->lexinfo)) {
            cerr << type_id_name->loc.linenr << "."
                 << type_id_name->loc.offset << " error: "
                 << "\"" << *type_id_name->lexinfo << "\""
                 << " has already been globally declared.\n";
        } else {
            global.emplace(
                type_id_name->lexinfo, new symbol(type_id_name));
        }
    }
    write_symbol(symbol_file, type_id_name);
}

void type_check_assign(astree_ptr node)
{
    // ALIAS
    astree_ptr lhs = node->children[0];
    // astree_ptr rhs = node->children[1];
    // ASSIGNMENT
    node->attributes |= lhs->attributes;
    node->attributes.set(attr_LVAL);
    node->attributes.set(attr_VREG);
    // TYPE_CHECKING
    if (lhs->attributes.test(attr_NULLPTR_T)) {
        cerr << lhs->loc.linenr << "." << lhs->loc.offset << " error: "
             << "\"" << *lhs->lexinfo << "\""
             << " is on the left hand side and is of type nullptr_t.\n";
    }
    if (!lhs->attributes.test(attr_LVAL)) {
        cerr
            << lhs->loc.linenr << "." << lhs->loc.offset << " error: "
            << "\"" << *lhs->lexinfo << "\""
            << " is on the left hand side and does not of type lval.\n";
    }
    // FIXME check_compatible(lhs, rhs);
}

//[] ALMOST
void type_check_return(astree_ptr node, const string* fn_name)
{
    if (!global.count(fn_name)) { // THIS WILL NEVER HAPPEN LOL???
        cerr << node->loc.linenr << "." << node->loc.offset
             << " error: return called with invalid function name\n";
        return;
    }
    if (!node->children.size()) { // NO CHILDREN
        if (!global[fn_name]->attributes.test(attr_VOID)) {
            cerr << node->loc.linenr << "." << node->loc.offset
                 << " error: "
                 << "return called without operand without return type "
                    "VOID\n";
        }
        return; // DONT DO ANYTHING ELSE
    }
    // HAS CHILDREN
    if (global[fn_name]->attributes.test(attr_VOID)) {
        cerr << node->loc.linenr << "." << node->loc.offset
             << " error: "
             << "return called with operand with return type VOID\n";
        return;
    }
    // astree_ptr return_operand = node->children[0];
    // CHECK COMPATIBLE MUST BE SPECIAL BC ITS CHECKING AGAINST SYMBOL
    // NODE!!!
    // FIXME     return_check_compatible(return_operand,
    // global[fn_name]);
}

void type_check_eq(astree_ptr node)
{
    // ALIAS
    astree_ptr lhs = node->children[0];
    // astree_ptr rhs = node->children[1];
    // ASSIGNMENT
    node->attributes.set(attr_INT);
    node->attributes.set(attr_VREG);
    // TYPE_CHECK
    if (lhs->attributes.test(attr_NULLPTR_T)) {
        cerr << lhs->loc.linenr << "." << lhs->loc.offset << " error: "
             << "\"" << *lhs->lexinfo << "\""
             << " is on the left hand side and is of type nullptr_t.\n";
    }
    // FIXME     check_compatible(lhs, rhs);
}

void type_check_binop(astree_ptr node)
{
    // ALIAS
    astree_ptr lhs = node->children[0];
    astree_ptr rhs = node->children[1];
    // ASSIGNMENT
    node->attributes.set(attr_INT);
    node->attributes.set(attr_VREG);
    // TYPE CHECK
    check_int(lhs);
    check_int(rhs);
}

void type_check_unop(astree_ptr node)
{
    // ALIAS
    astree_ptr child1 = node->children[0];
    // ASSIGNMENT
    node->attributes.set(attr_INT);
    node->attributes.set(attr_VREG);
    // TYPE CHECK
    check_int(child1);
}

void type_check_alloc(astree_ptr node)
{
    // ALIAS
    astree_ptr child1 = node->children[0];
    // BASE ASSIGNMENT
    node->attributes.set(attr_VREG);

    switch (child1->symbol) {
    // alloc
    // |   ident
    case TOK_IDENT: {
        // ASSIGNMENT
        node->attributes.set(attr_STRUCT);
        node->struct_entry = child1->struct_entry; 
        node->attributes |= child1->attributes;
        break;
    }
    // alloc
    // |   string
    // |   expr
    case TOK_STRING: {
        // ALIAS
        astree_ptr child2 = node->children[1];
        // ASSIGNMENT
        node->attributes.set(attr_STRING);
        // TYPE CHECK
        check_int(child2);
        break;
    }
    // alloc
    // |   array
    // |   |   plaintype
    // |   expr
    case TOK_ARRAY: {
        // ALIAS
        astree_ptr child2 = node->children[1];
        // ASSIGNMENT
        node->attributes |= child1->children[0]->attributes;
        // TYPE CHECK
        check_int(child2);
        break;
    }
    default:
        break;
    }
}

void type_check_call(astree_ptr node)
{
    // ALIAS
    astree_ptr call_ident = node->children[0];
    // BASE ASSIGNMENT
    node->attributes.set(attr_VREG);

    // CHECK IF FUNCTION DEFINED
    if (!global.count(call_ident->lexinfo)) {
        cerr << call_ident->loc.linenr << "." << call_ident->loc.offset
             << " error: "
             << "\"" << *call_ident->lexinfo << "\""
             << " is not a defined function.\n";
        return;
    }

    // ASSIGN FUNCTION TYPE
    node->attributes |= global[call_ident->lexinfo]->attributes;

    // CHECK PARAM COMPATIBILITY - WILL ONLY DISPLAY ERRORS
    auto function_params = global[call_ident->lexinfo]->parameters;
    if (function_params->size() + 1 != node->children.size()) {
        cerr << call_ident->loc.linenr << "." << call_ident->loc.offset
             << " error: "
             << "\"" << *call_ident->lexinfo << "\""
             << " function call parameters do not match.\n";
    }
    for (size_t i = 0; i < function_params->size(); i++) {
        // FIXME check_compatible(node->children[i + 1],
        // function_params->at(i));
    }
}

void type_check_ident(astree_ptr node)
{
    if (local.count(node->lexinfo)) {
        node->attributes = local[node->lexinfo]->attributes;
        if (local[node->lexinfo]->struct_entry) { }
        node->struct_entry = local[node->lexinfo]->struct_entry;
    } else if (global.count(node->lexinfo)) {
        node->attributes = global[node->lexinfo]->attributes;
    }
    if (!node->attributes.test(attr_FUNCTION)) {
        node->attributes.set(attr_VARIABLE);
        node->attributes.set(attr_LVAL);
    }
}

void type_check_index(astree_ptr node)
{
    // 'string' '[' expr:mustbe:int ']' -> int vaddr lval
    // 'array' base '[' expr:mustbe:int ']' -> base vaddr lval
    astree_ptr lhs = node->children[0];
    astree_ptr rhs = node->children[1];
    switch (lhs->symbol) { // first child of index is either array or
                           // string
    // TOK_INDEX
    // |   TOK_STRING
    // |   expr:mustbe:int
    case TOK_STRING: {
        check_int(rhs);
        node->attributes.set(attr_INT);
        node->attributes.set(attr_VADDR);
        node->attributes.set(attr_LVAL);
        break;
    }
    // TOK_INDEX
    // |   TOK_ARRAY
    // |   |   base
    // |   expr:mustbe:int
    case TOK_ARRAY: {
        check_int(rhs);
        // copy all attributes from plaintype
        astree_ptr array_type_child = lhs->children[0];
        node->attributes |= array_type_child->attributes;
        break;
    }
    default:
        break;
    }
}

void type_check_arrow(astree_ptr node)
{
    // ALIAS
    astree_ptr lhs = node->children[0];
    astree_ptr rhs = node->children[1];
    node->struct_entry = lhs->struct_entry;
    // BASE ASSIGNMENT
    node->attributes.set(attr_VADDR);
    node->attributes.set(attr_LVAL);
    rhs->attributes.set(attr_FIELD);

    // LHS IS NOT A STRUCT
    if (!lhs->attributes.test(attr_STRUCT)) {
        cerr << node->loc.linenr << "." << node->loc.offset
             << " error: "
             << "\"" << *lhs->lexinfo << "\""
             << " is not a struct.\n";
        return;
    }
    // NO REFERENCE TO NAMES STRUCT OR
    // REFERENCE TO NAMED STRUCT NOT IN STRUCT_TABLE
    if (!lhs->struct_entry || !struct_table.count(lhs->struct_entry)) {
        cerr << node->loc.linenr << "." << node->loc.offset
             << " error: "
             << "\"" << *lhs->lexinfo << "\""
             << " has struct type with no struct to reference.\n";
        return;
    }
    // STRUCT TABLE LHS REFERS TO IS INCOMPLETE
    if (!struct_table[lhs->struct_entry]->fields) {
        cerr << node->loc.linenr << "." << node->loc.offset
             << " error: "
             << "\"" << *lhs->lexinfo << "\""
             << " references an incomplete struct.\n";
        return;
    }
    // STRUCT TABLE IS COMPLETE BUT FIELDS HAS NO INSTANCE OF SELECTED
    // FIELD
    if (!struct_table[lhs->struct_entry]->fields->count(rhs->lexinfo)) {
        cerr << node->loc.linenr << "." << node->loc.offset
             << " error: "
             << "\"" << *lhs->lexinfo << "\""
             << " references a struct without a"
             << "\"" << *rhs->lexinfo << "\""
             << "field.\n";
        return;
    }

    // AFTER ALL CHECKS, ASSIGN ALL ATTRIBUTES FROM OTHER TABLE TO BASE
    // NODE
    node->attributes |= struct_table[lhs->struct_entry]
                            ->fields->at(rhs->lexinfo)
                            ->attributes;
    node->attributes.reset(attr_FIELD);
}

void type_check_intcon(astree_ptr node)
{
    node->attributes.set(attr_INT);
    node->attributes.set(attr_CONST);
}

void type_check_stringcon(astree_ptr node)
{
    node->attributes.set(attr_STRING);
    node->attributes.set(attr_CONST);
}

void type_check_nullptr(astree_ptr node)
{
    node->attributes.set(attr_NULLPTR_T);
    node->attributes.set(attr_CONST);
}

void check_int(astree_ptr node)
{
    if (!(node->attributes.test(attr_INT))) {
        cerr << node->loc.linenr << "." << node->loc.offset
             << " error:" << *(node->lexinfo)
             << " supposed to be int\n";
    }
}

// void check_compatible_return() {

// }
// void check_compatible() {

// }

string print_attributes(astree_ptr start)
{
    string to_return = "";
    if (start->attributes.test(attr_VOID)) {
        to_return += "void ";
    }
    if (start->attributes.test(attr_INT)) {
        to_return += "int ";
    }
    if (start->attributes.test(attr_NULLPTR_T)) {
        to_return += "nullptr_t ";
    }
    if (start->attributes.test(attr_STRING)) {
        to_return += "string ";
    }
    if (start->attributes.test(attr_STRUCT)) {
        to_return += "struct ";
        if (start->struct_entry) {
            to_return += "\"" + *(start->struct_entry) + "\" ";
        }
    }
    if (start->attributes.test(attr_ARRAY)) {
        to_return += "array ";
    }
    if (start->attributes.test(attr_FUNCTION)) {
        to_return += "function ";
    }
    if (start->attributes.test(attr_VARIABLE)) {
        to_return += "variable ";
        symbol_ptr definition = nullptr;
        if (local.count(start->lexinfo)) {
            definition = local[start->lexinfo];
        }
        if (global.count(start->lexinfo)) {
            definition = global[start->lexinfo];
        }
        if (definition) {
            to_return += "(" + to_string(definition->lloc.filenr) + "."
                + to_string(definition->lloc.linenr) + "."
                + to_string(definition->lloc.offset) + ") ";
        }
    }
    if (start->attributes.test(attr_FIELD)) {
        to_return += "field ";
    }
    if (start->attributes.test(attr_PROTOTYPE)) {
        to_return += "prototype ";
    }
    if (start->attributes.test(attr_PARAM)) {
        to_return += "param ";
    }
    if (start->attributes.test(attr_LOCAL)) {
        to_return += "local ";
    }
    if (start->attributes.test(attr_LVAL)) {
        to_return += "lval ";
    }
    if (start->attributes.test(attr_CONST)) {
        to_return += "const ";
    }
    if (start->attributes.test(attr_VREG)) {
        to_return += "vreg ";
    }
    if (start->attributes.test(attr_VADDR)) {
        to_return += "vaddr ";
    }
    return to_return;
}

void write_line(FILE* symbol_file) { fprintf(symbol_file, "\n"); }

void write_symbol(FILE* symbol_file, astree_ptr type_id_name)
{
    if (!type_id_name) {
        return;
    }
    string attrs_to_print = "";
    bool tabbed = false;
    if (type_id_name->attributes.test(attr_VOID)) {
        attrs_to_print += "void ";
    }
    if (type_id_name->attributes.test(attr_INT)) {
        attrs_to_print += "int ";
    }
    if (type_id_name->attributes.test(attr_NULLPTR_T)) {
        attrs_to_print += "nullptr_t ";
    }
    if (type_id_name->attributes.test(attr_STRING)) {
        attrs_to_print += "string ";
    }
    if (type_id_name->attributes.test(attr_STRUCT)) {
        if (struct_table.count(type_id_name->lexinfo)) {
            attrs_to_print += "struct ";
            if (type_id_name->struct_entry) {
                attrs_to_print += *type_id_name->struct_entry;
            }
        } else {
            attrs_to_print += "ptr<struct ";
            if (type_id_name->struct_entry)
                attrs_to_print += *type_id_name->struct_entry;
            attrs_to_print += "> ";
        }
    }
    if (type_id_name->attributes.test(attr_ARRAY)) {
        attrs_to_print += "array ";
    }
    if (type_id_name->attributes.test(attr_FUNCTION)) {
        attrs_to_print += "function ";
    };
    if (type_id_name->attributes.test(attr_VARIABLE)) {
        attrs_to_print += "variable ";
    }
    if (type_id_name->attributes.test(attr_FIELD)) {
        attrs_to_print += "field " + to_string(type_id_name->sequence);
        tabbed = true;
    }
    if (type_id_name->attributes.test(attr_PROTOTYPE)) {
        attrs_to_print += "prototype ";
    }
    if (type_id_name->attributes.test(attr_PARAM)) {
        attrs_to_print += "param " + to_string(type_id_name->sequence);
        tabbed = true;
    }
    if (type_id_name->attributes.test(attr_LVAL)) {
        attrs_to_print += "lval ";
    }
    if (type_id_name->attributes.test(attr_LOCAL)) {
        attrs_to_print += "local " + to_string(type_id_name->sequence);
        tabbed = true;
    }
    if (type_id_name->attributes.test(attr_CONST)) {
        attrs_to_print += "const ";
    }
    if (type_id_name->attributes.test(attr_VREG)) {
        attrs_to_print += "vreg ";
    }
    if (type_id_name->attributes.test(attr_VADDR)) {
        attrs_to_print += "vaddr ";
    }
    string indentation = "";
    if (tabbed) {
        indentation = "   ";
    }
    fprintf(symbol_file, "%s%s (%zu.%zu.%zu) {%lu} %s\n",
        indentation.c_str(), type_id_name->lexinfo->c_str(),
        type_id_name->loc.filenr, type_id_name->loc.linenr,
        type_id_name->loc.offset, type_id_name->block_number,
        attrs_to_print.c_str());
}

void parse_util::write_ast() { write_ast(astree_root, 0); }

void parse_util::write_ast(astree_ptr start, int depth)
{
    for (int i = 0; i < depth; i++) {
        fprintf(ast_file, "|  ");
    }
    const char* tname = get_parser_yytname(start->symbol);
    if (strstr(tname, "TOK_") == tname)
        tname += 4;
    fprintf(ast_file, "%s \"%s\" (%zu.%zu.%zu) {%zu} %s\n", tname,
        start->lexinfo->c_str(), start->loc.filenr, start->loc.linenr,
        start->loc.offset, start->block_number,
        print_attributes(start).c_str());
    for (astree_ptr child : start->children) {
        write_ast(child, depth + 1);
    }
}
