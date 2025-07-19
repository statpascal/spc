/** \file a64gen.hpp
*/

#pragma once

#include <set>

#include "codegenerator.hpp"
#include "a64asm.hpp"

namespace statpascal {

class TA64Generator: public TBaseGenerator {
using inherited = TBaseGenerator;
public:
    TA64Generator (TRuntimeData &, bool codeRangeCheck = true, bool createCompilerListing = false);
    void getAssemblerCode (std::vector<std::uint8_t> &, bool generateListing, std::vector<std::string> &);

    virtual void generateCode (TTypeCast &) override;
    virtual void generateCode (TExpression &) override;
    virtual void generateCode (TPrefixedExpression &) override;
    virtual void generateCode (TSimpleExpression &) override;
    virtual void generateCode (TTerm &) override;
    virtual void generateCode (TVectorIndex &) override;
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
    struct TLabelDefinition {
        std::string label;
        std::size_t value;
    };
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
    struct TImmLogic {
        std::uint8_t size, length, rotation;
    };
    
    void assignParameterOffsets (ssize_t &pos, TBlock &, std::vector<TSymbol *> &registerParameters);
    void assignStackOffsets (TBlock &);    
    void assignRegisters (TSymbolList &);
    void codeBlock (TBlock &block, std::vector<TUnit *> units, bool hasStackFrame, std::vector<TA64Operation> &blockStatements);
    void generateBlock (TBlock &, std::vector<TUnit *> units = {});
    void externalRoutine (TSymbol &);
    void beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &, const std::vector<TA64Reg> &saveRegs, bool hasStackFrame);
    void endRoutineBody (std::size_t level, TSymbolList &, const std::vector<TA64Reg> &saveRegs, bool hasStackFrame);

    void outputBooleanCheck (TExpressionBase *, const std::string &label, bool branchOnFalse = true);
    void outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputPointerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputFloatOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputIntegerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputIntegerCmpOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputBinaryOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputCompare (const TA64Operand &, const std::int64_t);
    
    TA64Operand makeOperand (const TSymbol *, TA64OpSize = TA64OpSize::bit_default);	// only for globals and current level
    void codeSymbol (const TSymbol *, TA64Reg reg);	// address -> reg
    void loadLabelAddress (const std::string &label, TA64Reg reg);	// address of label (PIC) -> reg
    void loadImmediate (TA64Reg reg, ssize_t n);
    
    void loadImmediate (ssize_t);	// ->calculator stack
    void loadVariable (const TSymbol *, bool convertSingle = true);	// -> calculator stack
    void storeVariable (const TSymbol *);    // from calculator stack
    
    void codeLoadMemory (TType *, TA64Reg destReg, TA64Operand srcMem, bool convertSingle = true);
    void codeStoreMemory (TType *, TA64Reg srcReg, TA64Operand destMem);
    void codeMultiplyConst (TA64Reg, ssize_t, TA64Reg scratchReg);	// scratchReg used for non-shift
    void codeRuntimeCall (const std::string &fn, TA64Reg globalDataReg, const std::vector<std::pair<TA64Reg, ssize_t>> &additionalArgs);
    
    void codeInlinedFunction (TFunctionCall &);
    void codeIncDec (TPredefinedRoutine &);
    
    void codePush (TA64Reg, TA64Reg = TA64Reg::xzr);
    void codePop (TA64Reg, TA64Reg = TA64Reg::xzr);
    void codeModifySP (ssize_t);
    
    void saveReg (TA64Reg);
    void loadReg (TA64Reg);
    TA64Reg intStackTop ();
    TA64Reg fetchReg (TA64Reg);
    TA64Reg getSaveReg (TA64Reg);
    
    void saveDblReg (TA64Reg);
    void loadDblReg (TA64Reg);
    TA64Reg fetchDblReg (TA64Reg);
    TA64Reg getSaveDblReg (TA64Reg);
    
    void clearRegsUsed ();
    void setRegUsed (TA64Reg);
    bool isRegUsed (TA64Reg) const;

    std::string getUniqueLabelName (const std::string &);    
    
    bool is32BitLimit (std::int64_t);

    bool codeRangeCheck, createCompilerListing;
    TRuntimeData &runtimeData;
    TSymbol *globalRuntimeDataSymbol;
    std::size_t currentLevel;
    
    std::size_t intStackCount, dblStackCount;
    std::array<bool, static_cast<std::size_t> (TA64Reg::nrRegs)> regsUsed;
    
    std::map<std::uint64_t, TImmLogic> immLogicTable;
    
    std::size_t dblConstCount;
    std::vector<TLabelDefinition> labelDefinitions;
    std::vector<TGlobalDefinition> globalDefinitions;
    std::vector<TConstantDefinition> constantDefinitions;
    std::vector<TSetDefinition> setDefinitions;
    std::string endOfRoutineLabel;
    
    struct TJumpTable {
        std::string tableLabel, defaultLabel;
        std::vector<std::string> jumpLabels;
    };
    std::vector<TJumpTable> jumpTableDefinitions;
    
    std::vector<TA64Operation> program, *currentOutput;

    void setOutput (std::vector<TA64Operation> *);
    void outputCode (TA64Operation &&);
    void outputCode (TA64Op, std::vector<TA64Operand> && = {}, std::string comment = std::string ());
    void outputLabel (const std::string &label);
    void outputGlobal (const std::string &name, std::size_t size);
    void outputComment (const std::string &);
    
    void outputLocalJumpTables ();
    
    std::string registerConstant (double);
    std::string registerConstant (const std::array<std::uint64_t, TConfig::setwords> &);
    void registerLocalJumpTable (const std::string &tableLabel, const std::string &defaultLabel, std::vector<std::string> &&jumpLabels);
    
    
    void outputLabelDefinition (const std::string &label, std::size_t value);
    
    bool isCalleeSavedReg (TA64Reg);
    bool isCalleeSavedReg (const TA64Operand &);
    
    virtual void initStaticRoutinePtr (std::size_t addr, const TRoutineValue *) override;
    
    // peep hole optimizer
    
    void buildImmLogicTable ();

    bool isImmLogical (TA64Operand operand);
    
    bool isCalcStackReg (TA64Reg);
    bool isDblStackReg (TA64Reg);
    bool isDblStackReg (const TA64Operand &);
    bool isDblReg (const TA64Operand &);
    bool isCalcStackReg (const TA64Operand &);
    bool isSameReg (const TA64Operand &op1, const TA64Operand &op2);
    bool isSameCalcStackReg (const TA64Operand &op1, const TA64Operand &op2);
    bool isSameDblReg (const TA64Operand &op1, const TA64Operand &op2);
    bool isSameDblStackReg (const TA64Operand &op1, const TA64Operand &op2);
    bool isRegisterIndirectAddress (const TA64Operand &);
    bool isRIPRelative (const TA64Operand &);
    bool checkCode (TA64Op op1, TA64Op op2, const std::vector<TA64Op> &first, const std::vector<TA64Op> &second);
    
    void modifyReg (TA64Operand &operand, TA64Reg a, TA64Reg b);
    
    void removeUnusedLocalLabels (std::vector<TA64Operation> &code);
    void tryLeaveFunctionOptimization (std::vector<TA64Operation> &);
    void replacePseudoOpcodes (std::vector<TA64Operation> &);
    
    bool optimizePass1 (TA64Op &op1, TA64Op &op2, TA64Op &op3, std::vector<TA64Operand> &op_1, std::vector<TA64Operand> &op_2, std::vector<TA64Operand> &op_3);
    bool optimizePass2 (TA64Op &op1, TA64Op &op2, TA64Op &op3, std::vector<TA64Operand> &op_1, std::vector<TA64Operand> &op_2, std::vector<TA64Operand> &op_3);
    bool optimizePass3 (TA64Op &op1, TA64Op &op2, TA64Op &op3, std::vector<TA64Operand> &op_1, std::vector<TA64Operand> &op_2, std::vector<TA64Operand> &op_3);
    
    void optimizePeepHole (std::vector<TA64Operation> &);
};

}
