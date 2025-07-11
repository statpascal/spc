/** \file pcodegenerator.hpp
*/

#pragma once

#include "codegenerator.hpp"

namespace statpascal {

class TPCodeGenerator: public TBaseGenerator {
using inherited = TBaseGenerator;
public:
    TPCodeGenerator (TPCodeMemory &, bool codeRangeCheck = true, bool createCompilerListing = false);
    virtual ~TPCodeGenerator () override;
    
    const std::vector<std::string> &getCompilerListing ();

    virtual void generateCode (TTypeCast &) override;
    virtual void generateCode (TExpression &) override;
    virtual void generateCode (TPrefixedExpression &) override;
    virtual void generateCode (TSimpleExpression &) override;
    virtual void generateCode (TTerm &) override;
    virtual void generateCode (TVectorIndex &) override;
    virtual void generateCode (TCombineRoutine &) override;
    virtual void generateCode (TFunctionCall &) override;
    virtual void generateCode (TConstantValue &) override;
    virtual void generateCode (TRoutineValue &) override;
    virtual void generateCode (TVariable &) override;    
    virtual void generateCode (TReferenceVariable &) override;
    virtual void generateCode (TLValueDereference &) override;
    virtual void generateCode (TArrayIndex &) override;
    virtual void generateCode (TRecordComponent &) override;
    virtual void generateCode (TPointerDereference &) override;
    
    virtual void generateCode (TRuntimeRoutine &) override;
    virtual void generateCode (TPredefinedRoutine &) override;
    
    virtual void generateCode (TSimpleStatement &) override;
    virtual void generateCode (TAssignment &) override;
    virtual void generateCode (TRoutineCall &) override;
    virtual void generateCode (TIfStatement &) override;
    virtual void generateCode (TCaseStatement &) override;
    virtual void generateCode (TStatementSequence &) override;
    virtual void generateCode (TLabeledStatement &) override;
    virtual void generateCode (TGotoStatement &) override;
    
    virtual void generateCode (TBlock &) override;
    virtual void generateCode (TUnit &) override;
    virtual void generateCode (TProgram&) override;
    
    virtual TParameterLocation classifyType (const TType *) override;
    virtual TReturnLocation classifyReturnType (const TType *) override;
    
    virtual bool isFunctionCallInlined (TFunctionCall &) override;
    virtual bool isReferenceCallerCopy (const TType *type) override;
    
private:
    const std::size_t stackFrameSize = 3 * sizeof (void *);

    using TOperationMap = std::map<std::pair<TType *, TToken>, TPCode::TOperation>;
    
    struct TMemoryOperation {
        TPCode::TOperation load, store, push;
    };
    using TMemoryOperationMap = std::map<TType *, TMemoryOperation>;
    using TPredefinedFunctionMap = std::map<TPredefinedRoutine::TRoutine, TPCode::TOperation>;
    
    struct TMemoryOperationLookup {
        bool found;
        TMemoryOperation operation;
    };
    
    void outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputBinaryOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    
    TMemoryOperationLookup lookupMemoryOperation (TType *) const;
    
    void buildOperationMap ();

    const ssize_t invalidScalarTypeCode = -256;
    TPCode::TScalarTypeCode getScalarTypeCode (const TType *) const;

    void outputDestructors (const TSymbolList &);
    
    ssize_t getRoutinePosition(TRoutineValue &);
    
    TExternalType getExternalType (const TType *, bool isReturnValue);
    TCallbackDescription getCallbackDescription (const TRoutineType *);
    void externalRoutine (TSymbol &);
    
    void outputSymbolList (const std::string &routineName, std::size_t level, TSymbolList &);
    void createCodeListing ();
    
    void assignStackOffsets (TSymbolList &);    
    
    void generateBlock (TBlock &);

//    void assignValue (TSymbolList &predefinedSymbols, const std::string &s, std::int64_t val);    
    void generateRangeCheck (std::int64_t minval, std::int64_t maxval, bool vectorized = false, TPCode::TScalarTypeCode scrTypeCode = TPCode::TScalarTypeCode::invalid);
    
    void beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &);
    void endRoutineBody (std::size_t level, TSymbolList &);
    
    virtual void initStaticRoutinePtr (std::size_t addr, const TRoutineValue *) override;
    
    TPCodeMemory &pcodeMemory;
    
    TOperationMap operationMap;
    TMemoryOperationMap memoryOperationMap;
    const TPredefinedFunctionMap predefinedFunctionMap, predefinedVectorFunctionMap;
    
    std::size_t currentLevel;
    std::vector<std::size_t> unresolvedExitCalls;

    bool codeRangeCheck, createCompilerListing;
    std::vector<std::string> compilerListing;
    std::vector<std::pair<std::size_t, std::vector<std::string>>> routineHeader;
};



}