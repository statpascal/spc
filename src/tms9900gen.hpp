/** \file tms9900gen.hpp
*/

#pragma once

#include <list>
#include <set>

#include "codegenerator.hpp"
#include "tms9900asm.hpp"

namespace statpascal {


class T9900Generator: public TBaseGenerator {
using inherited = TBaseGenerator;
public:
    T9900Generator (TRuntimeData &, bool codeRangeCheck = true, bool createCompilerListing = false);
    
    void getAssemblerCode (std::vector<std::uint8_t> &, bool generateListing, std::vector<std::string> &);

    virtual void generateCode (TTypeCast &) override;
    virtual void generateCode (TExpression &) override;
    virtual void generateCode (TPrefixedExpression &) override;
    virtual void generateCode (TSimpleExpression &) override;
    virtual void generateCode (TTerm &) override;
    virtual void generateCode (TFunctionCall &) override;
    virtual void generateCode (TConstantValue &) override;
    virtual void generateCode (TRoutineValue &) override;
    virtual void generateCode (TVariable &) override;    
    virtual void generateCode (TReferenceVariable &) override;
    virtual void generateCode (TLValueDereference &) override;
    virtual void generateCode (TArrayIndex &) override;
    virtual void generateCode (TRecordComponent &) override;
    virtual void generateCode (TPointerDereference &) override;
    
    virtual void generateCode (TPredefinedRoutine &) override;
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
/*    struct TLabelDefinition {
        std::string label;
        std::size_t value;
    };
*/    
    struct TConstantDefinition {
        std::string label;
        double val;
    };
    struct TSetDefinition {
        std::string label;
        std::array<std::uint64_t, TConfig::setwords> val;
    };
    struct TGlobalDefinition {
        std::string name;
        std::size_t size;
    };
    struct TStringDefinition {
        std::string label, val;
    };
    
    using TCodeSequence = std::list<T9900Operation>;
    TCodeSequence program, *currentOutput;


    void assignParameterOffsets (TBlock &);
    void assignStackOffsets (TBlock &);    
    void assignRegisters (TSymbolList &);
    void assignGlobalVariables (TSymbolList &);
    void codeBlock (TBlock &block, bool hasStackFrame, TCodeSequence &blockStatements);
    void generateBlock (TBlock &);
    void externalRoutine (TSymbol &);
    void beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &, const std::set<T9900Reg> &saveRegs, bool hasStackFrame);
    void endRoutineBody (std::size_t level, TSymbolList &, const std::set<T9900Reg> &saveRegs, bool hasStackFrame);

    void outputBooleanCheck (TExpressionBase *, const std::string &label, bool branchOnFalse = true);
    void outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputPointerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputFloatOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputIntegerDivision (TToken operation, bool secondFirst, bool rightIsConstant, ssize_t n);
    void outputIntegerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputIntegerCmpOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputBinaryOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputCompare (const T9900Operand &, const std::int64_t);
    
    void codeSymbol (const TSymbol *, T9900Reg reg);	// address -> regName
    void codeLoadMemory (TType *, T9900Reg destReg, T9900Operand srcMem);
    void codeStoreMemory (TType *, T9900Operand destMem, T9900Reg srcReg);
    void codeMultiplyConst (T9900Reg, std::size_t);
    void codeMove (const TType *);
    
    void initStaticVariable (char *addr, const TType *t, const TConstant *constant);
    
    // TODO -> make signed args
    void codeRuntimeCall (const std::string &fn, T9900Reg globalDataReg, const std::vector<std::pair<T9900Reg, std::size_t>> &additionalArgs);
    void codeSignExtension (TType *, T9900Reg destReg, T9900Operand srcOperand);
    
    void codeInlinedFunction (TFunctionCall &);
    void codeIncDec (TPredefinedRoutine &);
    
    // keep track of SP for 16 byte stack alignment 
    void codePush (T9900Operand);
    void codePop (T9900Operand);
    void codeModifySP (ssize_t);
    
    void saveReg (T9900Reg);
    void loadReg (T9900Reg);
    T9900Reg fetchReg (T9900Reg);
    T9900Reg getSaveReg (T9900Reg);
    
    void clearRegsUsed ();
    void setRegUsed (T9900Reg);
    bool isRegUsed (T9900Reg) const;
    
    bool is32BitLimit (std::int64_t);

    bool codeRangeCheck, createCompilerListing;
    TRuntimeData &runtimeData;
    TSymbol *globalRuntimeDataSymbol;
    std::size_t currentLevel;
    
    std::size_t intStackCount, xmmStackCount;
    std::size_t stackPositions;		// align RSP on funciton call
    std::array<bool, static_cast<std::size_t> (T9900Reg::nrRegs)> regsUsed;
    
    std::size_t dblConstCount;
    std::unordered_map<std::string, std::size_t> labelDefinitions, relLabelDefinitions;
    std::vector<TGlobalDefinition> globalDefinitions;
    std::vector<TConstantDefinition> constantDefinitions;
    std::vector<TSetDefinition> setDefinitions;
    std::vector<TStringDefinition> stringDefinitions;
    std::string endOfRoutineLabel;
    
    struct TJumpTable {
        std::string tableLabel, defaultLabel;
        std::vector<std::string> jumpLabels;
    };
    std::vector<TJumpTable> jumpTableDefinitions;

    void setOutput (TCodeSequence *);
    void outputCode (const T9900Operation &);
    void outputCode (T9900Op, T9900Operand = T9900Operand (), T9900Operand = T9900Operand (), const std::string &comment = std::string ());
    void outputLabel (const std::string &label);
    void outputGlobal (const std::string &name, std::size_t size);
    void outputComment (const std::string &);
    
    void outputLocalJumpTables ();
    void outputGlobalConstants ();
    
    std::string registerConstant (double);
    std::string registerConstant (const std::array<std::uint64_t, TConfig::setwords> &);
    std::string registerConstant (const std::string &);
    void registerLocalJumpTable (const std::string &tableLabel, const std::string &defaultLabel, std::vector<std::string> &&jumpLabels);
    
    void outputLabelDefinition (const std::string &label, std::size_t value);
    
    bool isCalleeSavedReg (T9900Reg);
    bool isCalleeSavedReg (const T9900Operand &);
    bool isCalcStackReg (T9900Reg);
    bool isCalcStackReg (const T9900Operand &);
    bool isSameReg (const T9900Operand &op1, const T9900Operand &op2);
    bool isSameCalcStackReg (const T9900Operand &op1, const T9900Operand &op2);
    
    virtual void initStaticRoutinePtr (std::size_t addr, const TRoutineValue *) override;
    
    // peep hole optimizer
    
    void optimizePeepHole (TCodeSequence &);

    void removeUnusedLocalLabels (TCodeSequence &code);
    void removeLines (TCodeSequence &code, TCodeSequence::iterator &line, std::size_t count);    
    void replaceLabel (T9900Operation &, T9900Operand &, std::size_t offset);
    void assemblePass (int pass, std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &listing);
};

}
