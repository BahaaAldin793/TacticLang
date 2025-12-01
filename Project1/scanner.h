#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <vector>
#include <map>

using namespace std;

// =============================================================================
// 1. TOKEN DEFINITIONS
// =============================================================================

// Token types
enum TokenType {
    // Keywords
    TOK_CAMPAIGN, TOK_TACTIC, TOK_TROOP, TOK_AMMO, TOK_CODENAME, TOK_STATUS,
    TOK_BRIEF, TOK_INTEL, TOK_EVALUATE, TOK_ADJUST, TOK_MAINTAIN, TOK_DEPLOY,
    TOK_RETREAT, TOK_ABORT, TOK_SUPPLY,

    // Literals
    TOK_INTEGER, TOK_DOUBLE, TOK_STRING, TOK_TRUE, TOK_FALSE,

    // Identifiers
    TOK_IDENTIFIER,

    // Operators
    TOK_PLUS, TOK_MINUS, TOK_MULTIPLY, TOK_DIVIDE, TOK_MODULO,
    TOK_EQUAL, TOK_NOT_EQUAL, TOK_LESS, TOK_GREATER, TOK_LESS_EQUAL, TOK_GREATER_EQUAL,
    TOK_AND, TOK_OR, TOK_NOT, TOK_ASSIGN,

    // Delimiters
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMICOLON, TOK_COMMA,

    // Special
    TOK_EOF, TOK_ERROR
};

// Token structure
struct Token {
    TokenType type;
    string lexeme;
    int line;
    int column;

    Token(TokenType t, string lex, int ln, int col)
        : type(t), lexeme(lex), line(ln), column(col) {}
};

// =============================================================================
// 2. SCANNER CLASS DECLARATION
// =============================================================================

class Scanner {
private:
    string source;
    size_t start;
    size_t current;
    int line;
    int column;
    vector<Token> tokens;

    // Keyword map
    map<string, TokenType> keywords;

    // --- Private Helper Functions ---
    void initKeywords();
    bool isAtEnd();
    char advance();
    char peek();
    char peekNext();
    bool match(char expected);
    void addToken(TokenType type);
    void skipWhitespace();
    void scanComment();
    void scanString();
    void scanNumber();
    void scanIdentifier();
    void scanSupply();
    void scanToken();

public:
    // --- Public Interface ---
    Scanner(const string& src);
    vector<Token> scanTokens();
    static string tokenTypeToString(TokenType type);
};

#endif // SCANNER_H