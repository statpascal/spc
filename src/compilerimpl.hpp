/** \file compilerimpl.hpp
*/

#pragma once

#include <vector>
#include <stack>
#include <unordered_map>

#include "compiler.hpp"
#include "lexer.hpp"
#include "datatypes.hpp"
#include "syntaxtreenode.hpp"
#include "statements.hpp"

namespace statpascal {

class TBlock: public TSyntaxTreeNode {
public:
    TBlock (TCompilerImpl &compiler, TSymbolList *blockSymbols, TSymbol *ownSymbol, TBlock *parent);
    
    void parse ();
    void parseDeclarations (bool isUnitInterface = false);
    
    TSymbol *getSymbol () const;
    TStatement *getStatements () const;
    TBlock *getParentBlock () const;
    
    const TSimpleConstant *parseConstantLiteral ();
    const TSimpleConstant *parseConstExpression ();
    TSymbol *checkVariable ();
    
    /** search for missing label and forward defintions */
    void checkDefinitions ();
    
    /** flag for function call and access of local variable in surrounding block */
    void setDisplayNeeded ();
    bool isDisplayNeeded () const;
    
    TSymbolList &getSymbols () const;
    
    /** search for field in active with statements; returns nullptr if not found */
    TExpressionBase *searchActiveRecords (const std::string &identifier);
    void pushActiveRecord (TExpressionBase *);
    void popActiveRecord ();
    
    // TODO: class TCompilerEnvironment mit compiler/lexer/...     
    TCompilerImpl &getCompiler () const;
    TLexer &getLexer () const;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *returnLValueDeref;
private:
    const TSimpleConstant *parseSetConstant ();
    const TSimpleConstant *parseConstantSizeof ();
    const TSimpleConstant *parseConstFactor ();
    const TSimpleConstant *parseConstTerm ();
    
    void parseDeclarationList (std::vector<std::string> &identifiers, TType *&type, bool allowGenericVar);
    void parseExternalDeclaration (std::string &libName, std::string &symbolName);
    bool checkSymbolName (std::string &symbolName);

    TType *parseEnumerationType ();
    TType *parseArrayType ();
    TType *parseShortString ();
    TType *parseRecordType ();
    TType *parseSetType ();
    TType *parseFileType ();
    TType *parseRoutineType (bool isFunction, bool farCall = false);
    TType *parseFunctionType ();
    TType *parseProcedureType ();
    TType *parsePointerType ();
    TType *parseSubrangeType ();
    TType *parseVectorType ();
    TType *parseTypeIdentifier ();	// can be defined type or subrange beginning with const
    TType *parseType ();
    
    void parseVarParameterDeclaration (TSymbolList &symbolList, bool createReference, bool isParameter);
    void parseParameterDeclaration (TSymbolList &symbolList);
    
    void parseRecordFieldList (TRecordType *recordType);
    void parseRecordVariantPart (TRecordType *recordType);
    
    void parseConst (const std::string &identifier);
    
    void parseTypedConst (const std::string &identifier);
    const TConstant *parseTypedConstant (TType *type);
    const TSimpleConstant *parseSimpleConstant (TType *type);	// set or scalar/real/string
    const TArrayConstant *parseArrayConstant (TArrayType *type);
    const TRecordConstant *parseRecordConstant (TRecordType *type);
    
    void parseLabelDeclaration ();
    void parseConstDeclaration ();
    void parseTypeDeclaration ();
    void parseVarDeclaration ();
    void parseSubroutine (bool isFunction);
    void parseProcedure ();
    void parseFunction ();
    void parseDeclarationPart ();
    
    TCompilerImpl &compiler;
    TLexer &lexer;
    TSymbolList *symbols;
    TSymbol *ownSymbol;
    TBlock *parentBlock;
    
    TStatement *statements;
    bool isUnitInterface, parsingTypeDeclaration, displayNeeded;
    std::vector<TPointerType *> unresolvedPointerTypes;
    std::vector<TExpressionBase *> activeRecords;
};

class TUnit;

class TUsesDeclaration: public TMemoryPoolObject {
public:
    explicit TUsesDeclaration (TCompilerImpl &);
    
    TSymbolList *getPublicSymbols (TSymbolList *previousSymbols) const;
    TSymbolList *getPublicSymbols (TSymbolList *previousSymbols, std::size_t level) const;
    
private:
    void parse ();
    
    std::vector<TUnit *> getUnitsTransitive () const;
    void appendUsedUnits (std::vector<TUnit *> &result) const;
    
    TCompilerImpl &compiler;
    std::vector<std::string> unitnames;
};


class TUnit: public TSyntaxTreeNode {
public:
    TUnit (TCompilerImpl &compiler, TSymbolList *predefinedSymbols);
    
    void parseHeader ();
    void parseImplementation ();
    
    TStatement *getInitializationStatement () const;
    TStatement *getFinalizationStatement () const;
    
    TSymbolList *getPublicSymbols () const;
    TSymbolList *getAllSymbols () const;
    
    TUsesDeclaration *getPublicUnits () const;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
private:
    TStatement *parseInitFinal (const std::string funcName, TBlock &declarations);

    TCompilerImpl &compiler;
    TSymbolList *predefinedSymbols, *importedSymbols, *publicSymbols, *allSymbols;
    TUsesDeclaration *publicUnits;
    TStatement *initializationStatement, *finalizationStatement;
    std::string unitname;
};


class TProgram: public TSyntaxTreeNode {
public:
    TProgram (TCompilerImpl &compiler, TSymbolList *predefinedSymbols);
    
    void parse ();
    TBlock *getBlock () const;
    
    TSymbolList *getPredefinedSymbols () const;
    
    std::vector<TUnit *> getUnits () const;
    void appendUnit (TUnit *);
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
private:
    TCompilerImpl &compiler;
    TSymbolList *predefinedSymbols;
    TBlock *block;
    TUsesDeclaration *usedUnits;
    std::vector<TUnit *> allUnits;
};


class TCompilerImpl {
public:
    explicit TCompilerImpl (TCodeGenerator &);
    ~TCompilerImpl ();

    void setSource (const std::string &);
    void setSource (std::string &&);
    void setFilename (const std::string &);
    
    TCompiler::TCompileResult compile ();
    
    TLexer &getLexer ();
    TCodeGenerator &getCodeGenerator ();
    bool getErrorFlag () const;
    
    template<typename T, typename ...Args> T *createMemoryPoolObject (Args&&... args) {
        return memoryPoolFactory.create<T> (std::forward<Args> (args)...);
    }
    
    enum TErrorType {
        SyntaxError, IdentifierNotFound, StatementExpected, IdentifierExpected, ConstExpected, InvalidType, DuplicateCase,
        StringVarExpected, NumericVarExpected, ParameterTypeNotAllowed, ReturnOnlyInFunction, ArrayExpected,
        IncompatibleTypes, FunctionCallExpected, ProcedureCallExpected, IntegerConstantsExpected, InvalidNumberArguments,
        InvalidNumberInitializers, ComponentNotFound, IdentifierAlreadyDeclared, VariableExpected, IntegerVarExpected, NodeRequired,
        DivisionByZero, InvalidUseOfSymbol, PointerRequired, FileNotFound, MemoryFull, InternalError, AssemblerError,
        PositionFound
    };
    void errorMessage (TErrorType, const std::string &description = std::string ());
    void recoverPanicMode (const std::vector<TToken> &syncTokens);
    void checkToken (TToken t, const std::string &errorMessage);
    void checkAndSynchronize (TToken t, const std::string &errorMessage);

    void setUnitSearchPathes (const std::vector<std::string> &);    
    std::string searchUnitPathes (const std::string &);
    TUnit *loadUnit (const std::string &unitname);
    TUnit *getSystemUnit ();
    
    TMemoryPoolFactory &getMemoryPoolFactory ();
    
private:    
    void createPredefinedSymbols ();
    
    void parseProgram ();
    TUnit *parseUnit ();

    TMemoryPoolFactory memoryPoolFactory;
    TLexer programLexer;
    
    TCodeGenerator &codeGenerator;
    TSymbolList *predefinedSymbols;

    struct TUnitDescription {
        TUnitDescription (): unit (nullptr), complete (false) {}
        TLexer lexer;
        TUnit *unit;
        bool complete;
    };
    using TUnitMap = std::unordered_map<std::string, TUnitDescription>;
    TUnitMap unitMap;
    std::vector<TUnit *> allUnits;
    TUnit *systemUnit;
    std::vector<std::string> unitSearchPathes;
    std::stack<TLexer *> lexerStack;
    
    bool errorFlag;
    
};

// inlines

inline TSymbolList &TBlock::getSymbols () const {
    return *symbols;
}


inline TSymbol *TBlock::getSymbol () const {
    return ownSymbol;
}

inline TCompilerImpl &TBlock::getCompiler () const {
    return compiler;
}

inline TLexer &TBlock::getLexer () const {
    return lexer;
}

inline TStatement *TBlock::getStatements () const {
    return statements;
}

inline TBlock *TBlock::getParentBlock () const {
    return parentBlock;
}


inline TSymbolList *TProgram::getPredefinedSymbols () const {
    return predefinedSymbols;
}

inline TLexer &TCompilerImpl::getLexer () {
    return *lexerStack.top ();
}

inline TCodeGenerator &TCompilerImpl::getCodeGenerator () {
    return codeGenerator;
}

inline bool TCompilerImpl::getErrorFlag () const {
    return errorFlag;
}

}
