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
    
    void parseAssemblerBlock (TSymbol *, TBlock &block);

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
    TCodeSequence *currentOutput;
    
    struct TCodeBlock {
        TCodeBlock (TSymbol *s = nullptr): symbol (s) {}
        TSymbol *symbol;
        TCodeSequence codeSequence;
        std::size_t size, address, bank;
    };
    std::map<std::string, std::uint16_t> bankMapping;

    TCodeBlock sharedCode, mainProgram;
    std::vector<TCodeBlock> subPrograms;	// top level subprograms


    void assignParameterOffsets (TBlock &);
    void assignStackOffsets (TBlock &);    
    void assignRegisters (TSymbolList &);
    void assignGlobalVariables (TSymbolList &);
    void codeBlock (TBlock &block, bool hasStackFrame, bool isFar, TCodeSequence &blockStatements);
    void generateBlock (TBlock &);
    void resolvePascalSymbol (T9900Operand &operand, TSymbolList &symbols);
    void externalRoutine (TSymbol *);
    void beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &, const std::set<T9900Reg> &saveRegs, bool hasStackFrame);
    void endRoutineBody (std::size_t level, TSymbolList &, const std::set<T9900Reg> &saveRegs, bool hasStackFrame, bool isFar);

    void outputBooleanCheck (TExpressionBase *, std::string labelTrue, std::string labelFalse);
    void outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputPointerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputFloatOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputIntegerDivision (TToken operation, bool secondFirst, bool rightIsConstant, ssize_t n);
    void outputIntegerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputIntegerCmpOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    void outputBinaryOperation (TToken operation, TExpressionBase *left, TExpressionBase *right);
    
    void codeSymbol (const TSymbol *, T9900Reg reg);	// address -> regName
    void codeLoadMemory (TType *, T9900Reg destReg, T9900Operand srcMem);
    void codeStoreMemory (TType *, T9900Operand destMem, T9900Reg srcReg);
    void codeMultiplyConst (T9900Reg, std::size_t);
    
    void inlineMove (T9900Reg src, T9900Reg dst, T9900Reg count);
    void codeMove (const TType *);
    
//    void initStaticVariable (char *addr, const TType *t, const TConstant *constant);
    
    // TODO -> make signed args
    void codeSignExtension (TType *, T9900Reg destReg, T9900Operand srcOperand);
    
    void codeInlinedFunction (TFunctionCall &);
    void codeIncDec (TPredefinedRoutine &);
    
    // keep track of SP for 16 byte stack alignment 
    void codePush (T9900Operand);
    void codePop (T9900Operand);
    
    void saveReg (T9900Reg);
    void loadReg (T9900Reg);
    T9900Reg fetchReg (T9900Reg);
    T9900Reg getSaveReg (T9900Reg);
    
    void clearRegsUsed ();
    void setRegUsed (T9900Reg);
    bool isRegUsed (T9900Reg) const;
    
    bool codeRangeCheck, createCompilerListing;
    std::size_t currentLevel;
    
    std::size_t intStackCount;
    std::array<bool, static_cast<std::size_t> (T9900Reg::nrRegs)> regsUsed;
    
    std::size_t dblConstCount;
    std::vector<TConstantDefinition> constantDefinitions;
    std::vector<TSetDefinition> setDefinitions;
    std::vector<TStringDefinition> stringDefinitions;
    
    
    std::string endOfRoutineLabel;
    
    struct TJumpTable {
        std::string tableLabel, defaultLabel;
        std::vector<std::string> jumpLabels;
    };
    std::vector<TJumpTable> jumpTableDefinitions;
    
    struct TFarCallVec {
        std::string label, bank, proc;
    };
    std::vector<TFarCallVec> farCallVectors;
    
    std::map<TSymbol *, TCodeSequence> assemblerBlocks;

    void setOutput (TCodeSequence *);
    void outputCode (const T9900Operation &);
    void outputCode (T9900Op, T9900Operand = T9900Operand (), T9900Operand = T9900Operand (), const std::string &comment = std::string ());
    void outputLabel (const std::string &label);
    void outputComment (const std::string &);
    
    void outputLocalDefinitions ();
    void outputStaticConstants ();
    
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
    
    struct TStaticDataDefinition {
        std::string label;
        std::vector<char> values;
        std::vector<std::pair<std::uint16_t, std::string>> staticRoutinePtrs;
    };
    TStaticDataDefinition staticDataDefinition;
    std::vector<char> staticData;
    
    virtual void initStaticRoutinePtr (std::size_t addr, const TRoutineValue *) override;
    
    T9900Operand makeLabelMemory (const std::string label, T9900Reg index = T9900Reg::r0);
    
    // assembler

    T9900Reg parseRegister (TCompilerImpl &compiler, TLexer &lexer);    
    T9900Operand parseInteger (TCompilerImpl &compiler, TLexer &lexer, std::int64_t minval, std::int64_t maxvval);
    T9900Operand parseGeneralAddress (TCompilerImpl &compiler, TLexer &lexer);
    
    static const unsigned totalBanks = 128;
    std::array<std::uint16_t, totalBanks> bankOffset;
    unsigned maxBank;
    
    void calcLength (TCodeBlock &);
    void assignBank (TCodeBlock &);
    void resolveBankLabels (TCodeBlock &proc);
    std::string getBankName (const TSymbol *);
    std::string getBankName (const std::string &);
    
    // peep hole optimizer
    
    void optimizePeepHole (TCodeSequence &);
    void optimizeSingleLine (TCodeSequence &);
    
    std::map<std::string, std::int64_t> jmpLabels;
    void optimizeJumps (TCodeSequence &);
    
    void removeJmpLines (TCodeSequence &, TCodeSequence::iterator it, std::size_t count);
    void mergeLabel (TCodeSequence &code, TCodeSequence::iterator line);
    void mergeMultipleLabels (TCodeSequence &code);

    void removeUnusedLocalLabels (TCodeSequence &code);
    void removeLines (TCodeSequence &code, TCodeSequence::iterator &line, std::size_t count);    
    void replaceLabel (T9900Operation &, T9900Operand &, std::size_t offset);
    void assemblePass (int pass, std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &listing);
};

}
