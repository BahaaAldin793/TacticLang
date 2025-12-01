#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <sstream>
using namespace std;

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
    TOK_COMMENT, TOK_EOF, TOK_ERROR
};
struct Token {
    TokenType type;
    string lexeme;
    int line;
    int column;
    
    Token(TokenType t, string lex, int ln, int col) 
        : type(t), lexeme(lex), line(ln), column(col) {}
};