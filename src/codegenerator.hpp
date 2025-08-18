/** \file codegenerator.hpp
*/

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "predefined.hpp"
#include "statements.hpp"
#include "anymanager.hpp"
#include "runtime.hpp"

#include <map>

namespace statpascal {

class TCodeGenerator {
public:
    virtual ~TCodeGenerator ();

    virtual void generateCode (TTypeCast &) = 0;
    virtual void generateCode (TExpression &) = 0;
    virtual void generateCode (TPrefixedExpression &) = 0;
    virtual void generateCode (TSimpleExpression &) = 0;
    virtual void generateCode (TTerm &) = 0;
    virtual void generateCode (TFunctionCall &) = 0;
    virtual void generateCode (TConstantValue &) = 0;
    virtual void generateCode (TRoutineValue &) = 0;
    virtual void generateCode (TVariable &) = 0;    
    virtual void generateCode (TReferenceVariable &) = 0;
    virtual void generateCode (TLValueDereference &) = 0;
    virtual void generateCode (TArrayIndex &) = 0;
    virtual void generateCode (TRecordComponent &) = 0;
    virtual void generateCode (TPointerDereference &) = 0;
    
    virtual void generateCode (TPredefinedRoutine &) = 0;
    virtual void generateCode (TAssignment &) = 0;
    virtual void generateCode (TRoutineCall &) = 0;
    virtual void generateCode (TIfStatement &) = 0;
    virtual void generateCode (TCaseStatement &) = 0;
    virtual void generateCode (TStatementSequence &) = 0;
    virtual void generateCode (TLabeledStatement &) = 0;
    virtual void generateCode (TGotoStatement &) = 0;
    
    virtual void generateCode (TBlock &) = 0;
    virtual void generateCode (TUnit &) = 0;
    virtual void generateCode (TProgram &) = 0;

    virtual void alignType (TType *) = 0;

    enum class TParameterLocation {IntReg, FloatReg, Stack, ObjectByValue};
    virtual TParameterLocation classifyType (const TType *) = 0;
    
    enum class TReturnLocation {Register, Reference};
    virtual TReturnLocation classifyReturnType (const TType *) = 0;
    
    virtual bool isFunctionCallInlined (TFunctionCall &) = 0;
    
    /** return true if ABI uses reference to copy made by caller */
    virtual bool isReferenceCallerCopy (const TType *type) = 0;

    void visit (TSyntaxTreeNode *);
    

};

class TBaseGenerator: public TCodeGenerator {
using inherited = TCodeGenerator;
public:
    virtual ~TBaseGenerator ();

    virtual void alignType (TType *) override;
    
    void *getGlobalDataArea () const;
    TRuntimeData *getRuntimeData () const;
    
    struct TTypeAnyManager {
        TAnyManager *anyManager;
        std::size_t runtimeIndex;
    };
    TTypeAnyManager lookupAnyManager (const TType *);
protected:
    explicit TBaseGenerator (TRuntimeData &);

    std::string getNextLocalLabel ();
    std::string getNextCaseLabel ();
    
    void assignAlignedOffset (TSymbol &s, ssize_t &offset, std::size_t align);
    void assignStackOffsets (ssize_t &pos, TSymbolList &symbolList, bool handleParameters);
    
    void alignRecordFields (TRecordType::TRecordFields &, std::size_t &size, std::size_t &alignment);
    void addRecordFieldOffset (TRecordType::TRecordFields &, std::size_t offset);

    using TTypeAnyManagerMap = std::map<const TType *, TTypeAnyManager>;
    
    
    TAnyManager *buildAnyManager (const TSymbolList &symbolList);
    TAnyManager *buildAnyManagerRecord (const TRecordType *recordType);
    TAnyManager *buildAnyManagerArray (const TType *type);
    TAnyManager *buildAnyManager (const TType *type);
    
    std::size_t registerAnyManager (TAnyManager *);
    std::size_t registerStringConstant (const std::string &);
    std::size_t registerData (const void *, std::size_t);
    
    void assignGlobals (TSymbolList &globalSymbols);
    void initStaticGlobals (TSymbolList &globalSymbols);
    void initStaticVariable (char *addr, const TType *t, const TConstant *constant);
    void makeUniqueLabelNames (TSymbolList &);
    
    TType *getMemoryOperationType (TType *type);
    bool getSetTypeLimit (const TExpressionBase *, std::int64_t &minval, std::int64_t &maxval);
    
    std::vector<std::string> createSymbolList (const std::string &routineName, std::size_t level, TSymbolList &, const std::vector<std::string> &regNames, int offset = 0);

    // TODO: private after A64 modificiation    
    void allocateGlobalDataArea (std::size_t n);
    
private:    
    virtual void initStaticRoutinePtr (std::size_t addr, const TRoutineValue *) = 0;
    
    TTypeAnyManagerMap typeAnyManagerMap;
    std::size_t labelCount;
    TRuntimeData &runtimeData;
    void *globalDataArea;
};

}
