#include "lexer.h"
#include <cctype>
#include <iostream>
#include <string>

static int gettok() {
    int resvalue;
    static int LastChar = ' ';
    while (isspace(LastChar)) {
        LastChar = getchar();
    }
    // check if is A-Z
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum(LastChar = getchar())) {
            IdentifierStr += LastChar;
        }
        if (IdentifierStr == "def") {
            resvalue = tok_def;
        } else if (IdentifierStr == "extern") {
            resvalue = tok_extern;
        } else {
            resvalue = tok_identifier;
        }
    }
    // this thing only checks 1.23 and not 1.23.45.678 for example
    else if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');
        NumVal = strtod(NumStr.c_str(), 0);
        resvalue = tok_number;
    }
    // now for the comments
    else if (LastChar == '#') {
        do {
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        if (LastChar != EOF) {
            return gettok();
        }
    }
    // now read EOF but don't eat it!
    else if (LastChar == EOF) {
        resvalue = tok_eof;
    }
    // it has to be an ASCII character like +,- and so on
    else {
        int ThisChar = LastChar;
        LastChar = getchar();
        resvalue = ThisChar;
    }
    return resvalue;
}
int main() { std::cout << gettok() << std::endl; }
