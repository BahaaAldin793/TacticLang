// parser.cpp
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <algorithm>
#include "scanner.h" // يعطي: Token, TokenType, scanFile(...)
using namespace std;

// --- AST node ---
struct AST {
    string kind;                 // e.g. "Program", "VarDecl", "FuncDecl", "BinaryExpr"
    string value;                // optional (identifier name, literal text, operator)
    vector<shared_ptr<AST>> ch;  // children

    AST(const string& k, const string& v = "") : kind(k), value(v) {}
};

static void printAST(const shared_ptr<AST>& node, int indent = 0) {
    if (!node) return;
    cout << string(indent, ' ');
    cout << node->kind;
    if (!node->value.empty()) cout << " [" << node->value << "]";
    cout << "\n";
    for (auto &c : node->ch) printAST(c, indent + 2);
}

// --- Parser ---
class Parser {
    vector<Token> tokens;
    int current = 0;
    vector<string> errors;

public:
    Parser(const vector<Token>& toks) : tokens(toks), current(0) {}

    shared_ptr<AST> parseProgram() {
        auto root = make_shared<AST>("Program");
        while (!isAtEnd()) {
            try {
                auto d = declaration();
                if (d) root->ch.push_back(d);
            } catch (...) {
                // synchronization already attempted in declaration()
            }
        }
        return root;
    }

    // ----------------- Utilities -----------------
private:
    Token peek() const { return tokens[current]; }
    Token previous() const { return tokens[current - 1]; }
    bool isAtEnd() const { return peek().type == TOK_EOF; }

    Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    bool check(TokenType t) const {
        if (isAtEnd()) return false;
        return peek().type == t;
    }

    bool match(initializer_list<TokenType> list) {
        for (auto t : list)
            if (check(t)) { advance(); return true; }
        return false;
    }

    bool match(TokenType t) {
        if (check(t)) { advance(); return true; }
        return false;
    }

    Token consume(TokenType t, const string& msg) {
        if (check(t)) return advance();
        error(peek(), msg);
        throw syncPoint();
    }

    // Error + sync helpers
    struct syncPoint {}; // used for control flow (exception) to indicate parse error and recovery

    void error(const Token& tok, const string& msg) {
        stringstream ss;
        ss << "Syntax Error at line " << tok.line << ", column " << tok.column
           << ": " << msg << " (token: " << tok.lexeme << ")";
        errors.push_back(ss.str());
        cerr << ss.str() << "\n";
    }

    void synchronize() {
        // advance until probable statement start or next semicolon/brace
        advance();
        while (!isAtEnd()) {
            if (previous().type == TOK_SEMICOLON) return;
            switch (peek().type) {
                case TOK_TACTIC:
                case TOK_TROOP:
                case TOK_AMMO:
                case TOK_CODENAME:
                case TOK_STATUS:
                case TOK_BRIEF:
                case TOK_EVALUATE:
                case TOK_ADJUST:
                case TOK_DEPLOY:
                case TOK_RETREAT:
                case TOK_ABORT:
                case TOK_SUPPLY:
                case TOK_IDENTIFIER:
                    return;
                default:
                    break;
            }
            advance();
        }
    }

    // ----------------- Grammar -----------------
    // program -> declaration*
    // declaration -> varDecl | funcDecl | statement

    shared_ptr<AST> declaration() {
        try {
            if (match({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
                Token typeTok = previous();
                return varDecl(typeTok);
            }
            if (match(TOK_TACTIC)) {
                return funcDecl();
            }
            // directives like #supply maybe tokenized as TOK_SUPPLY (handled as statement)
            return statement();
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // varDecl -> TYPE IDENTIFIER ( '=' expression )? ';'
    shared_ptr<AST> varDecl(const Token& typeTok) {
        try {
            Token name = consume(TOK_IDENTIFIER, "Expected variable name after type");
            auto node = make_shared<AST>("VarDecl");
            node->value = typeTok.lexeme + " " + name.lexeme;
            if (match(TOK_ASSIGN)) {
                auto expr = expression();
                node->ch.push_back(make_shared<AST>("Init", ""));
                node->ch.back()->ch.push_back(expr);
            }
            consume(TOK_SEMICOLON, "Expected ';' after variable declaration");
            return node;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // funcDecl -> 'tactic' IDENTIFIER '(' params ')' block
    shared_ptr<AST> funcDecl() {
        try {
            Token name = consume(TOK_IDENTIFIER, "Expected function name after 'tactic'");
            consume(TOK_LPAREN, "Expected '(' after function name");
            auto paramsNode = make_shared<AST>("Params");
            if (!check(TOK_RPAREN)) {
                for (;;) {
                    // Expect type? in your language parameters like "troop kills" or maybe only types appear.
                    if (match({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
                        Token typeTok = previous();
                        Token pname = consume(TOK_IDENTIFIER, "Expected parameter name");
                        auto pnode = make_shared<AST>("Param", typeTok.lexeme + " " + pname.lexeme);
                        paramsNode->ch.push_back(pnode);
                    } else if (check(TOK_IDENTIFIER)) {
                        // fallback: accept bare identifier param
                        Token pname = advance();
                        auto pnode = make_shared<AST>("Param", pname.lexeme);
                        paramsNode->ch.push_back(pnode);
                    } else {
                        error(peek(), "Expected parameter declaration");
                        throw syncPoint();
                    }

                    if (match(TOK_COMMA)) continue;
                    break;
                }
            }
            consume(TOK_RPAREN, "Expected ')' after parameters");
            auto body = block(); // expects '{' ... '}'
            auto node = make_shared<AST>("FuncDecl", name.lexeme);
            node->ch.push_back(paramsNode);
            node->ch.push_back(body);
            return node;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // statement -> block | exprStmt | briefStmt | evaluateStmt | adjustStmt | deployStmt | retreatStmt | functionCallStmt | directive
    shared_ptr<AST> statement() {
        try {
            if (match(TOK_LBRACE)) {
                // we've already advanced over '{'
                current--; // step back so block() will consume '{'
                return block();
            }
            if (match(TOK_BRIEF)) return briefStmt();
            if (match(TOK_EVALUATE)) return evaluateStmt();
            if (match(TOK_ADJUST)) return adjustStmt();
            if (match(TOK_DEPLOY)) return deployStmt();
            if (match(TOK_RETREAT)) return retreatStmt();
            if (match(TOK_SUPPLY)) {
                // #supply directive token: treat as directive statement, may have identifier following
                auto node = make_shared<AST>("Directive", previous().lexeme);
                if (check(TOK_IDENTIFIER)) {
                    node->ch.push_back(make_shared<AST>("Arg", advance().lexeme));
                }
                // optional semicolon (some examples had no semicolon after #supply) — we will accept optional semicolon
                if (check(TOK_SEMICOLON)) advance();
                return node;
            }

            // expression starting with identifier might be assignment or function call
            if (check(TOK_IDENTIFIER)) {
                // need to lookahead
                if (peekNextType() == TOK_LPAREN) {
                    auto fc = functionCall();
                    consume(TOK_SEMICOLON, "Expected ';' after function call");
                    return fc;
                } else {
                    // could be assignment
                    auto expr = expression();
                    consume(TOK_SEMICOLON, "Expected ';' after expression");
                    return expr;
                }
            }

            // fallback: expression statement
            auto expr = expression();
            consume(TOK_SEMICOLON, "Expected ';' after expression");
            return expr;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // briefStmt -> 'brief' STRING ';'
    shared_ptr<AST> briefStmt() {
        try {
            if (check(TOK_STRING)) {
                Token s = advance();
                consume(TOK_SEMICOLON, "Expected ';' after brief statement");
                auto n = make_shared<AST>("Brief", s.lexeme);
                return n;
            } else {
                error(peek(), "Expected string after 'brief'");
                throw syncPoint();
            }
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // evaluateStmt -> 'evaluate' '(' expression ')' block (optionally 'adjust' / else part is handled as separate adjust blocks)
    shared_ptr<AST> evaluateStmt() {
        try {
            consume(TOK_LPAREN, "Expected '(' after 'evaluate'");
            auto cond = expression();
            consume(TOK_RPAREN, "Expected ')' after condition");
            auto thenBlock = block();
            auto node = make_shared<AST>("Evaluate");
            node->ch.push_back(cond);
            node->ch.push_back(thenBlock);
            // check for 'adjust' which often follows evaluate in examples
            if (match(TOK_ADJUST)) {
                auto elseBlock = block();
                node->ch.push_back(elseBlock);
            }
            return node;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // adjustStmt -> 'adjust' ( optionally 'evaluate' '(' cond ')' )? block
    shared_ptr<AST> adjustStmt() {
        try {
            // if adjust evaluate (cond) { ... } handled by evaluateStmt when starting with evaluate
            if (match(TOK_EVALUATE)) return evaluateStmt();
            auto b = block();
            auto node = make_shared<AST>("Adjust");
            node->ch.push_back(b);
            return node;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // deployStmt -> 'deploy' '(' ... ')' block
    shared_ptr<AST> deployStmt() {
        try {
            consume(TOK_LPAREN, "Expected '(' after 'deploy'");
            // in examples deploy sometimes has full for-like syntax or just an expression
            // We'll parse a full expression sequence until ')' (a bit permissive)
            vector<shared_ptr<AST>> parts;
            if (!check(TOK_RPAREN)) {
                // gather tokens until RPAREN as a mini-statement/expression list
                parts.push_back(expression());
                while (match(TOK_COMMA)) {
                    parts.push_back(expression());
                }
            }
            consume(TOK_RPAREN, "Expected ')' after deploy parameters");
            auto body = block();
            auto node = make_shared<AST>("Deploy");
            for (auto &p : parts) node->ch.push_back(p);
            node->ch.push_back(body);
            return node;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // retreatStmt -> 'retreat' expression ';'
    shared_ptr<AST> retreatStmt() {
        try {
            auto expr = expression();
            consume(TOK_SEMICOLON, "Expected ';' after retreat statement");
            auto node = make_shared<AST>("Retreat");
            node->ch.push_back(expr);
            return node;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // block -> '{' declaration* '}'
    shared_ptr<AST> block() {
        try {
            consume(TOK_LBRACE, "Expected '{' to start block");
            auto node = make_shared<AST>("Block");
            while (!check(TOK_RBRACE) && !isAtEnd()) {
                auto d = declaration();
                if (d) node->ch.push_back(d);
            }
            consume(TOK_RBRACE, "Expected '}' after block");
            return node;
        } catch (syncPoint&) {
            synchronize();
            return nullptr;
        }
    }

    // function call: IDENTIFIER '(' args ')'
    shared_ptr<AST> functionCall() {
        // assume current token is IDENTIFIER
        Token name = advance();
        consume(TOK_LPAREN, "Expected '(' after function name");
        auto node = make_shared<AST>("Call", name.lexeme);
        if (!check(TOK_RPAREN)) {
            node->ch.push_back(expression());
            while (match(TOK_COMMA)) node->ch.push_back(expression());
        }
        consume(TOK_RPAREN, "Expected ')' after call arguments");
        return node;
    }

    TokenType peekNextType() {
        if (current + 1 >= (int)tokens.size()) return TOK_EOF;
        return tokens[current + 1].type;
    }

    // ----------------- Expressions -----------------
    // expression -> or
    // or -> and ( "||" and )*
    // and -> equality ( "&&" equality )*
    // equality -> comparison ( (== | !=) comparison )*
    // comparison -> term ( (<|>|<=|>=) term )*
    // term -> factor ( (+ | -) factor )*
    // factor -> unary ( (* | / | %) unary )*
    // unary -> ( '!' | '-' ) unary | call
    // call -> primary ( '(' args ')' )*
    // primary -> NUMBER | STRING | IDENTIFIER | '(' expression ')'

    shared_ptr<AST> expression() {
        return parseOr();
    }

    shared_ptr<AST> parseOr() {
        auto node = parseAnd();
        while (match(TOK_OR)) {
            Token op = previous();
            auto right = parseAnd();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node);
            n->ch.push_back(right);
            node = n;
        }
        return node;
    }

    shared_ptr<AST> parseAnd() {
        auto node = parseEquality();
        while (match(TOK_AND)) {
            Token op = previous();
            auto right = parseEquality();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node);
            n->ch.push_back(right);
            node = n;
        }
        return node;
    }

    shared_ptr<AST> parseEquality() {
        auto node = parseComparison();
        while (match({TOK_EQUAL, TOK_NOT_EQUAL})) {
            Token op = previous();
            auto right = parseComparison();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node);
            n->ch.push_back(right);
            node = n;
        }
        return node;
    }

    shared_ptr<AST> parseComparison() {
        auto node = parseTerm();
        while (match({TOK_LESS, TOK_GREATER, TOK_LESS_EQUAL, TOK_GREATER_EQUAL})) {
            Token op = previous();
            auto right = parseTerm();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node);
            n->ch.push_back(right);
            node = n;
        }
        return node;
    }

    shared_ptr<AST> parseTerm() {
        auto node = parseFactor();
        while (match({TOK_PLUS, TOK_MINUS})) {
            Token op = previous();
            auto right = parseFactor();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node);
            n->ch.push_back(right);
            node = n;
        }
        return node;
    }

    shared_ptr<AST> parseFactor() {
        auto node = parseUnary();
        while (match({TOK_MULTIPLY, TOK_DIVIDE, TOK_MODULO})) {
            Token op = previous();
            auto right = parseUnary();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node);
            n->ch.push_back(right);
            node = n;
        }
        return node;
    }

    shared_ptr<AST> parseUnary() {
        if (match({TOK_NOT, TOK_MINUS})) {
            Token op = previous();
            auto right = parseUnary();
            auto n = make_shared<AST>("UnaryExpr", op.lexeme);
            n->ch.push_back(right);
            return n;
        }
        return parseCall();
    }

    shared_ptr<AST> parseCall() {
        auto node = parsePrimary();
        // support chained calls like f()(...) but simple approach:
        while (true) {
            if (match(TOK_LPAREN)) {
                // it's a call on previous node if it's an identifier or an expression that yields a callable
                auto callNode = make_shared<AST>("Call");
                // if node is identifier style, set name
                if (node->kind == "Identifier") callNode->value = node->value;
                // parse args
                if (!check(TOK_RPAREN)) {
                    callNode->ch.push_back(expression());
                    while (match(TOK_COMMA)) callNode->ch.push_back(expression());
                }
                consume(TOK_RPAREN, "Expected ')' after call arguments");
                node = callNode;
            } else break;
        }
        return node;
    }

    shared_ptr<AST> parsePrimary() {
        if (match(TOK_INTEGER)) {
            return make_shared<AST>("Integer", previous().lexeme);
        }
        if (match(TOK_DOUBLE)) {
            return make_shared<AST>("Double", previous().lexeme);
        }
        if (match(TOK_STRING)) {
            return make_shared<AST>("String", previous().lexeme);
        }
        if (match(TOK_TRUE)) {
            return make_shared<AST>("Bool", "true");
        }
        if (match(TOK_FALSE)) {
            return make_shared<AST>("Bool", "false");
        }
        if (match(TOK_IDENTIFIER)) {
            return make_shared<AST>("Identifier", previous().lexeme);
        }
        if (match(TOK_LPAREN)) {
            auto expr = expression();
            consume(TOK_RPAREN, "Expected ')' after expression");
            auto group = make_shared<AST>("Group", "");
            group->ch.push_back(expr);
            return group;
        }
        // no match
        error(peek(), "Expected expression");
        throw syncPoint();
    }

};

// utility to help one-liner adding child and returning node (for Group case)
static shared_ptr<AST> push_child_and_return(shared_ptr<AST> parent, shared_ptr<AST> child) {
    parent->ch.push_back(child);
    return parent;
}
// provide a small helper via lambda-like free function usage
// because method chaining in parsePrimary for Group is awkward.
shared_ptr<AST> AST_push_child_and_return_wrapper(shared_ptr<AST> parent, shared_ptr<AST> child) {
    return push_child_and_return(parent, child);
}

// Since I used an inline attempt to push child and return, add a tiny helper on AST to use:
namespace {
    struct ASTHelpers {
        static shared_ptr<AST> push_and_return(shared_ptr<AST> parent, shared_ptr<AST> child) {
            parent->ch.push_back(child);
            return parent;
        }
    };
}

// We'll adjust parsePrimary's group handling by re-defining Parser with corrected group handling.
// (To avoid deep refactor here: implement correct version of parsePrimary below by re-opening class methods)

// Because above we can't mutate easily, let's instead provide a new Parser2 class with corrected simple implementation.

class Parser2 {
    vector<Token> tokens;
    int current = 0;
    vector<string> errors;
public:
    Parser2(const vector<Token>& toks) : tokens(toks) {}
    shared_ptr<AST> parseProgram() {
        auto root = make_shared<AST>("Program");
        while (!isAtEnd()) {
            auto d = declaration();
            if (d) root->ch.push_back(d);
        }
        return root;
    }

private:
    Token peek() const { return tokens[current]; }
    Token previous() const { return tokens[current - 1]; }
    bool isAtEnd() const { return peek().type == TOK_EOF; }
    Token advance() { if (!isAtEnd()) current++; return previous(); }
    bool check(TokenType t) const { if (isAtEnd()) return false; return peek().type == t; }
    bool match(initializer_list<TokenType> list) {
        for (auto t : list) if (check(t)) { advance(); return true; } return false;
    }
    bool match(TokenType t) { if (check(t)) { advance(); return true; } return false; }
    Token consume(TokenType t, const string& msg) {
        if (check(t)) return advance();
        error(peek(), msg);
        throw 1;
    }
    void error(const Token& tok, const string& msg) {
        stringstream ss; ss << "Syntax Error at line " << tok.line << ", column " << tok.column
                            << ": " << msg << " (token: " << tok.lexeme << ")";
        cerr << ss.str() << "\n";
        errors.push_back(ss.str());
    }
    void synchronize() {
        advance();
        while (!isAtEnd()) {
            if (previous().type == TOK_SEMICOLON) return;
            switch (peek().type) {
                case TOK_TACTIC: case TOK_TROOP: case TOK_AMMO: case TOK_CODENAME: case TOK_STATUS:
                case TOK_BRIEF: case TOK_EVALUATE: case TOK_ADJUST: case TOK_DEPLOY: case TOK_RETREAT:
                case TOK_ABORT: case TOK_SUPPLY: case TOK_IDENTIFIER:
                    return;
                default: break;
            }
            advance();
        }
    }

    // Simple declaration & statements similar to previous but slightly trimmed
    shared_ptr<AST> declaration() {
        try {
            if (match({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
                Token typeTok = previous();
                return varDecl(typeTok);
            }
            if (match(TOK_TACTIC)) return funcDecl();
            return statement();
        } catch (int) {
            synchronize();
            return nullptr;
        }
    }

    shared_ptr<AST> varDecl(const Token& typeTok) {
        Token name = consume(TOK_IDENTIFIER, "Expected variable name after type");
        auto node = make_shared<AST>("VarDecl", typeTok.lexeme + " " + name.lexeme);
        if (match(TOK_ASSIGN)) {
            auto init = expression();
            node->ch.push_back(make_shared<AST>("Init"));
            node->ch.back()->ch.push_back(init);
        }
        consume(TOK_SEMICOLON, "Expected ';' after variable declaration");
        return node;
    }

    shared_ptr<AST> funcDecl() {
        Token name = consume(TOK_IDENTIFIER, "Expected function name after 'tactic'");
        consume(TOK_LPAREN, "Expected '(' after function name");
        auto paramsNode = make_shared<AST>("Params");
        if (!check(TOK_RPAREN)) {
            for (;;) {
                if (match({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
                    Token typeTok = previous();
                    Token pname = consume(TOK_IDENTIFIER, "Expected parameter name");
                    paramsNode->ch.push_back(make_shared<AST>("Param", typeTok.lexeme + " " + pname.lexeme));
                } else {
                    Token pname = consume(TOK_IDENTIFIER, "Expected parameter name");
                    paramsNode->ch.push_back(make_shared<AST>("Param", pname.lexeme));
                }
                if (!match(TOK_COMMA)) break;
            }
        }
        consume(TOK_RPAREN, "Expected ')' after parameters");
        auto body = block();
        auto node = make_shared<AST>("FuncDecl", name.lexeme);
        node->ch.push_back(paramsNode);
        node->ch.push_back(body);
        return node;
    }

    shared_ptr<AST> statement() {
        if (match(TOK_LBRACE)) { current--; return block(); }
        if (match(TOK_BRIEF)) return briefStmt();
        if (match(TOK_EVALUATE)) return evaluateStmt();
        if (match(TOK_ADJUST)) return adjustStmt();
        if (match(TOK_DEPLOY)) return deployStmt();
        if (match(TOK_RETREAT)) return retreatStmt();
        if (match(TOK_SUPPLY)) {
            auto node = make_shared<AST>("Directive", previous().lexeme);
            if (check(TOK_IDENTIFIER)) node->ch.push_back(make_shared<AST>("Arg", advance().lexeme));
            if (check(TOK_SEMICOLON)) advance();
            return node;
        }
        if (check(TOK_IDENTIFIER) && peekNextType() == TOK_LPAREN) {
            auto call = functionCall();
            consume(TOK_SEMICOLON, "Expected ';' after function call");
            return call;
        }
        auto expr = expression();
        consume(TOK_SEMICOLON, "Expected ';' after expression");
        return expr;
    }

    shared_ptr<AST> briefStmt() {
        Token s = consume(TOK_STRING, "Expected string after 'brief'");
        consume(TOK_SEMICOLON, "Expected ';' after brief statement");
        return make_shared<AST>("Brief", s.lexeme);
    }

    shared_ptr<AST> evaluateStmt() {
        consume(TOK_LPAREN, "Expected '(' after 'evaluate'");
        auto cond = expression();
        consume(TOK_RPAREN, "Expected ')' after condition");
        auto thenBlock = block();
        auto node = make_shared<AST>("Evaluate");
        node->ch.push_back(cond);
        node->ch.push_back(thenBlock);
        if (match(TOK_ADJUST)) node->ch.push_back(block());
        return node;
    }

    shared_ptr<AST> adjustStmt() { return block(); }

    shared_ptr<AST> deployStmt() {
        consume(TOK_LPAREN, "Expected '(' after 'deploy'");
        vector<shared_ptr<AST>> parts;
        if (!check(TOK_RPAREN)) {
            parts.push_back(expression());
            while (match(TOK_COMMA)) parts.push_back(expression());
        }
        consume(TOK_RPAREN, "Expected ')' after deploy parameters");
        auto body = block();
        auto n = make_shared<AST>("Deploy");
        for (auto &p : parts) n->ch.push_back(p);
        n->ch.push_back(body);
        return n;
    }

    shared_ptr<AST> retreatStmt() {
        auto expr = expression();
        consume(TOK_SEMICOLON, "Expected ';' after retreat");
        auto n = make_shared<AST>("Retreat");
        n->ch.push_back(expr);
        return n;
    }

    shared_ptr<AST> block() {
        consume(TOK_LBRACE, "Expected '{' to start block");
        auto n = make_shared<AST>("Block");
        while (!check(TOK_RBRACE) && !isAtEnd()) n->ch.push_back(declaration());
        consume(TOK_RBRACE, "Expected '}' after block");
        return n;
    }

    shared_ptr<AST> functionCall() {
        Token name = consume(TOK_IDENTIFIER, "Expected function name");
        consume(TOK_LPAREN, "Expected '(' after function name");
        auto node = make_shared<AST>("Call", name.lexeme);
        if (!check(TOK_RPAREN)) {
            node->ch.push_back(expression());
            while (match(TOK_COMMA)) node->ch.push_back(expression());
        }
        consume(TOK_RPAREN, "Expected ')' after call arguments");
        return node;
    }

    TokenType peekNextType() const {
        if (current + 1 >= (int)tokens.size()) return TOK_EOF;
        return tokens[current + 1].type;
    }

    // Expressions (same precedence chain)
    shared_ptr<AST> expression() { return parseOr(); }

    shared_ptr<AST> parseOr() {
        auto node = parseAnd();
        while (match(TOK_OR)) {
            Token op = previous();
            auto right = parseAnd();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node); n->ch.push_back(right);
            node = n;
        }
        return node;
    }
    shared_ptr<AST> parseAnd() {
        auto node = parseEquality();
        while (match(TOK_AND)) {
            Token op = previous();
            auto right = parseEquality();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node); n->ch.push_back(right);
            node = n;
        }
        return node;
    }
    shared_ptr<AST> parseEquality() {
        auto node = parseComparison();
        while (match({TOK_EQUAL, TOK_NOT_EQUAL})) {
            Token op = previous();
            auto right = parseComparison();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node); n->ch.push_back(right);
            node = n;
        }
        return node;
    }
    shared_ptr<AST> parseComparison() {
        auto node = parseTerm();
        while (match({TOK_LESS, TOK_GREATER, TOK_LESS_EQUAL, TOK_GREATER_EQUAL})) {
            Token op = previous();
            auto right = parseTerm();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node); n->ch.push_back(right);
            node = n;
        }
        return node;
    }
    shared_ptr<AST> parseTerm() {
        auto node = parseFactor();
        while (match({TOK_PLUS, TOK_MINUS})) {
            Token op = previous();
            auto right = parseFactor();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node); n->ch.push_back(right);
            node = n;
        }
        return node;
    }
    shared_ptr<AST> parseFactor() {
        auto node = parseUnary();
        while (match({TOK_MULTIPLY, TOK_DIVIDE, TOK_MODULO})) {
            Token op = previous();
            auto right = parseUnary();
            auto n = make_shared<AST>("BinaryExpr", op.lexeme);
            n->ch.push_back(node); n->ch.push_back(right);
            node = n;
        }
        return node;
    }
    shared_ptr<AST> parseUnary() {
        if (match({TOK_NOT, TOK_MINUS})) {
            Token op = previous();
            auto right = parseUnary();
            auto n = make_shared<AST>("UnaryExpr", op.lexeme);
            n->ch.push_back(right); return n;
        }
        return parseCall();
    }
    shared_ptr<AST> parseCall() {
        auto node = parsePrimary();
        while (true) {
            if (match(TOK_LPAREN)) {
                auto call = make_shared<AST>("Call");
                // name if identifier
                if (node->kind == "Identifier") call->value = node->value;
                if (!check(TOK_RPAREN)) {
                    call->ch.push_back(expression());
                    while (match(TOK_COMMA)) call->ch.push_back(expression());
                }
                consume(TOK_RPAREN, "Expected ')' after call arguments");
                node = call;
            } else break;
        }
        return node;
    }
    shared_ptr<AST> parsePrimary() {
        if (match(TOK_INTEGER)) return make_shared<AST>("Integer", previous().lexeme);
        if (match(TOK_DOUBLE)) return make_shared<AST>("Double", previous().lexeme);
        if (match(TOK_STRING)) return make_shared<AST>("String", previous().lexeme);
        if (match(TOK_TRUE)) return make_shared<AST>("Bool", "true");
        if (match(TOK_FALSE)) return make_shared<AST>("Bool", "false");
        if (match(TOK_IDENTIFIER)) return make_shared<AST>("Identifier", previous().lexeme);
        if (match(TOK_LPAREN)) {
            auto expr = expression();
            consume(TOK_RPAREN, "Expected ')' after expression");
            auto g = make_shared<AST>("Group");
            g->ch.push_back(expr);
            return g;
        }
        error(peek(), "Expected expression");
        throw 1;
    }
};

// -------------------- Main --------------------
int main(int argc, char** argv) {
    string filepath = "soldier.tac";
    if (argc >= 2) filepath = argv[1];

    vector<Token> tokens = scanFile(filepath);
    // optional: print tokens (debug)
    // for (auto &t : tokens) cout << t.lexeme << " (" << t.line << ":" << t.column << ")\n";

    Parser2 parser(tokens);
    auto ast = parser.parseProgram();

    cout << "===================== Parse Tree =====================\n";
    printAST(ast);
    cout << "===================== End of Tree =====================\n";

    cout << "\nParsing finished. Check errors (if any) printed above.\n";
    return 0;
}
