#include "scanner.h"  // <-- 1. THE MOST IMPORTANT FIX: Include the header.
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

// 2. ALL DEFINITIONS (enum, struct, class) are REMOVED.
//    They are now inherited from "scanner.h".

// 3. All function bodies now use the 'Scanner::' prefix
//    to show they "implement" the class from the header.

// --- Constructor Implementation ---
Scanner::Scanner(const string& src) : source(src), start(0), current(0), line(1), column(0) {
    initKeywords();
}

// --- Keyword Map Implementation ---
void Scanner::initKeywords() {
    keywords["campaign"] = TOK_CAMPAIGN;
    keywords["tactic"] = TOK_TACTIC;
    keywords["troop"] = TOK_TROOP;
    keywords["ammo"] = TOK_AMMO;
    keywords["codename"] = TOK_CODENAME;
    keywords["status"] = TOK_STATUS;
    keywords["brief"] = TOK_BRIEF;
    keywords["intel"] = TOK_INTEL;
    keywords["evaluate"] = TOK_EVALUATE;
    keywords["adjust"] = TOK_ADJUST;
    keywords["maintain"] = TOK_MAINTAIN;
    keywords["deploy"] = TOK_DEPLOY;
    keywords["retreat"] = TOK_RETREAT;
    keywords["abort"] = TOK_ABORT;
    keywords["true"] = TOK_TRUE;
    keywords["false"] = TOK_FALSE;
    keywords["#supply"] = TOK_SUPPLY; // Handle #supply
}

// --- Main Scan Function Implementation ---
vector<Token> Scanner::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        skipWhitespace();
        if (!isAtEnd()) {
            start = current;
            scanToken();
        }
    }

    tokens.push_back(Token(TOK_EOF, "", line, column));
    return tokens;
}

// --- ALL OTHER SCANNER HELPER FUNCTIONS ---

bool Scanner::isAtEnd() {
    return current >= source.length();
}

char Scanner::advance() {
    column++;
    return source[current++];
}

char Scanner::peek() {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Scanner::peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

bool Scanner::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    column++;
    return true;
}

void Scanner::addToken(TokenType type) {
    string text = source.substr(start, current - start);
    tokens.push_back(Token(type, text, line, column - text.length()));
}

void Scanner::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                line++;
                column = 0;
                advance();
                break;
            default:
                return;
        }
    }
}

void Scanner::scanComment() {
    // Skip until end of line
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
}

void Scanner::scanString() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            column = 0;
        }
        advance();
    }

    if (isAtEnd()) {
        tokens.push_back(Token(TOK_ERROR, "Unterminated string", line, column));
        return;
    }

    advance(); // Closing "
    addToken(TOK_STRING);
}

void Scanner::scanNumber() {
    while (isdigit(peek())) {
        advance();
    }

    // Check for decimal point
    if (peek() == '.' && isdigit(peekNext())) {
        advance(); // Consume '.'
        while (isdigit(peek())) {
            advance();
        }
        addToken(TOK_DOUBLE);
    }
    else {
        addToken(TOK_INTEGER);
    }
}

void Scanner::scanIdentifier() {
    while (isalnum(peek()) || peek() == '_') {
        advance();
    }

    string text = source.substr(start, current - start);
    TokenType type = TOK_IDENTIFIER;

    if (keywords.find(text) != keywords.end()) {
        type = keywords[text];
    }

    addToken(type);
}

void Scanner::scanSupply() {
    // #supply directive
    while (isalpha(peek())) {
        advance();
    }

    string text = source.substr(start, current - start);
    if (text == "#supply") {
        addToken(TOK_SUPPLY);
    }
    else {
        tokens.push_back(Token(TOK_ERROR, "Unknown directive: " + text, line, column));
    }
}

void Scanner::scanToken() {
    char c = advance();

    switch (c) {
        case '(': addToken(TOK_LPAREN); break;
        case ')': addToken(TOK_RPAREN); break;
        case '{': addToken(TOK_LBRACE); break;
        case '}': addToken(TOK_RBRACE); break;
        case ';': addToken(TOK_SEMICOLON); break;
        case ',': addToken(TOK_COMMA); break;
        case '+': addToken(TOK_PLUS); break;
        case '-': addToken(TOK_MINUS); break;
        case '*': addToken(TOK_MULTIPLY); break;
        case '/': addToken(TOK_DIVIDE); break;
        case '%': addToken(TOK_MODULO); break;

        case '=':
            addToken(match('=') ? TOK_EQUAL : TOK_ASSIGN);
            break;
        case '!':
            addToken(match('=') ? TOK_NOT_EQUAL : TOK_NOT);
            break;
        case '<':
            addToken(match('=') ? TOK_LESS_EQUAL : TOK_LESS);
            break;
        case '>':
            addToken(match('=') ? TOK_GREATER_EQUAL : TOK_GREATER);
            break;
        case '&':
            if (match('&')) {
                addToken(TOK_AND);
            } else {
                tokens.push_back(Token(TOK_ERROR, "Unexpected character: &", line, column));
            }
            break;
        case '|':
            if (match('|')) {
                addToken(TOK_OR);
            } else {
                tokens.push_back(Token(TOK_ERROR, "Unexpected character: |", line, column));
            }
            break;

        case '#':
            if (isalpha(peek())) {
                scanSupply();
            } else {
                scanComment();
            }
            break;

        case '"':
            scanString();
            break;

        default:
            if (isdigit(c)) {
                scanNumber();
            } else if (isalpha(c) || c == '_') {
                scanIdentifier();
            } else {
                tokens.push_back(Token(TOK_ERROR, string("Unexpected character: ") + c, line, column));
            }
            break;
    }
}


// --- Static Token-to-String Implementation ---
string Scanner::tokenTypeToString(TokenType type) {
    static const char* names[] = {
        "CAMPAIGN", "TACTIC", "TROOP", "AMMO", "CODENAME", "STATUS",
        "BRIEF", "INTEL", "EVALUATE", "ADJUST", "MAINTAIN", "DEPLOY",
        "RETREAT", "ABORT", "SUPPLY",
        "INTEGER", "DOUBLE", "STRING", "TRUE", "FALSE",
        "IDENTIFIER",
        "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "MODULO",
        "EQUAL", "NOT_EQUAL", "LESS", "GREATER", "LESS_EQUAL", "GREATER_EQUAL",
        "AND", "OR", "NOT", "ASSIGN",
        "LPAREN", "RPAREN", "LBRACE", "RBRACE", "SEMICOLON", "COMMA",
        "EOF", "ERROR"
        // Note: TOK_COMMENT is removed as we don't store it
    };
    return names[type];
}

// 4. ALL OTHER FUNCTIONS like readFile, writeTokensToFile, and main
//    are REMOVED from this file. They are in parser.cpp.