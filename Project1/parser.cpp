#include <iostream>
#include <vector>
#include <string>
#include <stdexcept> // For parser errors
#include <fstream>   // For readFile
#include <sstream>   // For readFile

// This file *requires* you to have "scanner.h" in the same folder.
// "scanner.h" must define:
// 1. enum TokenType { ... };
// 2. struct Token { ... };
// 3. class Scanner { ... };
#include "scanner.h"

using namespace std;

// =============================================================================
// 1. PARSER CLASS
// =============================================================================

class Parser {
private:
    vector<Token> tokens;
    int current = 0;

    // --- Parser Error Class ---
    // A custom exception to throw on a syntax error
    class ParseError : public runtime_error {
    public:
        ParseError(const string& message) : runtime_error(message) {}
    };

    // --- Helper Functions ---
    
    // Peek at the current token without consuming it
    Token peek() {
        return tokens[current];
    }

    // Return the previous token
    Token previous() {
        return tokens[current - 1];
    }

    // Check if we're at the end of the token list
    bool isAtEnd() {
        return peek().type == TOK_EOF;
    }

    // Consume the current token and return it
    Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    // Check if the current token is of a specific type
    bool check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    // Check if the current token is one of several types
    bool check(const vector<TokenType>& types) {
        for (TokenType type : types) {
            if (check(type)) {
                return true;
            }
        }
        return false;
    }

    // If the current token matches one of the types, consume it and return true
    bool match(const vector<TokenType>& types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    // Consume a specific token type, or throw an error
    Token consume(TokenType type, const string& message) {
        if (check(type)) return advance();
        throw error(peek(), message);
    }

    // --- Error Handling ---

    // Create and return a ParseError
    ParseError error(const Token& token, const string& message) {
        string errorMsg = "[Line " + to_string(token.line) + ", Col " + to_string(token.column) +
                          "] Error";
        if (token.type == TOK_EOF) {
            errorMsg += " at end: " + message;
        } else {
            errorMsg += " at '" + token.lexeme + "': " + message;
        }
        return ParseError(errorMsg);
    }

    // Error recovery: advance until we find a statement boundary
    void synchronize() {
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
                case TOK_INTEL:
                case TOK_EVALUATE:
                case TOK_DEPLOY:
                case TOK_MAINTAIN:
                case TOK_RETREAT:
                    return;
                default:
                    // Do nothing, just advance
                    break;
            }
            advance();
        }
    }

    // --- Grammar Rule Functions (Top-Down) ---
    // These functions match your grammar, rule by rule.

    // Program -> DeclarationList EOF
    // (We'll call declaration() in a loop in the public parse() function)

    // Declaration -> IncludeStatement | FunctionDefinition | VariableDeclaration
    void declaration() {
        try {
            if (check(TOK_SUPPLY)) {
                includeStatement();
            } else if (check(TOK_TACTIC)) {
                functionDefinition();
            } else if (check({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
                variableDeclaration();
            } else {
                // If it's none of the above, it's an error.
                throw error(peek(), "Expected a declaration (#supply, tactic, or variable type).");
            }
        } catch (ParseError& e) {
            cerr << e.what() << endl;
            synchronize(); // Recover to the next statement
        }
    }

    // IncludeStatement -> SUPPLY IDENTIFIER
    void includeStatement() {
        consume(TOK_SUPPLY, "Expected '#supply'.");
        consume(TOK_IDENTIFIER, "Expected identifier after '#supply'.");
        // Note: No semicolon in our grammar for #supply (based on tokens)
        // If it needs one, add: consume(TOK_SEMICOLON, "Expected ';'.");
    }

    // VariableDeclaration -> Type IDENTIFIER (ASSIGN Expr)? SEMICOLON
    void variableDeclaration() {
        // The 'Type' token (TROOP, AMMO, etc.) was already checked.
        advance(); // Consume the type token
        
        consume(TOK_IDENTIFIER, "Expected variable name.");

        if (match({TOK_ASSIGN})) {
            expr(); // Parse the initializer expression
        }
        
        consume(TOK_SEMICOLON, "Expected ';' after variable declaration.");
    }

    // FunctionDefinition -> TACTIC (IDENTIFIER | CAMPAIGN) LPAREN ParamList? RPAREN BlockStatement
    void functionDefinition() {
        consume(TOK_TACTIC, "Expected 'tactic'.");
        
        // Handle 'campaign' as a special IDENTIFIER
        if (!match({TOK_IDENTIFIER, TOK_CAMPAIGN})) {
           throw error(peek(), "Expected function name or 'campaign'.");
        }
        
        consume(TOK_LPAREN, "Expected '(' after function name.");
        
        // ParamList? -> Param (COMMA Param)*
        if (!check(TOK_RPAREN)) {
            do {
                // Param -> Type IDENTIFIER
                if (!match({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
                    throw error(peek(), "Expected parameter type.");
                }
                consume(TOK_IDENTIFIER, "Expected parameter name.");
            } while (match({TOK_COMMA}));
        }
        
        consume(TOK_RPAREN, "Expected ')' after parameters.");
        block(); // Parse function body
    }

    // StatementList -> (Statement)*
    // (This is handled by the block() function)

    // BlockStatement -> LBRACE StatementList RBRACE
    void block() {
        consume(TOK_LBRACE, "Expected '{' to begin block.");
        // StatementList
        while (!check(TOK_RBRACE) && !isAtEnd()) {
            statement();
        }
        consume(TOK_RBRACE, "Expected '}' to end block.");
    }

    // Statement -> (all statement types)
    void statement() {
        if (check(TOK_LBRACE)) {
            block();
        } else if (check({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
            variableDeclaration();
        } else if (check(TOK_EVALUATE)) {
            ifStatement();
        } else if (check(TOK_MAINTAIN)) {
            whileStatement();
        } else if (check(TOK_DEPLOY)) {
            forStatement();
        } else if (check(TOK_BRIEF)) {
            outputStatement();
        } else if (check(TOK_INTEL)) {
            inputStatement();
        } else if (check(TOK_RETREAT)) {
            returnStatement();
        } else if (check(TOK_ABORT)) {
            breakStatement();
        } else {
            // Default to an expression statement (e.g., assignment or function call)
            expressionStatement();
        }
    }

    // IfStatement -> EVALUATE LPAREN Expr RPAREN BlockStatement ElsePart?
    void ifStatement() {
        consume(TOK_EVALUATE, "Expected 'evaluate'.");
        consume(TOK_LPAREN, "Expected '(' after 'evaluate'.");
        expr();
        consume(TOK_RPAREN, "Expected ')' after condition.");
        block();

        // ElsePart? -> ADJUST IfStatement | ADJUST BlockStatement
        if (match({TOK_ADJUST})) {
            if (check(TOK_EVALUATE)) {
                ifStatement(); // Handle 'adjust evaluate' (else if)
            } else {
                block(); // Handle 'adjust' (else)
            }
        }
    }

    // WhileStatement -> MAINTAIN LPAREN Expr RPAREN BlockStatement
    void whileStatement() {
        consume(TOK_MAINTAIN, "Expected 'maintain'.");
        consume(TOK_LPAREN, "Expected '(' after 'maintain'.");
        expr();
        consume(TOK_RPAREN, "Expected ')' after condition.");
        block();
    }

    // ForStatement -> DEPLOY LPAREN ForInit ForCond ForUpdate RPAREN BlockStatement
    void forStatement() {
        consume(TOK_DEPLOY, "Expected 'deploy'.");
        consume(TOK_LPAREN, "Expected '(' after 'deploy'.");

        // ForInit -> VariableDeclaration | ExpressionStatement | SEMICOLON
        if (match({TOK_SEMICOLON})) {
            // No initializer
        } else if (check({TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS})) {
            variableDeclaration();
        } else {
            expressionStatement();
        }

        // ForCond -> Expr? SEMICOLON
        if (!check(TOK_SEMICOLON)) {
            expr();
        }
        consume(TOK_SEMICOLON, "Expected ';' after loop condition.");

        // ForUpdate -> Expr?
        if (!check(TOK_RPAREN)) {
            expr();
        }
        consume(TOK_RPAREN, "Expected ')' after for clauses.");

        block();
    }

    // OutputStatement -> BRIEF Expr SEMICOLON
    void outputStatement() {
        consume(TOK_BRIEF, "Expected 'brief'.");
        expr();
        consume(TOK_SEMICOLON, "Expected ';' after 'brief' statement.");
    }

    // InputStatement -> INTEL IDENTIFIER SEMICOLON
    void inputStatement() {
        consume(TOK_INTEL, "Expected 'intel'.");
        consume(TOK_IDENTIFIER, "Expected identifier after 'intel'.");
        consume(TOK_SEMICOLON, "Expected ';' after 'intel' statement.");
    }

    // ReturnStatement -> RETREAT Expr? SEMICOLON
    void returnStatement() {
        consume(TOK_RETREAT, "Expected 'retreat'.");
        if (!check(TOK_SEMICOLON)) {
            expr();
        }
        consume(TOK_SEMICOLON, "Expected ';' after 'retreat' statement.");
    }

    // BreakStatement -> ABORT SEMICOLON
    void breakStatement() {
        consume(TOK_ABORT, "Expected 'abort'.");
        consume(TOK_SEMICOLON, "Expected ';' after 'abort'.");
    }
    
    // ExpressionStatement -> Expr SEMICOLON
    void expressionStatement() {
        expr();
        consume(TOK_SEMICOLON, "Expected ';' after expression.");
    }

    // --- Expression Parsing (by precedence) ---
    // See TacticLang.grammar for the precedence table

    // Expr -> LogicalOr
    void expr() {
        logicalOr();
    }

    // LogicalOr -> LogicalAnd (OR LogicalAnd)*//a*5+2||2*3&&5>2<2
    void logicalOr() {
        logicalAnd();
        while (match({TOK_OR})) {
            logicalAnd();
        }
    }

    // LogicalAnd -> Equality (AND Equality)*
    void logicalAnd() {
        equality();
        while (match({TOK_AND})) {
            equality();
        }
    }

    // Equality -> Relational ( (EQUAL | NOT_EQUAL) Relational )*
    void equality() {
        relational();
        while (match({TOK_EQUAL, TOK_NOT_EQUAL})) {
            relational();
        }
    }

    // Relational -> Additive ( (LESS | GREATER | LESS_EQUAL | GREATER_EQUAL) Additive )*
    void relational() {
        additive();
        while (match({TOK_LESS, TOK_GREATER, TOK_LESS_EQUAL, TOK_GREATER_EQUAL})) {
            additive();
        }
    }

    // Additive -> Multiplicative ( (PLUS | MINUS) Multiplicative )*
    void additive() {
        multiplicative();
        while (match({TOK_PLUS, TOK_MINUS})) {
            multiplicative();
        }
    }

    // Multiplicative -> Unary ( (MULTIPLY | DIVIDE | MODULO) Unary )*
    void multiplicative() {
        unary();
        while (match({TOK_MULTIPLY, TOK_DIVIDE, TOK_MODULO})) {
            unary();
        }
    }

    // Unary -> (NOT | MINUS) Unary | Primary
    void unary() {
        if (match({TOK_NOT, TOK_MINUS})) {
            unary(); // Recursive call for stacked unary ops (e.g., !!true)
        } else {
            primary();
        }
    }

    // Primary -> ...
    void primary() {
        if (match({TOK_INTEGER, TOK_DOUBLE, TOK_STRING, TOK_TRUE, TOK_FALSE})) {
            return; // Literal value, we're done
        }

        // Check for function call: IDENTIFIER LPAREN ...
        if (check(TOK_IDENTIFIER) && tokens[current + 1].type == TOK_LPAREN) {
            advance(); // consume IDENTIFIER
            advance(); // consume LPAREN
            // ArgList? -> Expr (COMMA Expr)*
            if (!check(TOK_RPAREN)) {
                do {
                    expr();
                } while (match({TOK_COMMA}));
            }
            consume(TOK_RPAREN, "Expected ')' after function call arguments.");
            return;
        }
        
        // Check for assignment: IDENTIFIER ASSIGN ...
        if (check(TOK_IDENTIFIER) && tokens[current + 1].type == TOK_ASSIGN) {
            advance(); // consume IDENTIFIER
            advance(); // consume ASSIGN
            expr(); // Parse the right-hand side
            return;
        }

        // Must be a simple variable
        if (match({TOK_IDENTIFIER})) {
            return; 
        }

        // Grouping: ( Expr )
        if (match({TOK_LPAREN})) {
            expr(); // Parse the expression inside the parentheses
            consume(TOK_RPAREN, "Expected ')' after expression.");
            return;
        }

        // If we get here, no rule matched.
        throw error(peek(), "Expected expression (literal, variable, grouping).");
    }

public:
    // --- Public Interface ---
    
    Parser(const vector<Token>& tokens) : tokens(tokens) {}

    // Public entry point to start parsing
    void parse() {
        try {
            // Program -> DeclarationList EOF
            while (!isAtEnd()) {
                declaration();
            }
            cout << "Parsing complete. Syntax is valid." << endl;
        } catch (ParseError& e) {
            // Error was already printed by synchronize() or declaration()
            // We just stop the parse.
            cerr << "Parsing failed." << endl;
        }
    }
};

// =============================================================================
// 2. UTILITY FUNCTIONS
// =============================================================================

string readFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Error: Could not open file '" << filepath << "'" << endl;
        return "";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

// =============================================================================
// 3. MAIN FUNCTION
// =============================================================================

int main() {
    // File path from your original code
    string filepath = "D:\\Faculty\\Y4\\S1\\Compiler\\TacticLang\\soldier.tac";
    
    cout << "TacticLang Compiler" << endl;
    cout << "===================" << endl;
    cout << "Reading file: " << filepath << endl;

    // --- 1. Scanning ---
    string sourceCode = readFile(filepath);
    if (sourceCode.empty()) {
        cerr << "Error: Source file is empty or could not be read." << endl;
        return 1;
    }

    cout << "File read successfully. Scanning..." << endl;
    Scanner scanner(sourceCode);
    vector<Token> tokens = scanner.scanTokens();

    // Check for scanner errors
    int errorCount = 0;
    for (const Token& token : tokens) {
        if (token.type == TOK_ERROR) {
            cerr << "Scanner Error: " << token.lexeme << " at line " << token.line << endl;
            errorCount++;
        }
    }

    if (errorCount > 0) {
        cerr << "Scanning failed with " << errorCount << " errors." << endl;
        return 1;
    }
    
    cout << "Scanning complete. " << tokens.size() << " tokens found." << endl << endl;

    // --- 2. Parsing ---
    cout << "Parsing..." << endl;
    Parser parser(tokens);
    parser.parse(); // This will print "Parsing complete" or any errors.

    cout << endl << "Compiler run finished." << endl;

    return 0;
}