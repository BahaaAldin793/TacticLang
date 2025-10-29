#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <map>

using namespace std;

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
    TOK_COMMENT, TOK_EOF, TOK_ERROR
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

// Scanner class
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

    void initKeywords() {
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
    }

    bool isAtEnd() {
        return current >= source.length();
    }

    char advance() {
        column++;
        return source[current++];
    }

    char peek() {
        if (isAtEnd()) return '\0';
        return source[current];
    }

    char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source[current + 1];
    }

    bool match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        current++;
        column++;
        return true;
    }

    void addToken(TokenType type) {
        string text = source.substr(start, current - start);
        tokens.push_back(Token(type, text, line, column - text.length()));
    }

    void skipWhitespace() {
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

    void scanComment() {
        // Skip until end of line
        while (peek() != '\n' && !isAtEnd()) {
            advance();
        }
    }

    void scanString() {
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

    void scanNumber() {
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

    void scanIdentifier() {
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

    void scanSupply() {
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

    void scanToken() {
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
            }
            else {
                tokens.push_back(Token(TOK_ERROR, "Unexpected character: &", line, column));
            }
            break;
        case '|':
            if (match('|')) {
                addToken(TOK_OR);
            }
            else {
                tokens.push_back(Token(TOK_ERROR, "Unexpected character: |", line, column));
            }
            break;

        case '#':
            if (isalpha(peek())) {
                scanSupply();
            }
            else {
                scanComment();
            }
            break;

        case '"':
            scanString();
            break;

        default:
            if (isdigit(c)) {
                scanNumber();
            }
            else if (isalpha(c) || c == '_') {
                scanIdentifier();
            }
            else {
                tokens.push_back(Token(TOK_ERROR, string("Unexpected character: ") + c, line, column));
            }
            break;
        }
    }

public:
    Scanner(const string& src) : source(src), start(0), current(0), line(1), column(0) {
        initKeywords();
    }

    vector<Token> scanTokens() {
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

    static string tokenTypeToString(TokenType type) {
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
            "COMMENT", "EOF", "ERROR"
        };
        return names[type];
    }
};

// Read file contents
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

// Write tokens to output file
void writeTokensToFile(const vector<Token>& tokens, const string& outputPath) {
    ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        cerr << "Error: Could not create output file '" << outputPath << "'" << endl;
        return;
    }

    outFile << "TACTICLANG SCANNER OUTPUT" << endl;
    outFile << "=========================" << endl;
    outFile << "Total Tokens: " << tokens.size() << endl;
    outFile << "=========================" << endl << endl;

    for (const Token& token : tokens) {
        outFile << "Line " << token.line
            << ", Col " << token.column
            << ": " << Scanner::tokenTypeToString(token.type);

        if (!token.lexeme.empty() && token.type != TOK_EOF) {
            outFile << " [" << token.lexeme << "]";
        }
        outFile << endl;
    }

    outFile.close();
    cout << "Tokens written to: " << outputPath << endl;
}

// Main function
int main() {
    // File path
    string filepath = "D:\\1\\College\\4th Year\\Compilers\\Project\\Scanner\\Project1\\soldier.tac";
    string outputPath = "D:\\1\\College\\4th Year\\Compilers\\Project\\Scanner\\Project1\\tokens_output.txt";

    cout << "TacticLang Scanner" << endl;
    cout << "==================" << endl;
    cout << "Reading file: " << filepath << endl << endl;

    // Read source file
    string sourceCode = readFile(filepath);

    if (sourceCode.empty()) {
        cerr << "Error: Source file is empty or could not be read." << endl;
        return 1;
    }

    cout << "File read successfully (" << sourceCode.length() << " characters)" << endl;
    cout << "Scanning..." << endl << endl;

    // Create scanner and scan tokens
    Scanner scanner(sourceCode);
    vector<Token> tokens = scanner.scanTokens();

    // Display tokens to console
    cout << "Tokens Found:" << endl;
    cout << "-------------" << endl;

    int displayLimit = 50; // Display first 50 tokens in console
    for (size_t i = 0; i < tokens.size() && i < displayLimit; i++) {
        const Token& token = tokens[i];
        cout << "Line " << token.line
            << ", Col " << token.column
            << ": " << Scanner::tokenTypeToString(token.type);

        if (!token.lexeme.empty() && token.type != TOK_EOF) {
            cout << " [" << token.lexeme << "]";
        }
        cout << endl;
    }

    if (tokens.size() > displayLimit) {
        cout << "... (showing first " << displayLimit << " tokens)" << endl;
    }

    cout << endl << "Total tokens: " << tokens.size() << endl << endl;

    // Write all tokens to output file
    writeTokensToFile(tokens, outputPath);

    // Count error tokens
    int errorCount = 0;
    for (const Token& token : tokens) {
        if (token.type == TOK_ERROR) {
            errorCount++;
        }
    }

    if (errorCount > 0) {
        cout << endl << "Warning: Found " << errorCount << " error token(s)" << endl;
    }
    else {
        cout << endl << "Scanning completed successfully - no errors!" << endl;
    }

    return 0;
}