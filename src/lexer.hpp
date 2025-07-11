/** \file lexer.hpp
*/

#pragma once

#include <string>
#include <memory>
#include <ostream>
#include <cstdint>

namespace statpascal {

enum class TToken {

    // program symbols
    Begin, End, If, Then, Else, For, To, Downto, Do, Const, 				
    Type, Var, Absolute, While, Repeat, Until, Record, Array, Set, File,
    Vector, Matrix, 		
    Of, Procedure, Function, Case, With, Nil, Forward, External, Export, CDecl, Overload, Unit, 
    Interface, Implementation, Initialization, Finalization, Uses, Label, Goto, Program,

    // operators
    In, And, Or, Xor, Not, Shl, Shr,
    DivInt, Mod, Add, Sub, Mul, Div,
    Equal, GreaterThan, LessThan, GreaterEqual, LessEqual, NotEqual,
    SizeOf, AddrOp,

    // other symbols
    BracketOpen, BracketClose, SquareBracketOpen, SquareBracketClose,
    Comma, Semicolon, 
    Colon, Point, Define, Points, Dereference, Identifier, CharConst, IntegerConst, 	
    RealConst, StringConst, 
    Terminator,									

    // unrecognized token
    Error
};

std::ostream &operator << (std::ostream &, TToken);

class TLexer final {
public:
    TLexer ();
    ~TLexer () {}
    TLexer (const TLexer &) = delete;
    TLexer &operator = (const TLexer &) = delete;
    TLexer (TLexer &&) = default;
    TLexer &operator = (TLexer &&) = default;
    
    void setSource (const std::string &);
    void setSource (std::string &&);
    void setFilename (const std::string &fn);
    
    void getNextToken ();
    TToken getToken ();
    
    /** Returns true and parses next token if the argument is the current token.
        If the argument and the current token do not match, the method returns false
        and does not parse the next token. */
    bool checkToken (TToken) ;
    
    double getDouble () const;
    std::int64_t getInteger () const;
    std::string getString () const;
    
    /** same as getString converted to lower case */
    std::string getIdentifier () const;
    unsigned char getChar () const;
    
    class TLexerPosition {
    public:
        TLexerPosition (unsigned lineNumber, unsigned linePosition, const std::string &filename);
        unsigned getLineNumber () const;
        unsigned getLinePosition () const;
        std::string getFilename () const;
    
    private:
        const unsigned lineNumber, linePosition;
        const std::string filename;
    };
    
    TLexerPosition getLexerPosition () const;
    
private:
    class TLexerImpl;
    struct TLexerImplDeleter {
        void operator () (TLexerImpl *);
    };
    std::unique_ptr<TLexerImpl, TLexerImplDeleter> pImpl;
    
    const TLexerImpl *impl () const;
    TLexerImpl *impl ();
};

}
