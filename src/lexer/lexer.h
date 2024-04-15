#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <cctype>
#include <string>

enum Token
{
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5,
};

extern int CurTok;
extern std::string IdentifierStr;
extern double NumVal;

int gettok();
int getNextToken();

#endif // TOKENIZER_H
