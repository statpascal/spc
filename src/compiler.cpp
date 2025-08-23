#include "compilerimpl.hpp"

#include "expression.hpp"
#include "codegenerator.hpp"
#include "tms9900gen.hpp"
#include "config.hpp"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <stack>
#include <filesystem>

namespace statpascal {

TBlock::TBlock (TCompilerImpl &compiler, TSymbolList *symbols, TSymbol *ownSymbol, TBlock *parent):
  compiler (compiler),
  lexer (compiler.getLexer ()),
  symbols (symbols),
  ownSymbol (ownSymbol),
  parentBlock (parent),
  statements (nullptr),
  isUnitInterface (false),
  parsingTypeDeclaration (false),
  displayNeeded (false) {
}

const TSimpleConstant *TBlock::parseConstantLiteral () {
    const TSimpleConstant *result = nullptr;
    switch (lexer.getToken ()) {
        case TToken::IntegerConst:
            result = compiler.createMemoryPoolObject<TSimpleConstant> (lexer.getInteger (), &stdType.Int64);
            break;
        case TToken::CharConst:
            result = compiler.createMemoryPoolObject<TSimpleConstant> (static_cast<std::int64_t> (lexer.getChar ()), &stdType.Char);
            break;
        case TToken::RealConst:
            result = compiler.createMemoryPoolObject<TSimpleConstant> (lexer.getDouble (), &stdType.Real);
            break;
        case TToken::StringConst:
            result = compiler.createMemoryPoolObject<TSimpleConstant> (lexer.getString (), &stdType.String);
            break;
        case TToken::SizeOf:
            return parseConstantSizeof ();
        case TToken::SquareBracketOpen:
            return parseSetConstant ();
        default:
            // return something useful after error message
            result = compiler.createMemoryPoolObject<TSimpleConstant> (static_cast<std::int64_t> (1), &stdType.Int64);
            compiler.errorMessage (TCompilerImpl::SyntaxError, "Constant literal expected");
    }
    lexer.getNextToken ();
    return result;
}

const TSimpleConstant *TBlock::parseSetConstant () {
    TEnumeratedType *baseType = nullptr;
    TSimpleConstant *result = compiler.createMemoryPoolObject<TSimpleConstant> ();
    lexer.getNextToken ();
    bool firstExpression = true;
    
    if (!lexer.checkToken (TToken::SquareBracketClose)) {
        do {
            const TSimpleConstant *c1 = parseConstExpression (), *c2 = nullptr;
            if (lexer.checkToken (TToken::Points))
                c2 = parseConstExpression ();
                
            if (firstExpression) {
                if (c1) {
                    TType *type = c1->getType ();
                    if (!type->isEnumerated ())
                            compiler.errorMessage (TCompilerImpl::InvalidType, "Need enumerated type to construct set, but got '" + type->getName () + "'");
                        else                        
                            baseType = static_cast<TEnumeratedType *> (type);
                }
                firstExpression = false;
            }
            if (baseType && c1) {
                if (c1->getType () != baseType)
                    compiler.errorMessage (TCompilerImpl::InvalidType, "Cannot include value of type '" + c1->getType ()->getName () + "' in set of type '" + baseType->getName () + "'");
                if (c2 && c2->getType () != baseType)
                    compiler.errorMessage (TCompilerImpl::InvalidType, "Cannot include value of type '" + c2->getType ()->getName () + "' in set of type '" + baseType->getName () + "'");
                if (c2)
                    result->addSetRange (c1->getInteger (), c2->getInteger ());
                else
                    result->addSetValue (c1->getInteger ());
            }
        } while (lexer.checkToken (TToken::Comma));
        compiler.checkToken (TToken::SquareBracketClose, "']' expected to end set constructor");          
    }
    result->setType (compiler.createMemoryPoolObject<TSetType> (baseType));
    return result;
}

const TSimpleConstant *TBlock::parseConstantSizeof () {
    lexer.getNextToken ();
    compiler.checkToken (TToken::BracketOpen, "'(' expected in sizeof");
    const TType *type = nullptr;
    if (lexer.getToken () == TToken::Identifier) 
        if (TSymbol *symbol = symbols->searchSymbol (lexer.getIdentifier (), TSymbol::NamedType)) {
            type = symbol->getType ();
            lexer.getNextToken ();
        }
    if (!type) 
        if (const TExpressionBase *expr = TExpression::parse (*this))
            type = expr->getType ();
            
    compiler.checkToken (TToken::BracketClose, "')' expected in sizeof");
    return compiler.createMemoryPoolObject<TSimpleConstant> (static_cast<std::int64_t> (type ? type->getSize () : 1), &stdType.Int64);
}

const TSimpleConstant *TBlock::parseConstFactor () {
    const TSimpleConstant *result = nullptr;
    if (lexer.getToken () == TToken::Identifier) {
        const std::string identifier = lexer.getIdentifier ();
        lexer.getNextToken ();
        if (TSymbol *symbol = symbols->searchSymbol (identifier)) {
            if (symbol->checkSymbolFlag (TSymbol::Constant))
                result = dynamic_cast<const TSimpleConstant *> (symbol->getConstant ());
            else if (symbol->checkSymbolFlag (TSymbol::Routine))
                result = compiler.createMemoryPoolObject<TSimpleConstant> (identifier, symbol->getType ());
            else
                compiler.errorMessage (TCompilerImpl::ConstExpected, "'" + identifier + "' cannot be used as a constant");
        } else
            compiler.errorMessage (TCompilerImpl::IdentifierNotFound, "'" + identifier + "' is not declared");
    } else if (lexer.getToken () == TToken::BracketOpen) {
        lexer.getNextToken ();
        result = parseConstExpression ();
        compiler.checkToken (TToken::BracketClose, "')' expected in constant expression");
    } else 
        result = parseConstantLiteral ();
    return result;
}

const TSimpleConstant *TBlock::parseConstTerm () {
    const TSimpleConstant *result = parseConstFactor ();
    TToken token = lexer.getToken ();
    while (token == TToken::Mul || token == TToken::Div || token == TToken::DivInt) {
        lexer.getNextToken ();
        const TSimpleConstant *c = parseConstFactor ();
        if (result->getType () == &stdType.Int64 && c->getType () == &stdType.Int64)
            if (token == TToken::Mul)
                result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getInteger () * c->getInteger (), &stdType.Int64);
            else if (!c->getInteger ())
                compiler.errorMessage (TCompilerImpl::DivisionByZero);
            else if (token == TToken::DivInt)
                result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getInteger () / c->getInteger (), &stdType.Int64);
            else
                result = compiler.createMemoryPoolObject<TSimpleConstant> (static_cast<double> (result->getInteger ()) / c->getInteger (), &stdType.Real);
        else if (result->hasType (&stdType.Int64, &stdType.Real) && c->hasType (&stdType.Int64, &stdType.Real) && token != TToken::DivInt)
            if (token == TToken::Mul)
                result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getDouble () * c->getDouble (), &stdType.Real);
            else if (c->getDouble () == 0.0)
                compiler.errorMessage (TCompilerImpl::DivisionByZero);
            else 
                result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getDouble () / c->getDouble (), &stdType.Real);
        else 
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes);
        token = lexer.getToken ();
    }
    return result;
}

const TSimpleConstant *TBlock::parseConstExpression () {
    bool isNegative = lexer.checkToken (TToken::Sub);
    const TSimpleConstant *result = parseConstTerm ();
    if (isNegative) {
        if (result->getType () == &stdType.Int64)
            result = compiler.createMemoryPoolObject<TSimpleConstant> (-result->getInteger (), &stdType.Int64);
        else if (result->getType () == &stdType.Real)
            result = compiler.createMemoryPoolObject<TSimpleConstant> (-result->getDouble (), &stdType.Real);
        else {
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Negative sign connot be applied to type '" + result->getType ()->getName ());
//            result->setType (nullptr);
        }
    }
    TToken token = lexer.getToken ();
    while (token == TToken::Add || token == TToken::Sub) {
        lexer.getNextToken ();
        const TSimpleConstant *c = parseConstTerm ();
        if (result->getType () == &stdType.Int64 && c->getType () == &stdType.Int64)
            if (token == TToken::Add)
                result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getInteger () + c->getInteger (), &stdType.Int64);
            else
                result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getInteger () - c->getInteger (), &stdType.Int64);
        else if (result->getType () == &stdType.String && c->getType () == &stdType.String)
            if (token == TToken::Add)
                result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getString () + c->getString (), &stdType.String);
            else 
                compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Cannot substract string constants");
        else if (result->getType () && c->getType ()) {
            if (result->hasType (&stdType.Int64, &stdType.Real) && c->hasType (&stdType.Int64, &stdType.Real)) 
                if (token == TToken::Add)
                    result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getDouble () + c->getDouble (), &stdType.Real);
                else
                    result = compiler.createMemoryPoolObject<TSimpleConstant> (result->getDouble () - c->getDouble (), &stdType.Real);
            else
                compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Error in constant expression");
        }
        token = lexer.getToken ();
    }
    return result;
}


TSymbol *TBlock::checkVariable () {
    TSymbol *symbol = nullptr;
    if (lexer.getToken () == TToken::Identifier) {
        const std::string identifier = lexer.getIdentifier ();
        lexer.getNextToken ();
        symbol = symbols->searchSymbol (identifier);
        if (!symbol) 
            compiler.errorMessage (TCompilerImpl::IdentifierNotFound, "'" + identifier + "' is not declared");
        else if (symbol->checkSymbolFlag (TSymbol::Constant))
            // TODO: check for other errors?
            compiler.errorMessage (TCompilerImpl::VariableExpected, "'" + identifier + "' is not a variable");
    } else
        compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Identifier expcted");
    return symbol;
}

void TBlock::checkDefinitions () {
    for (TSymbol *s: *symbols)
        if (s->checkSymbolFlag (TSymbol::Label) && s->getOffset () == TSymbol::UndefinedLabelUsed)
            compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "Label '" + s->getName () + "' used but not defined");
        else if (s->checkSymbolFlag (TSymbol::Forward))
            compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "Definition of forward subroutine '" + s->getName () + "' missing");
            
    if (symbols->getLevel () != 1) {            
        for (TSymbol *s: *symbols)
            if (s->checkSymbolFlag (TSymbol::StaticVariable)) {
                s->setName ("$static_" + s->getName ());
                s->setLevel (1);
            }
        TSymbolList *prev = symbols;
        while (prev && prev->getLevel () != 1)
            prev = prev->getPreviousLevel ();
        if (prev)
            symbols->moveSymbols (TSymbol::StaticVariable, *prev);
        else
            compiler.errorMessage (TCompilerImpl::InternalError, "Cannot move typed constants to global symbol table");
    }
}

void TBlock::setDisplayNeeded () {
    displayNeeded = true;
}

bool TBlock::isDisplayNeeded () const {
    return displayNeeded;
}

TExpressionBase *TBlock::searchActiveRecords (const std::string &identifier) {
    for (std::vector<TExpressionBase *>::reverse_iterator it = activeRecords.rbegin (); it != activeRecords.rend (); ++it) {
        const TRecordType *type = static_cast<TRecordType *> ((*it)->getType ());
        if (TSymbol *s = type->searchComponent (identifier))
            return compiler.createMemoryPoolObject<TRecordComponent> (*it, identifier, s);
    }
    return nullptr;
}

void TBlock::pushActiveRecord (TExpressionBase *expr) {
    activeRecords.push_back (expr);
}

void TBlock::popActiveRecord () {
    activeRecords.pop_back ();
}

void TBlock::parseDeclarationList (std::vector<std::string> &identifiers, TType *&type, bool allowGenericVar) {
    bool goon = true;
    identifiers.clear ();
    do {
        if (lexer.getToken () == TToken::Identifier) {
            identifiers.push_back (lexer.getIdentifier ());
            lexer.getNextToken ();
            goon = lexer.checkToken (TToken::Comma);
        } else {
            compiler.errorMessage (TCompilerImpl::IdentifierExpected);
            goon = false;
        }
    } while (goon);
    
    if (allowGenericVar && lexer.getToken () != TToken::Colon)
        type = &stdType.GenericVar;
    else {
        compiler.checkToken (TToken::Colon, "':' expected in declaration");
        type = parseType ();
    }
}

void TBlock::parseExternalDeclaration (std::string &libName, std::string &symbolName) {
    if (lexer.getToken () != TToken::Semicolon) {
        if (!checkSymbolName (symbolName)) {
            const TSimpleConstant *lib = parseConstExpression ();
            if (lib->getType () == &stdType.String)
                libName = lib->getString ();
            // TODO: error if not string
            checkSymbolName (symbolName);
#ifdef CREATE_9900
            const std::string s = compiler.searchUnitPathes (libName);
            if (s.empty ())
                compiler.errorMessage (TCompilerImpl::FileNotFound, "Cannot find external file " + libName);
            else
                libName = s;
#endif            
        }
    }
}
    
bool TBlock::checkSymbolName (std::string &symbolName) {
    if (lexer.getToken () == TToken::Identifier && lexer.getIdentifier () == "name") {
        lexer.getNextToken ();
        const TSimpleConstant *name = parseConstExpression ();
        if (name->getType () != &stdType.String)
            compiler.errorMessage (TCompilerImpl::InvalidType, "External function name expected");
        else
            symbolName = name->getString ();
        return true;
    } else
        return false;
}

// Parse types

TType *TBlock::parseEnumerationType () {
    lexer.getNextToken ();
    std::vector<std::string> values;
    do {
        if (lexer.getToken () != TToken::Identifier)
            compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Identifier required in declaration of enumerated type");
        else {
            values.push_back (lexer.getIdentifier ());
            lexer.getNextToken ();
        }
    } while (lexer.checkToken (TToken::Comma));
    compiler.checkToken (TToken::BracketClose, "Missing ')' at end of enumeration");

    if (!values.empty ()) {
        std::size_t size = values.size () - 1;
        TEnumeratedType *type = compiler.createMemoryPoolObject<TEnumeratedType> ("enum", 0, size);
        for (std::size_t i = 0; i <= size; ++i) 
            if (symbols->addConstant (values [i], compiler.createMemoryPoolObject<TSimpleConstant> (static_cast<std::int64_t> (i), type)).alreadyPresent)
                compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "'" + values [i] + "' is already declared in the current block");
        return type;       
    }
    return nullptr;
}

TType *TBlock::parseArrayType () {
    lexer.getNextToken ();
    compiler.checkToken (TToken::SquareBracketOpen, "'[' required in array declaration");
        
    std::stack<TEnumeratedType *> indexTypes;
    do {
        TType *indexType = parseType ();
        if (indexType && indexType->isEnumerated ())
            indexTypes.push (static_cast<TEnumeratedType *> (indexType));
        else
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Ordinal type required as array index");
    } while (lexer.checkToken (TToken::Comma));
    
    compiler.checkToken (TToken::SquareBracketClose, "']' required in array declaration");
    compiler.checkToken (TToken::Of, "'of' required in array declaration");
        
    TType *type = parseType ();
    if (type)
        while (!indexTypes.empty ()) {
            type = compiler.createMemoryPoolObject<TArrayType> (type, indexTypes.top ());
            indexTypes.pop ();
        }
    return type;
}

TType *TBlock::parseShortString () {
    std::size_t length = 255;
    lexer.getNextToken ();
    if (lexer.checkToken (TToken::SquareBracketOpen)) {
        const TSimpleConstant *size = parseConstExpression ();
        compiler.checkToken (TToken::SquareBracketClose, "']' required in shortstring declaration");
        if (size->getType () != &stdType.Int64)
            compiler.errorMessage (TCompilerImpl::InvalidType, "Need integer constant for shortstring length");
        length = size->getInteger ();
        if (length < 1 || length > 255)
            compiler.errorMessage (TCompilerImpl::InvalidType, "Invalid shortstring length (must be 1..255)");
    }
    if (length == 255)
        return &stdType.ShortString;
    else
        return compiler.createMemoryPoolObject<TShortStringType> (
            compiler.createMemoryPoolObject<TSubrangeType> (std::string (), &stdType.Int64, 0, length));
}

void TBlock::parseRecordFieldList (TRecordType *recordType) {
    std::vector<std::string> identifiers;
    do {
        TType *type = nullptr;
        if (lexer.getToken () != TToken::End && lexer.getToken () != TToken::Case) {
            parseDeclarationList (identifiers, type, false);
            if (type)
                for (const std::string &s: identifiers)
                    if (recordType->addComponent (s, type).alreadyPresent)
                        compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "Record component '" + s + "' already declared");
        }
    } while (lexer.checkToken (TToken::Semicolon));
    if (lexer.checkToken (TToken::Case))
        parseRecordVariantPart (recordType);
}

void TBlock::parseRecordVariantPart (TRecordType *recordType) {
    std::string identifier;
    TType *type = nullptr;
    if (lexer.getToken () == TToken::Identifier) {
        identifier = lexer.getIdentifier ();
        lexer.getNextToken ();
        if (!lexer.checkToken (TToken::Colon)) {
            TSymbol *s = symbols->searchSymbol (identifier, TSymbol::NamedType);
            if (!s) 
                compiler.errorMessage (TCompilerImpl::InvalidType, "'" + identifier + "' is not a type");
            else
                type = s->getType ();
        }
    }
    if (!type) {
        type = parseType ();
        if (type)
            if (recordType->addComponent (identifier, type).alreadyPresent)
                 compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "Record component '" + identifier + "' already declared");
    }
    if (type) {
        if (!type->isEnumerated ())
            compiler.errorMessage (TCompilerImpl::InvalidType, "Enumerated type required for tag of variant");
        else if (type->isSubrange ())
            type = type->getBaseType ();
    }
    compiler.checkToken (TToken::Of, "'of' expected in variant declaration");
    do {
        if (lexer.getToken () != TToken::End) {
            do {
                const TSimpleConstant *c = parseConstExpression ();
                if (c && c->getType () != type)
                    compiler.errorMessage (TCompilerImpl::InvalidType, "Invalid type used in tag value");
            } while (lexer.checkToken (TToken::Comma));
            compiler.checkToken (TToken::Colon, "':' expected in variant declaration");
            compiler.checkToken (TToken::BracketOpen, "'(' expected in variant declaration");
            recordType->enterVariant (compiler.createMemoryPoolObject<TSymbolList> (nullptr, compiler.getMemoryPoolFactory ()));
            parseRecordFieldList (recordType);
            recordType->leaveVariant ();
            compiler.checkToken (TToken::BracketClose, "')' expected in variant declaration");
        }
    } while (lexer.checkToken (TToken::Semicolon));
}

TType *TBlock::parseRecordType () {
    lexer.getNextToken ();
    TRecordType *recordType = compiler.createMemoryPoolObject<TRecordType> (compiler.createMemoryPoolObject<TSymbolList> (nullptr, compiler.getMemoryPoolFactory ()));
    
    if (lexer.getToken () != TToken::End)
        parseRecordFieldList (recordType);
    compiler.checkToken (TToken::End, "'end' required at end of record declaration");
    
    return recordType;
}

TType *TBlock::parseSetType () {
    lexer.getNextToken ();
    compiler.checkToken (TToken::Of, "'of' expected in set declaration");
    TType *type = parseType ();
    if (type) {
        if (type->isEnumerated ()) {
            TEnumeratedType *enumType = static_cast<TEnumeratedType *> (type);
            if (enumType->getMinVal () >= 0 && enumType->getMaxVal () < static_cast<std::int64_t> (TConfig::setwords * 8 * sizeof (std::int64_t)))
                return compiler.createMemoryPoolObject<TSetType> (enumType);
            else
                compiler.errorMessage (TCompilerImpl::InvalidType, "Base type of set is too large");
        } else
            compiler.errorMessage (TCompilerImpl::InvalidType, "Need enumerated type for set declaration");
    } 
    return nullptr;
}

TType *TBlock::parseFileType () {
    TType *baseType = nullptr;
    lexer.getNextToken ();
    if (lexer.checkToken (TToken::Of))
        baseType = parseType ();
    if (baseType && !baseType->isSerializable ())
        compiler.errorMessage (TCompilerImpl::InvalidType, "Invalid type used for file");
    if (baseType)
        return compiler.createMemoryPoolObject<TFileType> (baseType, compiler.createMemoryPoolObject<TSymbolList> (nullptr, compiler.getMemoryPoolFactory ()));
    else
        return symbols->searchSymbol (TConfig::binFileType, TSymbol::NamedType)->getType ();
}

TType *TBlock::parseRoutineType (bool isFunction, bool farCall) {
    TSymbolList parameters (nullptr, compiler.getMemoryPoolFactory ());
    parseParameterDeclaration (parameters);
    
    TType *returnType = &stdType.Void;
    if (isFunction) {
        compiler.checkToken (TToken::Colon, "':' expected in function declaration");
        returnType = parseType ();
        if (!returnType)
            // avoid invalid entry in symbol table
            returnType = &stdType.Void;
    }
    return compiler.createMemoryPoolObject<TRoutineType> (std::move (parameters), returnType, farCall);
}

TType *TBlock::parseFunctionType () {
    lexer.getNextToken ();
    return parseRoutineType (true);
}

TType *TBlock::parseProcedureType () {
    lexer.getNextToken ();
    return parseRoutineType (false);
}

TType *TBlock::parsePointerType () {
    lexer.getNextToken ();
    TPointerType *result = nullptr;
    if (parsingTypeDeclaration && lexer.getToken () == TToken::Identifier) {
        result = compiler.createMemoryPoolObject<TPointerType> (lexer.getIdentifier ());
        unresolvedPointerTypes.push_back (result);
        lexer.getNextToken ();
    } else 
        if (TType *type = parseType ())
            result = compiler.createMemoryPoolObject<TPointerType> (type);
    return result;
}

TType *TBlock::parseSubrangeType () {
    const TSimpleConstant *begin = parseConstExpression ();
    compiler.checkToken (TToken::Points, "'..' required in subrange declaration");
    const TSimpleConstant *end = parseConstExpression ();

    TType *result = nullptr;
    if (begin->isValid () && end->isValid ()) {
        if (begin->getType () != end->getType ())
            compiler.errorMessage (TCompilerImpl::InvalidType, "Types in subrange declaration must be the same");
        else if (!begin->getType ()->isEnumerated ())
            compiler.errorMessage (TCompilerImpl::InvalidType, "Need enumerated type for subrange declaration");
        else
            result = compiler.createMemoryPoolObject<TSubrangeType> (std::string (), begin->getType (), begin->getInteger (), end->getInteger ());
    }
    return result;
}

TType *TBlock::parseVectorType () {
    lexer.getNextToken ();
    compiler.checkToken (TToken::Of, "'of' expected in vector declaration");
    if (TType *type = parseType ())
        return compiler.createMemoryPoolObject<TVectorType> (type);
    else
        return nullptr;
}

TType *TBlock::parseTypeIdentifier () {
    const std::string s = lexer.getIdentifier ();
    if (symbols->searchSymbol (s, TSymbol::Constant))
        return parseSubrangeType ();
        
    lexer.getNextToken ();
    if (TSymbol *symbol = symbols->searchSymbol (s, TSymbol::NamedType))
        return symbol->getType ();
        
    compiler.errorMessage (TCompilerImpl::TCompilerImpl::InvalidType, "'" + s + "' is not a type");
    return nullptr;
}

TType *TBlock::parseType () {
    typedef std::unordered_map<TToken, TType *(TBlock::*) ()> TDispatcherMap;

    static const TDispatcherMap dispatcher = {
      { TToken::BracketOpen,	&TBlock::parseEnumerationType },
      { TToken::Array,		&TBlock::parseArrayType },
      { TToken::ShortString,    &TBlock::parseShortString },
      { TToken::Record,		&TBlock::parseRecordType },
      { TToken::Set,		&TBlock::parseSetType },
      { TToken::File,           &TBlock::parseFileType },
      { TToken::Function,	&TBlock::parseFunctionType },
      { TToken::Procedure,	&TBlock::parseProcedureType },
      { TToken::Dereference,	&TBlock::parsePointerType },
      { TToken::IntegerConst,	&TBlock::parseSubrangeType },
      { TToken::CharConst,      &TBlock::parseSubrangeType },
      { TToken::Sub,		&TBlock::parseSubrangeType },
      { TToken::Vector,		&TBlock::parseVectorType },
      { TToken::Identifier,	&TBlock::parseTypeIdentifier }
    };
    
    TDispatcherMap::const_iterator it = dispatcher.find (lexer.getToken ());
    if (it != dispatcher.end ()) {
        TType *type = (this->*(it->second)) ();
        if (type)
            compiler.getCodeGenerator ().alignType (type);
        return type;
    } else {
        compiler.errorMessage (TCompilerImpl::TCompilerImpl::InvalidType);
        return nullptr;
    }
}

void TBlock::parseVarParameterDeclaration (TSymbolList &symbolList, bool createReference, bool isParameter) {
    std::vector<std::string> identifiers;
    TType *type;
    parseDeclarationList (identifiers, type, createReference && isParameter);
    
    if (type) {
        if (createReference) //  || (isParameter && compiler.getCodeGenerator ().isReferenceCallerCopy (type)))
            type = compiler.createMemoryPoolObject<TReferenceType> (type);
            
        TSymbol *aliasSymbol = nullptr;
        const TSimpleConstant *absoluteAddress = nullptr;
        if (lexer.checkToken (TToken::Absolute)) {
            if (isParameter)
                compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "'absolute' cannot be used in parameter declaration");
            else if (lexer.getToken () == TToken::Identifier)
                aliasSymbol = checkVariable ();
            else {
                absoluteAddress = parseConstExpression ();
                if (absoluteAddress) {
                    if (absoluteAddress->getType () != &stdType.Int64)
                        compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "'absolute' required variable or integer constant");
                    else if (symbolList.getLevel () != 1)
                        compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "'absolute' address can only be used with global variable");
                }
            }                
        }
        for (const std::string &s: identifiers) {
            TSymbolList::TAddSymbolResult result;
            if (aliasSymbol)
                result = symbolList.addAlias (s, type, aliasSymbol);
            else if (absoluteAddress)
                result = symbolList.addAbsolute (s, type, absoluteAddress->getInteger ());
            else if (isParameter)
                result = symbolList.addParameter (s, type);
            else
                result = symbolList.addVariable (s, type);
            if (result.alreadyPresent)
                compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "`" + s + "' is already declared in the current block");
        }
    }
}

void TBlock::parseParameterDeclaration (TSymbolList &symbolList) {
    if (lexer.checkToken (TToken::BracketOpen)) {
        do {
            bool referenceParameter = lexer.checkToken (TToken::Var);
            parseVarParameterDeclaration (symbolList, referenceParameter, true);
        } while (lexer.checkToken (TToken::Semicolon));
        compiler.checkToken (TToken::BracketClose, "Parameter list must be closed with ')'");
    }
}

void TBlock::parseLabelDeclaration () {
    if (!isUnitInterface) {
        std::string s;
        do {
            if (lexer.getToken () == TToken::Identifier)
                s = lexer.getIdentifier ();
            else if (lexer.getToken () == TToken::IntegerConst)
                s = std::to_string (lexer.getInteger ());
            else
                compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Illegal label declaration: identifier or integer expected");
            lexer.getNextToken ();
            if (symbols->addLabel (s).alreadyPresent)
                compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "`" + s + "' is already declared in the current block");
        } while (lexer.checkToken (TToken::Comma));
        if (lexer.checkToken (TToken::Semicolon))
            return;
        compiler.errorMessage (TCompilerImpl::SyntaxError, "Label declartion must be terminated with ';'");
    } else
        compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "Label declarations not allowed in interface section of unit");
    compiler.recoverPanicMode ({TToken::Semicolon, TToken::Var, TToken::Const, TToken::Type, TToken::Label, TToken::Procedure, TToken::Function, TToken::Begin, TToken::Terminator});
    lexer.checkToken (TToken::Semicolon);
}

void TBlock::parseConst (const std::string &identifier) {
    const TSimpleConstant *c = parseConstExpression ();
    if (c->getType ())
        if (symbols->addConstant (identifier, c).alreadyPresent)
            compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "`" + identifier + "' is already declared in the current block");
}

const TSimpleConstant *TBlock::parseSimpleConstant (TType *type) {
    const TSimpleConstant *c = parseConstExpression ();
    
    TType *basetype = type;
    while (basetype->isSubrange () || basetype->isSet ())
        basetype = basetype->getBaseType ();
        
        
    if ((basetype->isReal () || basetype->isSingle ()) && (c->getType () == &stdType.Int64 || c->getType ()->isReal ()))
        c = compiler.createMemoryPoolObject<TSimpleConstant> (c->getDouble (), basetype);
    else if ((basetype->isShortString () || basetype == &stdType.String) && c->getType () == &stdType.Char)
        c = compiler.createMemoryPoolObject<TSimpleConstant> (std::string (1, c->getChar ()), &stdType.String);
    else if (basetype->isRoutine () && c->getType ()->isRoutine () && static_cast<const TRoutineType *> (basetype)->matchesOverload (static_cast<TRoutineType *> (c->getType ()))) {
        TRoutineValue *rval = compiler.createMemoryPoolObject <TRoutineValue> (c->getString (), *this);
        if (rval->resolveConversion (static_cast<const TRoutineType *> (basetype)))
            return compiler.createMemoryPoolObject<TSimpleConstant> (rval); // rval.getSymbol ()->getOverloadName (), basetype);
        else 
            compiler.errorMessage (TCompilerImpl::InvalidType, "Cannot convert routine '" + c->getString () + "' to type " + basetype->getName ());
    } else if (type->isPointer () && c->getType ()->isPointer () && c->getType ()->getBaseType () == &stdType.Void) {
        TSimpleConstant *result = compiler.createMemoryPoolObject<TSimpleConstant> (*c);
        result->setType (type);
        c = result;
    } else if (c->getType () != basetype && !(c->getType ()->isSet () && (!c->getType ()->getBaseType () || c->getType ()->getBaseType () == basetype))  &&
               !(type->isShortString () && c->getType () == &stdType.String ))
        compiler.errorMessage (TCompilerImpl::InvalidType, "Expected constant of type '" + type->getName () + "' but got '" + c->getType ()->getName () + "'");
    else {
        TSimpleConstant *result = compiler.createMemoryPoolObject<TSimpleConstant> (*c);
        result->setType (type);
        c = result;
        // TODO: Range check !!!!
    }
        
    return c;
}

const TArrayConstant *TBlock::parseArrayConstant (TArrayType *type) {
    TType *baseType = type->getBaseType ();
    TEnumeratedType *indexType = type->getIndexType ();
    compiler.checkToken (TToken::BracketOpen, "Expected '(' at begin of typed array constant");
    TArrayConstant *arrayConstant = compiler.createMemoryPoolObject<TArrayConstant> (type);
    do 
        arrayConstant->addValue (parseTypedConstant (baseType));
    while (lexer.checkToken (TToken::Comma));
    compiler.checkToken (TToken::BracketClose, "Expected ')' at end of typed array constant");
    if (static_cast<std::int64_t> (arrayConstant->getValues ().size ()) != indexType->getMaxVal () - indexType->getMinVal () + 1)
        compiler.errorMessage (TCompilerImpl::InvalidNumberInitializers, "Number of array initializers does not match array size");
    return arrayConstant;
}

const TRecordConstant *TBlock::parseRecordConstant (TRecordType *type) {
    // TODO: initialize variant record !!!!
    const TSymbolList &recordMembers = *type->getRecordFields ().components;
    compiler.checkToken (TToken::BracketOpen, "Expected '(' at begin of typed record constant");
    TRecordConstant *recordConstant = compiler.createMemoryPoolObject<TRecordConstant> (type);
    do {
        std::string component;
        const TSymbol *s = nullptr;
        if (lexer.getToken () == TToken::Identifier) {
            component = lexer.getIdentifier ();
            s = recordMembers.searchSymbol (component);
            if (!s)
                compiler.errorMessage (TCompilerImpl::ComponentNotFound, "'" + component + "' is not a record member");
        } else
            compiler.errorMessage (TCompilerImpl::SyntaxError, "Field name expected");
        lexer.getNextToken ();
        compiler.checkToken (TToken::Colon, "':' expected in typed record constant");
        if (s)
            recordConstant->addValue (component, parseTypedConstant (s->getType ()));
        else
            lexer.getNextToken ();
    } while (lexer.checkToken (TToken::Semicolon));
    compiler.checkToken (TToken::BracketClose, "Expected ')' at end of typed record constant");
    return recordConstant;
}


const TConstant *TBlock::parseTypedConstant (TType *type) {
    if (type->isRecord ())
        return parseRecordConstant (dynamic_cast<TRecordType *> (type));
    if (type->isArray () && !type->isShortString ())
        return parseArrayConstant (dynamic_cast<TArrayType *> (type));
    return parseSimpleConstant (type);
}

void TBlock::parseTypedConst (const std::string &identifier) {
    TType *type = parseType ();
    compiler.checkToken (TToken::Equal, "'=' required in typed const declaration");
    if (type)
        if (const TConstant *c = parseTypedConstant (type)) 
            if (c->getType ())
                if (symbols->addStaticVariable (identifier, c).alreadyPresent)
                    compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "`" + identifier + "' is already declared in the current block");
}

void TBlock::parseConstDeclaration () {
    do {
        if (lexer.getToken () != TToken::Identifier)
            compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Const declaration must start with an identifier");
        else {
            const std::string name = lexer.getIdentifier ();
            lexer.getNextToken ();
            if (lexer.checkToken (TToken::Equal))
                parseConst (name);
            else if (lexer.checkToken (TToken::Colon))
                parseTypedConst (name);
            else
                compiler.errorMessage (TCompilerImpl::SyntaxError, "'=' or ':' required in const declaration");
        }
        if (!lexer.checkToken (TToken::Semicolon)) {
            compiler.errorMessage (TCompilerImpl::SyntaxError,  "Const declaration must be terminated with ';'");
            compiler.recoverPanicMode ({TToken::Semicolon, TToken::Var, TToken::Const, TToken::Type, TToken::Label, TToken::Procedure, TToken::Function, TToken::Begin});
            lexer.checkToken (TToken::Semicolon);
        }
    } while (lexer.getToken () == TToken::Identifier);
}

void TBlock::parseTypeDeclaration () {
    parsingTypeDeclaration = true;
    do {
        bool startRecovery = false;
        if (lexer.getToken () != TToken::Identifier) 
            compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Type declaration must start with an identifier");
        else {
            const std::string name = lexer.getIdentifier ();
            lexer.getNextToken ();
            compiler.checkToken (TToken::Equal, "'=' required in type declaration");
            if (TType *type = parseType ())
                if (symbols->addNamedType (name, type).alreadyPresent)
                    compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "`" + name + "' is already declared in the current block");
        }
        if (!lexer.checkToken (TToken::Semicolon))
            startRecovery = true;
        // eat cdecls - TODO: check if proc/func type
        if (lexer.checkToken (TToken::CDecl) && !lexer.checkToken (TToken::Semicolon)) 
            startRecovery = true;
        if (startRecovery) {
            compiler.errorMessage (TCompilerImpl::SyntaxError,  "Type declaration must be terminated with ';'");
            compiler.recoverPanicMode ({TToken::Semicolon, TToken::Var, TToken::Const, TToken::Type, TToken::Label, TToken::Procedure, TToken::Function, TToken::Begin});
            lexer.checkToken (TToken::Semicolon);
        }
    } while (lexer.getToken () == TToken::Identifier);
    parsingTypeDeclaration = false;

    for (TPointerType *pointerType: unresolvedPointerTypes) {
        pointerType->setBaseType (&stdType.Void);
        if (TSymbol *symbol = symbols->searchSymbol (pointerType->getBaseName (), TSymbol::NamedType)) {
            TType *baseType = symbol->getType (), *t = baseType;
            while (t && t->isPointer () && t != pointerType)
                t = t->getBaseType ();
            if (t != pointerType)
                pointerType->setBaseType (baseType);
            else
                compiler.errorMessage (TCompilerImpl::InvalidType, "Forward pointer to '" + pointerType->getBaseName () + "' has cyclic definition");
        } else
            compiler.errorMessage (TCompilerImpl::InvalidType, "Forward pointer to '" + pointerType->getBaseName () + "' is unresolved");
    }
    unresolvedPointerTypes.clear ();
}

void TBlock::parseVarDeclaration () {
    do {
        parseVarParameterDeclaration (*symbols, false, false);
        if (!lexer.checkToken (TToken::Semicolon)) {
            compiler.errorMessage (TCompilerImpl::SyntaxError, "Declaration must be terminated with ';'");
            compiler.recoverPanicMode ({TToken::Semicolon, TToken::Var, TToken::Const, TToken::Type, TToken::Label, TToken::Procedure, TToken::Function, TToken::Begin, TToken::Terminator});
            lexer.checkToken (TToken::Semicolon);
        }
    } while (lexer.getToken () == TToken::Identifier);
}

void TBlock::parseSubroutine (bool isFunction) {
    std::string identifier, symbolName, libName;
    if (lexer.getToken () != TToken::Identifier) {
        compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Name of subroutine is missing");
        identifier = "$Missing" + std::to_string (symbols->size ());
    } else {
        identifier = lexer.getIdentifier ();	// lower case
        symbolName = lexer.getString ();	// case sensitive name
    }
    
    lexer.getNextToken ();
    TRoutineType *routineType = static_cast<TRoutineType *> (parseRoutineType (isFunction, symbols->getLevel () == 1));
    compiler.checkAndSynchronize (TToken::Semicolon, "';' expected at end of subroutine header");
    
    bool isForward = false, isExport = false, isExternal = false, isAssembler = false;
    if (lexer.checkToken (TToken::CDecl))
        compiler.checkToken (TToken::Semicolon, "';' expected after 'cdecl' declaration");
    if (lexer.checkToken (TToken::Overload))
        compiler.checkToken (TToken::Semicolon, "';' expected after 'overload' declaration");
    if (lexer.checkToken (TToken::Forward))
        isForward = true;
    else if (lexer.checkToken (TToken::Export))
        isExport = true;
    else if (lexer.checkToken (TToken::External)) {
        isExternal = true;
        parseExternalDeclaration (libName, symbolName);
    } else if (lexer.checkToken (TToken::Assembler))
        isAssembler = true;
    if (isForward || isExport || isExternal || isAssembler)
        compiler.checkToken (TToken::Semicolon, "';' expected at end of subroutine header");
    
    TSymbolList::TAddSymbolResult result = symbols->addRoutine (identifier, routineType);
    TSymbol *symbol = result.symbol;
    if (result.alreadyPresent) {
        if (!symbol->checkSymbolFlag (TSymbol::Forward) || isUnitInterface)
            compiler.errorMessage (TCompilerImpl::IdentifierAlreadyDeclared, "'" + identifier + "' is already declared in the current scope");
        else if (!(routineType->parameterEmpty () && routineType->getReturnType () == &stdType.Void) && 
                 !routineType->matchesForward (static_cast<TRoutineType *> (symbol->getType ())))
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Routine header does not match forward declaration");
    } 
    if (!result.alreadyPresent || !symbol->checkSymbolFlag (TSymbol::Routine)) {
        symbol->setType (routineType);
        TSymbolList *subRoutineSymbols = compiler.createMemoryPoolObject<TSymbolList> (symbols, compiler.getMemoryPoolFactory ());
        TExpressionBase *returnLValueDeref = nullptr;
        if (isFunction) {
            TType *retType = routineType->getReturnType ();
            TSymbolList::TAddSymbolResult result;
            TExpressionBase *resultLValue;
            const std::string retTempName = "$f_ret_" + identifier;
            if (getCompiler ().getCodeGenerator ().classifyReturnType (retType) == TCodeGenerator::TReturnLocation::Reference) {
                result = subRoutineSymbols->addParameter (retTempName, compiler.createMemoryPoolObject<TReferenceType> (retType));
                resultLValue = compiler.createMemoryPoolObject<TReferenceVariable> (result.symbol, *this);
            } else {
                result = subRoutineSymbols->addVariable (retTempName, retType);
                resultLValue = compiler.createMemoryPoolObject<TVariable> (result.symbol, *this);
            }
            returnLValueDeref = compiler.createMemoryPoolObject<TLValueDereference> (resultLValue);
        }
        for (const TSymbol *parameter: routineType->getParameter ())
            if (compiler.getCodeGenerator ().isReferenceCallerCopy (parameter->getType ()))
                subRoutineSymbols->addParameter (parameter->getName (), compiler.createMemoryPoolObject<TReferenceType> (parameter->getType ()));
            else
                subRoutineSymbols->addParameter (parameter->getName (), parameter->getType ());
        TBlock *block = compiler.createMemoryPoolObject<TBlock> (compiler, subRoutineSymbols, symbol, this);
        block->returnLValueDeref = returnLValueDeref;
        symbol->setBlock (block);
    }
    
    if (isExternal) {
        symbol->removeSymbolFlags (TSymbol::Forward);
        symbol->setExternal (libName, symbolName);
    } else if (isUnitInterface) {
        symbol->addSymbolFlags (TSymbol::Forward);
        if (isForward) 
            compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "'forward' declaration not allowed in interface section of unit");
    } else if (isForward) {
        if (symbol->checkSymbolFlag (TSymbol::Forward))
            compiler.errorMessage (TCompilerImpl::SyntaxError, "Subroutine " + identifier + " already declared forward");
        else
            symbol->addSymbolFlags (TSymbol::Forward);
    } else {
        symbol->removeSymbolFlags (TSymbol::Forward);
        if (isAssembler) {
            symbol->setExternal (libName, symbolName);
            if (T9900Generator *gen = dynamic_cast<T9900Generator *> (&getCompiler ().getCodeGenerator ()))
                gen->parseAssemblerBlock (symbol, *this);
            else {
                compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "'assembler' declaration not supported for target architecture");
                compiler.recoverPanicMode ({TToken::End});
            }
        } else {
            if (isExport)
                symbol->addSymbolFlags (TSymbol::Export);
            symbol->getBlock ()->parse ();
        }
        compiler.checkToken (TToken::Semicolon, "';' expected after subroutine");
    }
}

void TBlock::parseProcedure () {
    parseSubroutine (false);
}

void TBlock::parseFunction () {
    parseSubroutine (true);
}

void TBlock::parseDeclarations (bool isInterface) {
    using TDispatcherMap = std::unordered_map<TToken, void (TBlock::*) ()>;
    
    static const TDispatcherMap dispatcher = {
      { TToken::Label,		&TBlock::parseLabelDeclaration },
      { TToken::Const,		&TBlock::parseConstDeclaration },
      { TToken::Type,		&TBlock::parseTypeDeclaration },
      { TToken::Var,		&TBlock::parseVarDeclaration },
      { TToken::Function,	&TBlock::parseFunction },
      { TToken::Procedure,	&TBlock::parseProcedure }
    };

    isUnitInterface = isInterface;
    TDispatcherMap::const_iterator it = dispatcher.find (lexer.getToken ());
    while (it != dispatcher.end ()) {
        lexer.getNextToken ();
        (this->*(it->second)) ();
        it = dispatcher.find (lexer.getToken ());
    }
}

void TBlock::parse () {
    parseDeclarations ();
    if (lexer.getToken () != TToken::Begin)
        compiler.errorMessage (TCompilerImpl::SyntaxError, "'begin' expected");
    else {
        statements = TStatement::parse (*this);
        checkDefinitions ();
    }
}

void TBlock::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}

// ---

TUnit::TUnit (TCompilerImpl &compiler, TSymbolList *predefinedSymbols):
  compiler (compiler),
  predefinedSymbols (predefinedSymbols),
  initializationStatement (nullptr),
  finalizationStatement (nullptr) {
}

void TUnit::parseHeader () {
    TLexer &lexer = compiler.getLexer ();
    compiler.checkToken (TToken::Unit, "A unit must start with 'unit'");
    if (lexer.getToken () != TToken::Identifier)
        compiler.errorMessage (TCompilerImpl::SyntaxError, "Unit name expected");
    else
        unitname = lexer.getIdentifier ();
    lexer.getNextToken ();
    
    compiler.checkToken (TToken::Semicolon, "';' expected");
    compiler.checkToken (TToken::Interface, "'interface' expected");
    
    publicUnits = compiler.createMemoryPoolObject<TUsesDeclaration> (compiler);
    importedSymbols = publicUnits->getPublicSymbols (predefinedSymbols);
    
    allSymbols = compiler.createMemoryPoolObject<TSymbolList> (importedSymbols, compiler.getMemoryPoolFactory (), importedSymbols->getLevel ());
    TBlock (compiler, allSymbols, nullptr, nullptr).parseDeclarations (true);	// TODO: own symbol?
    
    publicSymbols = compiler.createMemoryPoolObject<TSymbolList> (importedSymbols, compiler.getMemoryPoolFactory (), importedSymbols->getLevel ());
    allSymbols->copySymbols (TSymbol::AllSymbols, *publicSymbols);
}

TStatement *TUnit::parseInitFinal (const std::string funcName, TBlock &declarations) {
    TSymbolList parameters (nullptr, compiler.getMemoryPoolFactory ());
    TRoutineType *t = compiler.createMemoryPoolObject<TRoutineType> (std::move (parameters), &stdType.Void, true);
    TSymbol *initSymbol = allSymbols->addRoutine (funcName, t).symbol;
    initSymbol->setBlock (compiler.createMemoryPoolObject<TBlock> (compiler, compiler.createMemoryPoolObject<TSymbolList> (allSymbols, compiler.getMemoryPoolFactory ()), initSymbol, &declarations));
    initSymbol->getBlock ()->parse ();
    compiler.getLexer ().checkToken (TToken::Semicolon);
    return compiler.createMemoryPoolObject<TRoutineCall> (
        TExpressionBase::createRuntimeCall (funcName, &stdType.Void, {}, declarations, false));
}

void TUnit::parseImplementation () {    
    TLexer &lexer = compiler.getLexer ();
    compiler.checkToken (TToken::Implementation, "'implementation' expected");
    
    TUsesDeclaration *privateUnits = compiler.createMemoryPoolObject<TUsesDeclaration> (compiler);
    allSymbols->setPreviousLevel (privateUnits->getPublicSymbols (importedSymbols, importedSymbols->getLevel ()));
    
    TBlock declarations (compiler, allSymbols, nullptr, nullptr);	// TODO: own symbol?
    declarations.parseDeclarations ();
    
    bool initializationTokenPresent = lexer.checkToken (TToken::Initialization),
         oldInitCodePresent = false;
    if (!initializationTokenPresent)
        oldInitCodePresent = lexer.getToken () == TToken::Begin;
    if (initializationTokenPresent || oldInitCodePresent)
        initializationStatement = parseInitFinal ("$" + unitname + "_init", declarations);
    if (!oldInitCodePresent) {
        if (lexer.checkToken (TToken::Finalization))
            finalizationStatement = parseInitFinal ("$" + unitname + "_final", declarations);
        compiler.checkToken (TToken::End, "'end' expected");
    }

    compiler.checkToken (TToken::Point, "'.' expected at end of unit");
    compiler.checkToken (TToken::Terminator, "Input after end of unit");
    declarations.checkDefinitions ();
}

TStatement *TUnit::getInitializationStatement () const {
    return initializationStatement;
}

TStatement *TUnit::getFinalizationStatement () const {
    return finalizationStatement;
}

TSymbolList *TUnit::getPublicSymbols () const {
    return publicSymbols;
}

TSymbolList *TUnit::getAllSymbols () const {
    return allSymbols;
}

TUsesDeclaration *TUnit::getPublicUnits () const {
    return publicUnits;
}

void TUnit::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}

// ---

TUsesDeclaration::TUsesDeclaration (TCompilerImpl &compiler):
  compiler (compiler) {
    parse ();
}

void TUsesDeclaration::parse () {
    TLexer &lexer = compiler.getLexer ();
    if (lexer.checkToken (TToken::Uses)) {
        do {
            if (lexer.getToken () != TToken::Identifier)
                compiler.errorMessage (TCompilerImpl::SyntaxError, "Unit name expected");
            else
                unitnames.push_back (lexer.getIdentifier ());
            lexer.getNextToken ();
        } while (lexer.checkToken (TToken::Comma));
        
        if (!lexer.checkToken (TToken::Semicolon)) {
            compiler.errorMessage (TCompilerImpl::SyntaxError, "';' expected at end of 'uses' declaration");
            compiler.recoverPanicMode ({TToken::Semicolon, TToken::Const, TToken::Type, TToken::Label, TToken::Var, TToken::Function,
                               TToken::Procedure, TToken::Begin, TToken::Forward});
            lexer.checkToken (TToken::Semicolon);
        }
    }
    for (const std::string &s: unitnames)
        compiler.loadUnit (s);
}

TSymbolList *TUsesDeclaration::getPublicSymbols (TSymbolList *previousSymbols) const {
    return getPublicSymbols (previousSymbols, previousSymbols->getLevel () + 1);
}

TSymbolList *TUsesDeclaration::getPublicSymbols (TSymbolList *previousSymbols, std::size_t level) const {
    TSymbolList *unitSymbols = compiler.createMemoryPoolObject<TSymbolList> (previousSymbols, compiler.getMemoryPoolFactory (), level);
    for (TUnit *unit: getUnitsTransitive ()) {
        unit->getPublicSymbols ()->copySymbols (TSymbol::AllSymbols, *unitSymbols);
        unitSymbols = compiler.createMemoryPoolObject<TSymbolList> (unitSymbols, compiler.getMemoryPoolFactory (), level);
    }
    // TODO: remove empty layer at end of list
    return unitSymbols;
}        

std::vector<TUnit *> TUsesDeclaration::getUnitsTransitive () const {
    std::vector<TUnit *> result;
    if (TUnit *systemUnit = compiler.getSystemUnit ())
        result.push_back (systemUnit);
    appendUsedUnits (result);
    return result;
}

void TUsesDeclaration::appendUsedUnits (std::vector<TUnit *> &result) const {
    for (const std::string &s: unitnames) {
        if (TUnit *unit = compiler.loadUnit (s))
            if (std::find (result.begin (), result.end (), unit) == result.end ()) {
                result.push_back (unit);
                unit->getPublicUnits ()->appendUsedUnits (result);
            }
    }
}

// ---

TProgram::TProgram (TCompilerImpl &compiler, TSymbolList *predefinedSymbols):
  compiler (compiler), 
  predefinedSymbols (predefinedSymbols),
  block (nullptr),
  usedUnits (nullptr) {
}

void TProgram::parse () {
    TLexer &lexer = compiler.getLexer ();
    std::string programName;
    if (lexer.checkToken (TToken::Program)) {
        if (lexer.getToken () == TToken::Identifier) {
            programName = lexer.getIdentifier ();
            lexer.getNextToken ();
        } else
            compiler.errorMessage (TCompilerImpl::SyntaxError, "Program name expected");
        // ignore file list
        if (lexer.checkToken (TToken::BracketOpen)) {
            do 
                compiler.checkToken (TToken::Identifier, "Identifier expected");
            while (lexer.checkToken (TToken::Comma));
            compiler.checkToken (TToken::BracketClose, "')' expected");
        }
        compiler.checkToken (TToken::Semicolon, "';' expected");
    }
    TSymbolList::TAddSymbolResult result = predefinedSymbols->addVariable (programName, compiler.createMemoryPoolObject<TRoutineType> (TSymbolList (nullptr, compiler.getMemoryPoolFactory ()), &stdType.Void));
    
    usedUnits = compiler.createMemoryPoolObject<TUsesDeclaration> (compiler);
    TSymbolList *unitSymbols = usedUnits->getPublicSymbols (predefinedSymbols);
    TSymbolList *globalSymbols = compiler.createMemoryPoolObject<TSymbolList> (unitSymbols, compiler.getMemoryPoolFactory (), unitSymbols->getLevel ());
    
    block = compiler.createMemoryPoolObject<TBlock> (compiler, globalSymbols, result.symbol, nullptr);
    block->parse ();
    
    compiler.checkToken (TToken::Point, "'.' expected at end of program");
    compiler.checkToken (TToken::Terminator, "Input after end of program");
}

TBlock *TProgram::getBlock () const {
    return block;
}

std::vector<TUnit *> TProgram::getUnits () const {
    return allUnits;
}

void TProgram::appendUnit (TUnit *unit) {
    allUnits.push_back (unit);
    unit->getAllSymbols ()->copySymbols (TSymbol::AllSymbols, block->getSymbols ());
    // TODO generate and call init prodedure
    if (TStatement *statement = unit->getInitializationStatement ())
        static_cast<TStatementSequence *> (block->getStatements ())->appendFront (statement);
    if (TStatement *statement = unit->getFinalizationStatement ())
        static_cast<TStatementSequence *> (block->getStatements ())->appendBack (statement);
}

void TProgram::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}

// ---

TCompilerImpl::TCompilerImpl (TCodeGenerator &codeGenerator):
  memoryPoolFactory (1024 * 1024),
  codeGenerator (codeGenerator),
  systemUnit (nullptr),
  errorFlag (false) {
    lexerStack.push (&programLexer);
    predefinedSymbols = memoryPoolFactory.create<TSymbolList> (nullptr, memoryPoolFactory);
    createPredefinedSymbols ();
}

TCompilerImpl::~TCompilerImpl () {
}    

void TCompilerImpl::setSource (const std::string &s) {
    getLexer ().setSource (s);
}

void TCompilerImpl::setSource (std::string &&s) {
    getLexer ().setSource (std::move (s));
}

void TCompilerImpl::setFilename (const std::string &fn) {
    getLexer ().setFilename (fn);
}

TCompiler::TCompileResult TCompilerImpl::compile () {
    errorFlag = false;
    
    TUnit *unit = loadUnit ("system");
    systemUnit = unit;
    
    if (getLexer ().getToken () == TToken::Unit) {
        parseUnit ();
        return errorFlag ? TCompiler::Error : TCompiler::UnitCompiled;
    } else {
        parseProgram ();
        return errorFlag ? TCompiler::Error : TCompiler::ProgramCompiled;
    }
}

void TCompilerImpl::errorMessage (TCompilerImpl::TErrorType errorType, const std::string &description) {
    errorFlag = true;
    std::cerr << getLexer ().getLexerPosition ().getFilename () << ":" << getLexer ().getLexerPosition ().getLineNumber () << ":" << getLexer ().getLexerPosition ().getLinePosition () << ": error: " << description << std::endl;
}

void TCompilerImpl::recoverPanicMode (const std::vector<TToken> &syncTokens) {
    bool found = false;
    do {
        TToken token = getLexer ().getToken ();
        found = token == TToken::Terminator || std::find (syncTokens.begin (), syncTokens.end (), token) != syncTokens.end ();
        if (!found)
            getLexer ().getNextToken ();
    } while (!found);
}

void TCompilerImpl::checkToken (TToken t, const std::string &msg) {
    if (!getLexer ().checkToken (t))
        errorMessage (SyntaxError, msg);
}

void TCompilerImpl::checkAndSynchronize (TToken t, const std::string &msg) {
    if (!getLexer ().checkToken (t)) {
        errorMessage (TCompilerImpl::SyntaxError, msg);
        recoverPanicMode ({TToken::Semicolon, TToken::Const, TToken::Type, TToken::Label, TToken::Var, TToken::Function,
                          TToken::Procedure, TToken::Begin, TToken::Forward});
        getLexer ().checkToken (t);
    }
}

void TCompilerImpl::setUnitSearchPathes (const std::vector<std::string> &pathes) {
    unitSearchPathes = pathes;
}

std::string TCompilerImpl::searchUnitPathes (const std::string &s) {
    for (const std::string &path: unitSearchPathes) {
        const std::string fn = path + '/' + s;
        if (std::filesystem::exists (fn))
            return fn;
    }
    return std::string ();
}

TUnit *TCompilerImpl::loadUnit (const std::string &unitname) {
    TUnitDescription &unitDescription = unitMap [unitname];
    if (!unitDescription.unit) {
        const std::string fn = searchUnitPathes (unitname + ".pas");
        if (!fn.empty ()) {
//        for (const std::string &path: unitSearchPathes) {
//            const std::string fn = path + '/' + unitname + ".pas";
//            if (std::filesystem::exists (fn)) {
                lexerStack.push (&unitDescription.lexer);
                unitDescription.lexer.setFilename (fn);
                unitDescription.unit = memoryPoolFactory.create<TUnit> (*this, predefinedSymbols);
                if (allUnits.empty ())
                    allUnits.push_back (unitDescription.unit);
                else
                    allUnits.insert (allUnits.begin () + 1, unitDescription.unit);
                unitDescription.unit->parseHeader ();
                lexerStack.pop ();
        }
    }
    if (!unitDescription.unit)
        errorMessage (FileNotFound, "Unit '" + unitname + "' not found");
    return unitDescription.unit;
}

TUnit *TCompilerImpl::getSystemUnit () {
    return systemUnit;
}

void TCompilerImpl::parseProgram () {
    TProgram program (*this, predefinedSymbols);
    program.parse ();
    
    bool allUnitsDone;
    do {
        allUnitsDone = true;
        for (std::pair<const std::string, TUnitDescription> &it: unitMap)
            if (!it.second.complete) {
                lexerStack.push (&it.second.lexer);
                if (it.second.unit)
                    it.second.unit->parseImplementation ();
                it.second.complete = true;
                lexerStack.pop ();
                allUnitsDone = false;
            }
    } while (!allUnitsDone);
    
    if (!errorFlag) {
        for (std::vector<TUnit *>::reverse_iterator it = allUnits.rbegin (); it != allUnits.rend (); ++it)
            program.appendUnit (*it);
        program.acceptCodeGenerator (codeGenerator);
    }
}

TUnit *TCompilerImpl::parseUnit () {
    TUnit *unit =  memoryPoolFactory.create<TUnit> (*this, predefinedSymbols);
    unit->parseHeader ();
    unit->parseImplementation ();
    return unit;
}

TMemoryPoolFactory &TCompilerImpl::getMemoryPoolFactory () {
    return memoryPoolFactory;
}

void TCompilerImpl::createPredefinedSymbols () {
    for (TType *t: std::vector<TType *> {&stdType.Boolean, &stdType.Char, &stdType.Uint8, &stdType.Int8, &stdType.Uint16, &stdType.Int16, &stdType.Uint32, &stdType.Int32, &stdType.Int64, &stdType.Real, &stdType.Single, &stdType.String})
        predefinedSymbols->addNamedType (t->getName (), t);
    predefinedSymbols->addNamedType (TConfig::binFileType, memoryPoolFactory.create<TFileType> (nullptr, memoryPoolFactory.create<TSymbolList> (nullptr, memoryPoolFactory)));
    predefinedSymbols->addNamedType ("text", memoryPoolFactory.create<TFileType> (nullptr, memoryPoolFactory.create<TSymbolList> (nullptr, memoryPoolFactory)));
    predefinedSymbols->addNamedType ("pointer", &stdType.GenericPointer);
    predefinedSymbols->addNamedType ("__generic_vector", &stdType.GenericVector);
    
    predefinedSymbols->addConstant ("false", createMemoryPoolObject<TSimpleConstant> (static_cast<std::int64_t> (0), &stdType.Boolean));
    predefinedSymbols->addConstant ("true", createMemoryPoolObject<TSimpleConstant> (static_cast<std::int64_t> (1), &stdType.Boolean));
    predefinedSymbols->addConstant ("nil", createMemoryPoolObject<TSimpleConstant> (static_cast<std::int64_t> (0), &stdType.GenericPointer));
}

// ---

void TCompilerImplDeleter::operator () (TCompilerImpl *impl) {
    delete impl;
}

TCompiler::TCompiler (TCodeGenerator &codeGenerator):
  pImpl (new TCompilerImpl (codeGenerator)) {
}

TCompiler::~TCompiler () {
}

void TCompiler::setSource (const std::string &s) {
    impl ()->setSource (s);
}

void TCompiler::setSource (std::string &&s) {
    impl ()->setSource (std::move (s));
}

void TCompiler::setFilename (const std::string &fn) {
    impl ()->setFilename (fn);
}

void TCompiler::setUnitSearchPathes (const std::vector<std::string> &pathes) {
    impl ()->setUnitSearchPathes (pathes);
}

TCompiler::TCompileResult TCompiler::compile () {
    return impl ()->compile ();
}

const TCompilerImpl *TCompiler::impl () const {
    return pImpl.get ();
}

TCompilerImpl *TCompiler::impl () {
    return pImpl.get ();
}

}
