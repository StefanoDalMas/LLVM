// Base AST Class
#include "../lexer/lexer.h"
#include <map>


namespace
{

    /// ExprAST - Base class for all expression nodes.
    class ExprAST
    {
    public:
        virtual ~ExprAST() = default;
    };

    /// NumberExprAST - Expression class for numeric literals like "1.0".
    class NumberExprAST : public ExprAST
    {
        double Val;

    public:
        NumberExprAST(double Val) : Val(Val) {}
    };

    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAST : public ExprAST
    {
        std::string Name;

    public:
        VariableExprAST(const std::string &Name) : Name(Name) {}
    };

    /// BinaryExprAST - Expression class for a binary operator.
    class BinaryExprAST : public ExprAST
    {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                      std::unique_ptr<ExprAST> RHS)
            : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    };

    /// CallExprAST - Expression class for function calls.
    class CallExprAST : public ExprAST
    {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(const std::string &Callee,
                    std::vector<std::unique_ptr<ExprAST>> Args)
            : Callee(Callee), Args(std::move(Args)) {}
    };

    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the number
    /// of arguments the function takes).
    class PrototypeAST
    {
        std::string Name;
        std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &Name, std::vector<std::string> Args)
            : Name(Name), Args(std::move(Args)) {}

        const std::string &getName() const { return Name; }
    };

    /// FunctionAST - This class represents a function definition itself.
    class FunctionAST
    {
        std::unique_ptr<PrototypeAST> Proto;
        std::unique_ptr<ExprAST> Body;

    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                    std::unique_ptr<ExprAST> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}
    };

} // end anonymous namespace

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokPrecedence()
{
    if (!isascii(CurTok))
        return -1;

    // Make sure it's a declared binop.
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str)
{
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
{
    LogError(Str);
    return nullptr;
}

std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr()
{
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken(); // consume the number
    return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> ParseParenExpr()
{
    getNextToken(); // eat (.
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat ).
    return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    std::string IdName = IdentifierStr;

    getNextToken(); // eat identifier.

    if (CurTok != '(') // Simple variable ref.
        return std::make_unique<VariableExprAST>(IdName);

    // Call.
    getNextToken(); // eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')')
    {
        while (true)
        {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    // Eat the ')'.
    getNextToken();

    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
std::unique_ptr<ExprAST> ParsePrimary()
{
    switch (CurTok)
    {
    default:
        return LogError("unknown token when expecting an expression");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    }
}

/// binoprhs
///   ::= ('+' primary)*
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS)
{
    // If this is a binop, find its precedence.
    while (true)
    {
        int TokPrec = GetTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
            return LHS;

        // Okay, we know this is a binop.
        int BinOp = CurTok;
        getNextToken(); // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec)
        {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS.
        LHS =
            std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

/// expression
///   ::= primary binoprhs
///
std::unique_ptr<ExprAST> ParseExpression()
{
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> ParsePrototype()
{
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success.
    getNextToken(); // eat ')'.

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParseDefinition()
{
    getNextToken(); // eat def.
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;

    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
    if (auto E = ParseExpression())
    {
        // Make an anonymous proto.
        auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                    std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern()
{
    getNextToken(); // eat extern.
    return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

void HandleDefinition()
{
    if (ParseDefinition())
    {
        fprintf(stderr, "Parsed a function definition.\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

void HandleExtern()
{
    if (ParseExtern())
    {
        fprintf(stderr, "Parsed an extern\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

void HandleTopLevelExpression()
{
    // Evaluate a top-level expression into an anonymous function.
    if (ParseTopLevelExpr())
    {
        fprintf(stderr, "Parsed a top-level expr\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
void MainLoop()
{
    while (true)
    {
        fprintf(stderr, "ready> ");
        switch (CurTok)
        {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main()
{
    // Install standard binary operators.
    // 1 is lowest precedence.
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest.

    // Prime the first token.
    fprintf(stderr, "ready> ");
    getNextToken();

    // Run the main "interpreter loop" now.
    MainLoop();

    return 0;
}