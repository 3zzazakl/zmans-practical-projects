#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* my_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* d = malloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

typedef enum {
    TOK_EOF, KW_INT, KW_FLOAT, KW_IF, KW_ELSE, KW_WHILE,
    ID, NUM_INT, NUM_FLOAT,
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_ASSIGN,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
    DELIM_SEMI, DELIM_LBRACE, DELIM_RBRACE, DELIM_LPAREN, DELIM_RPAREN,
    TOK_ERR
} TokenType;

const char* token_names[] = {
    "EOF","KW_INT","KW_FLOAT","KW_IF","KW_ELSE","KW_WHILE",
    "ID","NUM_INT","NUM_FLOAT",
    "OP_PLUS","OP_MINUS","OP_MUL","OP_DIV","OP_ASSIGN",
    "OP_LT","OP_GT","OP_LE","OP_GE","OP_EQ","OP_NE",
    "SEMI","LBRACE","RBRACE","LPAREN","RPAREN","ERROR"
};

typedef struct Token {
    TokenType type;
    char* lexeme;
    int line, col;
    struct Token* next;
} Token;

typedef struct {
    FILE* file;
    char cur;
    int line, col, eof;
    Token *first, *last;
} Lexer;

void lex_init(Lexer* l, const char* fname) {
    l->file = fopen(fname, "r");
    if (!l->file) { perror(fname); exit(1); }
    l->line = 1; l->col = 0; l->eof = 0;
    l->first = l->last = NULL;
    int c = fgetc(l->file);
    if (c == EOF) l->eof = 1;
    else { l->cur = c; l->col = 1; }
}

void lex_next(Lexer* l) {
    if (l->eof) return;
    int c = fgetc(l->file);
    if (c == EOF) { l->eof = 1; l->cur = 0; }
    else {
        l->cur = c;
        if (c == '\n') { l->line++; l->col = 0; }
        l->col++;
    }
}

void lex_skip_ws(Lexer* l) {
    while (!l->eof && (l->cur == ' ' || l->cur == '\t' || l->cur == '\n'))
        lex_next(l);
}

void lex_skip_line_comment(Lexer* l) {
    while (!l->eof && l->cur != '\n') lex_next(l);
    lex_next(l);
}

void lex_skip_block_comment(Lexer* l) {
    while (!l->eof) {
        if (l->cur == '*') {
            lex_next(l);
            if (l->cur == '/') { lex_next(l); return; }
        } else lex_next(l);
    }
}

void lex_add_token(Lexer* l, TokenType type, const char* lexeme, int line, int col) {
    Token* t = malloc(sizeof(Token));
    t->type = type;
    t->lexeme = my_strdup(lexeme);
    t->line = line; t->col = col; t->next = NULL;
    if (l->last) l->last->next = t;
    else l->first = t;
    l->last = t;
}

Token* lex_next_token(Lexer* l) {
    char buf[256];
    while (!l->eof) {
        lex_skip_ws(l);
        if (l->eof) break;
        int start_line = l->line, start_col = l->col;
        char c = l->cur;
        if (isalpha(c) || c == '_') {
            int pos = 0;
            while (isalnum(l->cur) || l->cur == '_') {
                buf[pos++] = l->cur;
                lex_next(l);
            }
            buf[pos] = '\0';
            TokenType t;
            if (!strcmp(buf, "int")) t = KW_INT;
            else if (!strcmp(buf, "float")) t = KW_FLOAT;
            else if (!strcmp(buf, "if")) t = KW_IF;
            else if (!strcmp(buf, "else")) t = KW_ELSE;
            else if (!strcmp(buf, "while")) t = KW_WHILE;
            else t = ID;
            lex_add_token(l, t, buf, start_line, start_col);
            return l->last;
        }
        if (isdigit(c)) {
            int pos = 0, is_float = 0;
            while (isdigit(l->cur)) {
                buf[pos++] = l->cur;
                lex_next(l);
            }
            if (l->cur == '.') {
                is_float = 1;
                buf[pos++] = '.';
                lex_next(l);
                while (isdigit(l->cur)) {
                    buf[pos++] = l->cur;
                    lex_next(l);
                }
            }
            buf[pos] = '\0';
            lex_add_token(l, is_float ? NUM_FLOAT : NUM_INT, buf, start_line, start_col);
            return l->last;
        }
        lex_next(l);
        switch (c) {
            case '+': lex_add_token(l, OP_PLUS, "+", start_line, start_col); return l->last;
            case '-': lex_add_token(l, OP_MINUS, "-", start_line, start_col); return l->last;
            case '*': lex_add_token(l, OP_MUL, "*", start_line, start_col); return l->last;
            case '/':
                if (l->cur == '/') { lex_next(l); lex_skip_line_comment(l); continue; }
                if (l->cur == '*') { lex_next(l); lex_skip_block_comment(l); continue; }
                lex_add_token(l, OP_DIV, "/", start_line, start_col);
                return l->last;
            case '=':
                if (l->cur == '=') { lex_next(l); lex_add_token(l, OP_EQ, "==", start_line, start_col); }
                else lex_add_token(l, OP_ASSIGN, "=", start_line, start_col);
                return l->last;
            case '<':
                if (l->cur == '=') { lex_next(l); lex_add_token(l, OP_LE, "<=", start_line, start_col); }
                else lex_add_token(l, OP_LT, "<", start_line, start_col);
                return l->last;
            case '>':
                if (l->cur == '=') { lex_next(l); lex_add_token(l, OP_GE, ">=", start_line, start_col); }
                else lex_add_token(l, OP_GT, ">", start_line, start_col);
                return l->last;
            case '!':
                if (l->cur == '=') { lex_next(l); lex_add_token(l, OP_NE, "!=", start_line, start_col); }
                else lex_add_token(l, TOK_ERR, "!", start_line, start_col);
                return l->last;
            case ';': lex_add_token(l, DELIM_SEMI, ";", start_line, start_col); return l->last;
            case '{': lex_add_token(l, DELIM_LBRACE, "{", start_line, start_col); return l->last;
            case '}': lex_add_token(l, DELIM_RBRACE, "}", start_line, start_col); return l->last;
            case '(': lex_add_token(l, DELIM_LPAREN, "(", start_line, start_col); return l->last;
            case ')': lex_add_token(l, DELIM_RPAREN, ")", start_line, start_col); return l->last;
            default:
                { char err[2] = {c,0}; lex_add_token(l, TOK_ERR, err, start_line, start_col); return l->last; }
        }
    }
    lex_add_token(l, TOK_EOF, "EOF", l->line, l->col);
    return l->last;
}

typedef enum { NODE_PROG, NODE_DECL, NODE_ASSIGN, NODE_IF, NODE_WHILE, NODE_BINOP, NODE_ID, NODE_INT, NODE_FLOAT } NodeType;
typedef struct ASTNode {
    NodeType type; int line, col;
    union {
        struct { char* name; int is_float; struct ASTNode* init; } decl;
        struct { char* name; struct ASTNode* expr; } assign;
        struct { struct ASTNode* cond, *then, *els; } ifs;
        struct { struct ASTNode* cond, *body; } wh;
        struct { char op; int rel; struct ASTNode* l, *r; } bin;
        struct { char* name; } id;
        struct { int ival; } ilit;
        struct { float fval; } flit;
    } u;
    struct ASTNode* next;
} ASTNode;

ASTNode* new_node(NodeType t, int line, int col) {
    ASTNode* n = calloc(1, sizeof(ASTNode));
    n->type = t; n->line = line; n->col = col;
    return n;
}

/* ---------- Symbol Table with error flag ---------- */
typedef struct Sym { char* name; int is_float; struct Sym* next; } Sym;
Sym* symtab = NULL;
int semantic_errors = 0;   // global error counter

Sym* sym_find(const char* name) {
    for (Sym* s = symtab; s; s = s->next) if (!strcmp(s->name, name)) return s;
    return NULL;
}

void sym_insert(const char* name, int is_float, int line, int col) {
    if (sym_find(name)) {
        fprintf(stderr, "Semantic error at line %d, col %d: variable '%s' already declared\n", line, col, name);
        semantic_errors = 1;
        return;
    }
    Sym* s = malloc(sizeof(Sym));
    s->name = my_strdup(name);
    s->is_float = is_float;
    s->next = symtab;
    symtab = s;
}

/* Type of an expression: 0 = int, 1 = float, -1 = error */
int expr_type(ASTNode* e) {
    if (!e) return -1;
    switch (e->type) {
        case NODE_INT: return 0;
        case NODE_FLOAT: return 1;
        case NODE_ID: {
            Sym* s = sym_find(e->u.id.name);
            if (!s) {
                fprintf(stderr, "Semantic error at line %d, col %d: variable '%s' not declared\n", e->line, e->col, e->u.id.name);
                semantic_errors = 1;
                return -1;
            }
            return s->is_float;
        }
        case NODE_BINOP: {
            int lt = expr_type(e->u.bin.l);
            int rt = expr_type(e->u.bin.r);
            if (lt == -1 || rt == -1) return -1;
            if (lt == 1 || rt == 1) return 1;   // float over int
            return 0;
        }
        default: return -1;
    }
}

/* Check assignment compatibility */
void check_assign(const char* varname, ASTNode* rhs, int line, int col) {
    Sym* var = sym_find(varname);
    if (!var) {
        fprintf(stderr, "Semantic error at line %d, col %d: variable '%s' not declared\n", line, col, varname);
        semantic_errors = 1;
        return;
    }
    int rhs_t = expr_type(rhs);
    if (rhs_t == -1) return;
    if (var->is_float == 0 && rhs_t == 1) {
        fprintf(stderr, "Semantic error at line %d, col %d: cannot assign float to int variable '%s'\n", line, col, varname);
        semantic_errors = 1;
    }
}

/* ---------- Parser ---------- */
typedef struct { Token* cur; int err; } Parser;
void parse_error(Parser* p, const char* msg) {
    if (p->cur) fprintf(stderr, "Syntax error at line %d: %s (near '%s')\n", p->cur->line, msg, p->cur->lexeme);
    p->err = 1;
}
void expect(Parser* p, TokenType t, const char* msg) {
    if (!p->cur || p->cur->type != t) parse_error(p, msg);
    else p->cur = p->cur->next;
}
int check(Parser* p, TokenType t) { return p->cur && p->cur->type == t; }

ASTNode* parse_expr(Parser* p);
ASTNode* parse_stmt(Parser* p);
ASTNode* parse_stmt_list(Parser* p);

ASTNode* parse_factor(Parser* p) {
    ASTNode* n = NULL;
    if (check(p, ID)) {
        n = new_node(NODE_ID, p->cur->line, p->cur->col);
        n->u.id.name = my_strdup(p->cur->lexeme);
        p->cur = p->cur->next;
    } else if (check(p, NUM_INT)) {
        n = new_node(NODE_INT, p->cur->line, p->cur->col);
        n->u.ilit.ival = atoi(p->cur->lexeme);
        p->cur = p->cur->next;
    } else if (check(p, NUM_FLOAT)) {
        n = new_node(NODE_FLOAT, p->cur->line, p->cur->col);
        n->u.flit.fval = atof(p->cur->lexeme);
        p->cur = p->cur->next;
    } else if (check(p, DELIM_LPAREN)) {
        p->cur = p->cur->next;
        n = parse_expr(p);
        expect(p, DELIM_RPAREN, "expected ')'");
    } else parse_error(p, "expected identifier, number or '('");
    return n;
}

ASTNode* parse_term(Parser* p) {
    ASTNode* left = parse_factor(p);
    while (check(p, OP_MUL) || check(p, OP_DIV)) {
        char op = (p->cur->type == OP_MUL) ? '*' : '/';
        p->cur = p->cur->next;
        ASTNode* right = parse_factor(p);
        ASTNode* n = new_node(NODE_BINOP, left->line, left->col);
        n->u.bin.op = op; n->u.bin.rel = 0; n->u.bin.l = left; n->u.bin.r = right;
        left = n;
    }
    return left;
}

ASTNode* parse_additive(Parser* p) {
    ASTNode* left = parse_term(p);
    while (check(p, OP_PLUS) || check(p, OP_MINUS)) {
        char op = (p->cur->type == OP_PLUS) ? '+' : '-';
        p->cur = p->cur->next;
        ASTNode* right = parse_term(p);
        ASTNode* n = new_node(NODE_BINOP, left->line, left->col);
        n->u.bin.op = op; n->u.bin.rel = 0; n->u.bin.l = left; n->u.bin.r = right;
        left = n;
    }
    return left;
}

ASTNode* parse_expr(Parser* p) {
    ASTNode* left = parse_additive(p);
    if (check(p, OP_LT) || check(p, OP_GT) || check(p, OP_LE) || check(p, OP_GE) || check(p, OP_EQ) || check(p, OP_NE)) {
        char op = p->cur->lexeme[0];
        p->cur = p->cur->next;
        ASTNode* right = parse_additive(p);
        ASTNode* n = new_node(NODE_BINOP, left->line, left->col);
        n->u.bin.op = op; n->u.bin.rel = 1; n->u.bin.l = left; n->u.bin.r = right;
        left = n;
    }
    return left;
}

ASTNode* parse_decl(Parser* p) {
    int is_float = check(p, KW_FLOAT);
    if (!is_float && !check(p, KW_INT)) { parse_error(p, "expected int or float"); return NULL; }
    p->cur = p->cur->next;
    if (!check(p, ID)) { parse_error(p, "expected identifier"); return NULL; }
    Token* id = p->cur;
    p->cur = p->cur->next;
    ASTNode* init = NULL;
    if (check(p, OP_ASSIGN)) {
        p->cur = p->cur->next;
        init = parse_expr(p);
    }
    expect(p, DELIM_SEMI, "expected ';' after declaration");
    // SEMANTIC: insert into symbol table (duplicate detection inside sym_insert)
    sym_insert(id->lexeme, is_float, id->line, id->col);
    // type check initializer
    if (init && !semantic_errors) {
        int init_type = expr_type(init);
        if (init_type != -1 && init_type != is_float) {
            if (is_float && init_type == 0) {
                // int -> float allowed, nothing to do
            } else {
                fprintf(stderr, "Semantic error at line %d: initializer type mismatch for '%s'\n", id->line, id->lexeme);
                semantic_errors = 1;
            }
        }
    }
    ASTNode* n = new_node(NODE_DECL, id->line, id->col);
    n->u.decl.name = my_strdup(id->lexeme);
    n->u.decl.is_float = is_float;
    n->u.decl.init = init;
    return n;
}

ASTNode* parse_assign(Parser* p) {
    Token* id = p->cur;
    if (id->type != ID) { parse_error(p, "expected identifier"); return NULL; }
    p->cur = p->cur->next;
    if (!check(p, OP_ASSIGN)) { parse_error(p, "expected '='"); return NULL; }
    p->cur = p->cur->next;
    ASTNode* expr = parse_expr(p);
    expect(p, DELIM_SEMI, "expected ';' after assignment");
    // SEMANTIC: check that variable exists and types are compatible
    check_assign(id->lexeme, expr, id->line, id->col);
    ASTNode* n = new_node(NODE_ASSIGN, id->line, id->col);
    n->u.assign.name = my_strdup(id->lexeme);
    n->u.assign.expr = expr;
    return n;
}

ASTNode* parse_if(Parser* p) {
    Token* tok = p->cur;
    p->cur = p->cur->next;
    expect(p, DELIM_LPAREN, "expected '(' after if");
    ASTNode* cond = parse_expr(p);
    expect(p, DELIM_RPAREN, "expected ')'");
    ASTNode* then_stmt = parse_stmt(p);
    ASTNode* else_stmt = NULL;
    if (check(p, KW_ELSE)) {
        p->cur = p->cur->next;
        else_stmt = parse_stmt(p);
    }
    ASTNode* n = new_node(NODE_IF, tok->line, tok->col);
    n->u.ifs.cond = cond; n->u.ifs.then = then_stmt; n->u.ifs.els = else_stmt;
    return n;
}

ASTNode* parse_while(Parser* p) {
    Token* tok = p->cur;
    p->cur = p->cur->next;
    expect(p, DELIM_LPAREN, "expected '(' after while");
    ASTNode* cond = parse_expr(p);
    expect(p, DELIM_RPAREN, "expected ')'");
    ASTNode* body = parse_stmt(p);
    ASTNode* n = new_node(NODE_WHILE, tok->line, tok->col);
    n->u.wh.cond = cond; n->u.wh.body = body;
    return n;
}

ASTNode* parse_compound(Parser* p) {
    expect(p, DELIM_LBRACE, "expected '{'");
    ASTNode* list = parse_stmt_list(p);
    expect(p, DELIM_RBRACE, "expected '}'");
    return list;
}

ASTNode* parse_stmt(Parser* p) {
    if (check(p, KW_INT) || check(p, KW_FLOAT)) return parse_decl(p);
    if (check(p, ID)) return parse_assign(p);
    if (check(p, KW_IF)) return parse_if(p);
    if (check(p, KW_WHILE)) return parse_while(p);
    if (check(p, DELIM_LBRACE)) return parse_compound(p);
    parse_error(p, "unexpected token");
    return NULL;
}

ASTNode* parse_stmt_list(Parser* p) {
    ASTNode* head = NULL, *tail = NULL;
    while (p->cur && p->cur->type != TOK_EOF && p->cur->type != DELIM_RBRACE) {
        ASTNode* stmt = parse_stmt(p);
        if (stmt) {
            if (!head) head = stmt;
            else tail->next = stmt;
            tail = stmt;
        }
        if (p->err || semantic_errors) break;   // stop on error
    }
    return head;
}

ASTNode* parse_program(Parser* p) {
    ASTNode* list = parse_stmt_list(p);
    ASTNode* prog = new_node(NODE_PROG, 0, 0);
    prog->next = list;
    return prog;
}

/* AST printing (unchanged) */
void print_ast(ASTNode* n, int indent) {
    if (!n) return;
    for (int i = 0; i < indent; i++) printf("  ");
    switch (n->type) {
        case NODE_PROG: printf("Program\n"); print_ast(n->next, indent); break;
        case NODE_DECL: printf("Declaration (%s %s)\n", n->u.decl.is_float ? "float" : "int", n->u.decl.name); break;
        case NODE_ASSIGN: printf("Assignment: %s = ", n->u.assign.name); print_ast(n->u.assign.expr, 0); printf("\n"); break;
        case NODE_IF:
            printf("If\n");
            print_ast(n->u.ifs.cond, indent + 1);
            print_ast(n->u.ifs.then, indent + 1);
            if (n->u.ifs.els) print_ast(n->u.ifs.els, indent + 1);
            break;
        case NODE_WHILE: printf("While\n"); print_ast(n->u.wh.cond, indent + 1); print_ast(n->u.wh.body, indent + 1); break;
        case NODE_BINOP:
            printf("("); print_ast(n->u.bin.l, 0); printf(" %c ", n->u.bin.op); print_ast(n->u.bin.r, 0); printf(")");
            if (indent == 0) printf("\n");
            break;
        case NODE_ID: printf("%s\n", n->u.id.name); break;
        case NODE_INT: printf("%d\n", n->u.ilit.ival); break;
        case NODE_FLOAT: printf("%f\n", n->u.flit.fval); break;
        default: break;
    }
    if (n->type != NODE_PROG && n->type != NODE_BINOP && n->next && indent == 0)
        print_ast(n->next, indent);
}

int main(int argc, char** argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s file.cmini\n", argv[0]); return 1; }
    Lexer lex;
    lex_init(&lex, argv[1]);
    Token* t;
    do { t = lex_next_token(&lex); } while (t && t->type != TOK_EOF);
    printf("===== TOKENS =====\n");
    int i = 1;
    for (Token* cur = lex.first; cur; cur = cur->next) {
        if (cur->type != TOK_EOF)
            printf("%3d: %-12s '%s' line %d col %d\n", i++, token_names[cur->type], cur->lexeme, cur->line, cur->col);
        else printf("%3d: %-12s (EOF)\n", i++, token_names[cur->type]);
    }
    Parser parser = { lex.first, 0 };
    semantic_errors = 0;
    ASTNode* ast = parse_program(&parser);
    if (parser.err || semantic_errors) {
        printf("\nParsing FAILED (syntax or semantic errors).\n");
        return 1;
    } else {
        printf("\n===== AST =====\n");
        print_ast(ast, 0);
        printf("\n===== SYMBOL TABLE =====\n");
        for (Sym* s = symtab; s; s = s->next) printf("%s : %s\n", s->name, s->is_float ? "float" : "int");
    }
    return 0;
}
