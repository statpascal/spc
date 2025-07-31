#include "lexer.hpp"

#include <unordered_map>
#include <limits>
#include <cmath>
#include <array>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#include <iostream>

namespace statpascal {

std::ostream &operator << (std::ostream &os, TToken token) {
    static std::array<std::string, static_cast<std::size_t> (TToken::Error) + 1> tokens = {
        // program symbols        
        "Begin", "End", "If", "Then", "Else", "For", "To", "Downto", "Do", "Const", 				
        "Type", "Var", "Absolute", "While", "Repeat", "Until", "Record", "Array", "Set", "File", "shortstring",
        "Vector", "Matrix", 
        "Of", "Procedure", "Function", "Case", "With", "Nil", "Forward", "External", "Export", "CDecl", "Overload", "Unit", 
        "Interface", "Implementation", "Initialization", "Finalization", "Uses", "Label", "Goto", "Program",

        // operators
        "In", "And", "Or", "Xor", "Not", "Shl", "Shr",
        "DivInt", "Mod", "Add", "Sub", "Mul", "Div",
        "Equal", "GreaterThan", "LessThan", "GreaterEqual", "LessEqual", "NotEqual",
        "SizeOf", "AddrOp",

        // other symbols
        "BracketOpen", "BracketClose", "SquareBracketOpen", "SquareBracketClose",
        "Comma", "Semicolon",
        "Colon", "Point", "Define", "Points", "Dereference", "Identifier", "CharConst", "IntegerConst",
        "RealConst", "StringConst", 
        "Terminator",

        // unrecognized token
        "Error"
    };
    return os << tokens [static_cast<std::size_t> (token)];
}

class TLexer::TLexerImpl {
public:
    TLexerImpl ();
    ~TLexerImpl () = default;

    void setSource (const std::string &);
    void setSource (std::string &&);
    void setFilename (const std::string &fn);
    
    void getNextToken ();
    TToken getToken ();
    bool checkToken (TToken) ;
    
    double getDouble () const;
    std::int64_t getInteger () const;
    std::string getString () const;
    std::string getIdentifier () const;
    unsigned char getChar () const;
    
    unsigned getLineNumber () const;
    unsigned getLinePosition () const;
    std::string getFilename () const;

private:
    typedef const char *TSourceIterator;
    typedef std::unordered_map<std::string, TToken> TStringTokenMap;
    typedef std::unordered_map<char, TToken> TCharTokenMap;

    void initLexer ();
    bool skipComment ();
    bool skipWhiteSpace ();
    
    void parseNumber ();
    void parseHex ();
    void parseOctal ();
    void parseNumericChar ();
    void parseQuotedString ();
    void parseString ();
    bool parseOperator (char base, TToken baseToken, char combine1, TToken combinedToken1, char combine2 = '\0', TToken combinedToken2 = TToken::Error);
    void parseSymbol ();
    
    void setCurrentTokenAndAdvance (TToken);
    
    const char stringTerminator = '\'';
    const char stringNumericEscape = '#';
    
    TStringTokenMap reservedWords;
    TCharTokenMap reservedSymbols;
    
    std::string source;
    TSourceIterator sourceIt, lineBegin;
    unsigned lineNumber, lastReadLineNumber, lastReadPosition;
    std::string filename;
    
    TToken currentToken;
    double fVal;
    std::int64_t iVal;
    std::string sVal, sValLower;
    unsigned char cVal;
};

inline void TLexer::TLexerImpl::setCurrentTokenAndAdvance (TToken t) {
    currentToken = t;
    ++sourceIt;
}

TLexer::TLexerImpl::TLexerImpl ():
  reservedWords ({
    {"and", 		TToken::And},
    {"array", 		TToken::Array}, 
    {"begin", 		TToken::Begin}, 
    {"case", 		TToken::Case}, 
    {"const", 		TToken::Const}, 
    {"div",		TToken::DivInt}, 
    {"sizeof",		TToken::SizeOf},
    {"do", 		TToken::Do}, 
    {"downto", 		TToken::Downto}, 
    {"else", 		TToken::Else}, 
    {"end", 		TToken::End}, 
    {"for", 		TToken::For}, 
    {"forward", 	TToken::Forward}, 
    {"external",	TToken::External},
    {"cdecl",           TToken::CDecl},
    {"overload",	TToken::Overload},
    {"export",		TToken::Export},
    {"file",		TToken::File},
    {"shortstring",     TToken::ShortString},
    {"finalization",    TToken::Finalization},
    {"function", 	TToken::Function}, 
    {"goto", 		TToken::Goto},
    {"if", 		TToken::If}, 
    {"implementation", 	TToken::Implementation}, 
    {"in", 		TToken::In}, 
    {"initialization",  TToken::Initialization},
    {"interface", 	TToken::Interface}, 
    {"label", 		TToken::Label}, 
    {"matrix", 		TToken::Matrix}, 
    {"mod", 		TToken::Mod},
//    {"nil", 		TToken::Nil},
    {"not", 		TToken::Not}, 
    {"shl",		TToken::Shl},
    {"shr",		TToken::Shr},
    {"of", 		TToken::Of},
    {"or", 		TToken::Or}, 
    {"procedure", 	TToken::Procedure}, 
    {"program", 	TToken::Program}, 
    {"with", 		TToken::With}, 
    {"record", 		TToken::Record}, 
    {"repeat", 		TToken::Repeat}, 
    {"set", 		TToken::Set}, 
    {"then", 		TToken::Then}, 
    {"to", 		TToken::To}, 
    {"type", 		TToken::Type}, 
    {"unit", 		TToken::Unit}, 
    {"until", 		TToken::Until}, 
    {"uses", 		TToken::Uses}, 
    {"var", 		TToken::Var},
    {"absolute",	TToken::Absolute},
    {"vector", 		TToken::Vector}, 
    {"while", 		TToken::While}, 
    {"xor", 		TToken::Xor} 
  }), reservedSymbols ({
    {'+', 	TToken::Add},
    {'-', 	TToken::Sub},
    {'*',	TToken::Mul},
    {'/', 	TToken::Div},
    {'^', 	TToken::Dereference},
    {'(', 	TToken::BracketOpen},
    {')', 	TToken::BracketClose},
    {'[', 	TToken::SquareBracketOpen},
    {']', 	TToken::SquareBracketClose},
    {'=',	TToken::Equal},
    {'@',	TToken::AddrOp},
    {',', 	TToken::Comma},
    {';', 	TToken::Semicolon},
    {'\0', 	TToken::Terminator},
    // The following need evaluation of the second character:
    {'*',	TToken::Error},
    {'.',	TToken::Error},
    {'<',	TToken::Error},
    {'>',	TToken::Error},
    {':',	TToken::Error},
    {'=',	TToken::Error}
  }) {
}

void TLexer::TLexerImpl::initLexer () {
    lineBegin = sourceIt = &source [0];
    lineNumber = lastReadLineNumber = lastReadPosition = 1;
    getNextToken ();
}

void TLexer::TLexerImpl::setSource (const std::string &s) {
    source = s;
    initLexer ();
}

void TLexer::TLexerImpl::setSource (std::string &&s) {
    source = std::move (s);
    initLexer ();
}

void TLexer::TLexerImpl::setFilename (const std::string &fn) {
    filename = fn;
    std::ifstream f (fn);
    std::stringstream buf;
    buf << f.rdbuf ();
    setSource (buf.str ());
}

bool TLexer::TLexerImpl::skipComment () {
    if (*sourceIt == '(' && *(sourceIt + 1) == '*') {
        sourceIt += 2;
        while (*sourceIt) {
            if (*sourceIt == '\n') {
                ++lineNumber;
                lineBegin = sourceIt + 1;
            } else if (*sourceIt == '*' && *(sourceIt + 1) == ')') {
                sourceIt += 2;
                return true;
            } 
            ++sourceIt;
        }
        return true;        
    } else if (*sourceIt == '{') {
        do {
            ++sourceIt;
            if (*sourceIt == '\n') {
                ++lineNumber;
                lineBegin = sourceIt + 1;
            } else if (*sourceIt == '}') {
                ++sourceIt;
                return true;
            }
        } while (*sourceIt);
        return true;
    } else if (*sourceIt == '/' && *(sourceIt + 1) == '/') {
        do 
            ++sourceIt;
        while (*sourceIt && *sourceIt != '\n');
        if (*sourceIt == '\n') {
            ++sourceIt; ++lineNumber; lineBegin = sourceIt + 1;
        }
        return true;
    } else 
        return false;
}

bool TLexer::TLexerImpl::skipWhiteSpace () {
    bool skipped = false;
    while (isspace (*sourceIt) || *sourceIt == '\x1a') {
        if (*sourceIt == '\n') {
            ++lineNumber;
            lineBegin = sourceIt + 1;
        }
        ++sourceIt;
        skipped = true;
    } 
    return skipped;
}

void TLexer::TLexerImpl::parseHex () {
    std::uint64_t uVal = 0;
    ++sourceIt;
    while (isxdigit (*sourceIt)) {
        uVal *= 16;
        if (*sourceIt >= '0' && *sourceIt <= '9')
            uVal += *sourceIt - '0';
        else if (*sourceIt >= 'A' && *sourceIt <= 'F')
            uVal += *sourceIt - 'A' + 10;
        else
            uVal += *sourceIt - 'a' + 10;
        ++sourceIt;
    }
    iVal = uVal;
    currentToken = TToken::IntegerConst;
}

void TLexer::TLexerImpl::parseOctal () {
    std::uint64_t uVal = 0;
    ++sourceIt;
    while (*sourceIt >= '0' && *sourceIt <= '7')
        uVal = uVal * 8 + *sourceIt++ - '0';
    iVal = uVal;
    currentToken = TToken::IntegerConst;
}

void TLexer::TLexerImpl::parseNumber () {
    const std::uint64_t maxVal = std::numeric_limits<std::int64_t>::max (),
                        maxVal10 = maxVal / 10 + 1;
    
    bool isInteger = true;
    fVal = 0.0;
    std::uint64_t uVal = 0;
    
    while (isdigit (*sourceIt)) {
        fVal = 10.0 * fVal + (*sourceIt - '0');
        if (isInteger) {
            if (uVal < maxVal10)
                uVal = 10 * uVal + (*sourceIt - '0');
            else
                isInteger = false;
        }
        ++sourceIt;
    }
    if (uVal > maxVal)
        isInteger = false;
        
    if (*sourceIt == '.' && *(sourceIt + 1) != '.') {
        ++sourceIt;
        isInteger = false;
        double dec = 0.1;
        while (isdigit (*sourceIt)) {
            fVal += dec * (*sourceIt - '0');
            dec *= 0.1;
            ++sourceIt;
        }
    }
    
    if (*sourceIt == 'e' || *sourceIt == 'E') {
        ++sourceIt;
        isInteger = false;
        bool isNegative = false;
        if (*sourceIt == '+' || *sourceIt == '-') {
            if (*sourceIt == '-') isNegative = true;
            ++sourceIt;
        }
        // TODO: check for numeric overflow error
        std::int64_t eVal = 0;
        // TODO: check for at least one digit 
        while (isdigit (*sourceIt)) {
            eVal = eVal * 10 + (*sourceIt - '0');
            ++sourceIt;
        }
        if (isNegative)
            eVal = -eVal;
        fVal *= std::pow (10.0, eVal);
    }

    if (isInteger) {
        iVal = uVal;
        currentToken = TToken::IntegerConst;
    } else
        currentToken = TToken::RealConst;
}

void TLexer::TLexerImpl::parseNumericChar () {
    if (*sourceIt == '$')
        parseHex ();
    else
        parseNumber ();
    if (currentToken == TToken::IntegerConst && iVal >= 0 && iVal <= 255)
        sVal += static_cast<char> (iVal);
    else
        sVal += '?';
}

void TLexer::TLexerImpl::parseQuotedString () {
    bool done = false;
    while (!done) {
        if (!*sourceIt || *sourceIt == '\n') {
            currentToken = TToken::Error;
            done = true;
        } else if (*sourceIt == stringTerminator) {
            if (*(sourceIt + 1) == stringTerminator) {
                sVal.push_back (stringTerminator);
                ++sourceIt;
            } else {
                currentToken = TToken::StringConst;
                done = true;
            }
        } else 
            sVal.push_back (*sourceIt);
        ++sourceIt;
    }
    
}

void TLexer::TLexerImpl::parseString () {
    sVal.clear ();
    
    char c = *sourceIt;
    while (c == stringTerminator || c == stringNumericEscape) {
        ++sourceIt;
        if (c == stringTerminator)
            parseQuotedString ();
        else
            parseNumericChar ();
        c = *sourceIt;
    }
    
    if (currentToken != TToken::Error && sVal.length () == 1) {
        currentToken = TToken::CharConst;
        cVal = sVal.front ();
    }
}

void TLexer::TLexerImpl::parseSymbol () {
    sVal.clear (); 
    sValLower.clear ();
    while (isalnum (*sourceIt) || *sourceIt == '_') 
        sVal.push_back (*sourceIt++);
    sValLower.resize (sVal.length ());
    std::transform (sVal.begin (), sVal.end (), sValLower.begin (), ::tolower);
    
    TStringTokenMap::const_iterator it = reservedWords.find (sValLower);
    currentToken = (it == reservedWords.end ()) ? TToken::Identifier : it->second;
}

bool TLexer::TLexerImpl::parseOperator (char base, TToken baseToken, char combine1, TToken combinedToken1, char combine2, TToken combinedToken2) {
    if (base == *sourceIt) {
        char c = *++sourceIt;
        if (c == combine1 || (combine2 && c == combine2))
            setCurrentTokenAndAdvance (c == combine1 ? combinedToken1 : combinedToken2);
        else
            currentToken = baseToken;
        return true;
    }
    return false;
}

void TLexer::TLexerImpl::getNextToken () {
    while (skipWhiteSpace () || skipComment ());
    
    char c = *sourceIt;
    if (isalpha (c) || c == '_')
        parseSymbol ();
    else if (c == '$')
        parseHex ();
    else if (c == '&')
        parseOctal ();
    else if (isdigit (c) || (c == '.' && isdigit (*(sourceIt + 1))))
        parseNumber ();
    else if (c == stringTerminator || c == stringNumericEscape)
        parseString ();
    else if (!c)
        currentToken = TToken::Terminator;
    else {
        TCharTokenMap::const_iterator it = reservedSymbols.find (c);
        if (it == reservedSymbols.end ())
            setCurrentTokenAndAdvance (TToken::Error);
        else if (it->second != TToken::Error)
            setCurrentTokenAndAdvance (it->second);
        else
          parseOperator ('.', TToken::Point, '.', TToken::Points) ||
          parseOperator ('<', TToken::LessThan, '=', TToken::LessEqual, '>', TToken::NotEqual) ||
          parseOperator ('>', TToken::GreaterThan, '=', TToken::GreaterEqual) ||
          parseOperator (':', TToken::Colon, '=', TToken::Define);
    }
}

inline TToken TLexer::TLexerImpl::getToken () {
    lastReadLineNumber = lineNumber;
    lastReadPosition = sourceIt - lineBegin + 1;
    return currentToken;
}

bool TLexer::TLexerImpl::checkToken (TToken t) {
    if (getToken () == t) {
        getNextToken ();
        return true;
    } else
        return false;
}

inline double TLexer::TLexerImpl::getDouble () const {
    return fVal;
}
    
inline std::int64_t TLexer::TLexerImpl::getInteger () const {
    return iVal;
}
    
inline std::string TLexer::TLexerImpl::getString () const {
    return sVal;
}

inline std::string TLexer::TLexerImpl::getIdentifier () const {
    return sValLower;
}

inline unsigned char TLexer::TLexerImpl::getChar () const {
    return cVal;
}

unsigned TLexer::TLexerImpl::getLineNumber () const {
    return lastReadLineNumber;
}

unsigned TLexer::TLexerImpl::getLinePosition () const {
    return lastReadPosition;
}

std::string TLexer::TLexerImpl::getFilename () const {
    return filename;
}

void TLexer::TLexerImplDeleter::operator () (TLexer::TLexerImpl *p) {
    delete p;
}

// ---

TLexer::TLexerPosition::TLexerPosition (unsigned lineNumber, unsigned linePosition, const std::string &filename):
  lineNumber (lineNumber), linePosition (linePosition), filename (filename) {
}

unsigned TLexer::TLexerPosition::getLineNumber () const {
    return lineNumber;
}

unsigned TLexer::TLexerPosition::getLinePosition () const {
    return linePosition;
}

std::string TLexer::TLexerPosition::getFilename () const {
    return filename;
}

// ---

TLexer::TLexer ():
  pImpl (new TLexerImpl) {
}

inline const TLexer::TLexerImpl *TLexer::impl () const {
    return pImpl.get ();
}

inline TLexer::TLexerImpl *TLexer::impl () {
    return pImpl.get ();
}

void TLexer::setSource (const std::string &s) {
    impl ()->setSource (s);
}

void TLexer::setSource (std::string &&s) {
    impl ()->setSource (std::move (s));
}

void TLexer::setFilename (const std::string &fn) {
    impl ()->setFilename (fn);
}

void TLexer::getNextToken () {
    impl ()->getNextToken ();
}

TToken TLexer::getToken () {
    return impl ()->getToken ();
}

bool TLexer::checkToken (TToken t) {
    return impl ()->checkToken (t);
}

double TLexer::getDouble () const {
    return impl ()->getDouble ();
}

std::int64_t TLexer::getInteger () const {
    return impl ()->getInteger ();
}

std::string TLexer::getString () const {
    return impl ()->getString ();
}

std::string TLexer::getIdentifier () const {
    return impl ()->getIdentifier ();
}

unsigned char TLexer::getChar () const {
    return impl ()->getChar ();
}

TLexer::TLexerPosition TLexer::getLexerPosition () const {
    return TLexerPosition (impl ()->getLineNumber (), impl ()->getLinePosition (), impl ()->getFilename ());
}

}
