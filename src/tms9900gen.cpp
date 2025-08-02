/** \file tms9900gen.hpp
*/

#include <dlfcn.h>
#include <unistd.h>
#include <cassert>
#include <iterator>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <iostream>

#include "tms9900gen.hpp"

/*

9900 calling conventions 

r0:	   scratch reg; used for shift/rotate count
r1 - r8:   used as calculator stack; saved by callee
r9:	   used as frame pointer; saved by callee
r10:       stack pointer (decrementing, points to last value in stack)
r11:	   link register
r12 - r15: saved by caller

Stack layout of activation frame:

+---------------------------------+
|				  |
|  parameters			  |
|				  |
|---------------------------------|
|  address of return value 	  |
|---------------------------------|
|  return address	          |
|---------------------------------|
|  saved frame pointer of caller  |	<- R9
|---------------------------------|
|				  |
|  frame pointers of higher 	  |
|  nesting levels (if existing)   |
|  in descending order		  |
|---------------------------------|
|                                 |
|  local variables                |
|                                 |   
|---------------------------------|
|                                 |
|  saved regs of caller           |
|                                 |  	<- R10
|---------------------------------|
|  free memory                    |

*/

namespace statpascal {

namespace {

std::string toHexString (std::uint64_t n) {
    std::stringstream ss;
    ss << std::hex << n;
    return ss.str ();
}

const std::vector<T9900Reg>
    intStackReg = {T9900Reg::r1, T9900Reg::r2, T9900Reg::r3, T9900Reg::r4, T9900Reg::r5, T9900Reg::r6, T9900Reg::r7}; 
    
const T9900Reg 
    intScratchReg1 = T9900Reg::r0,
    intScratchReg2 = T9900Reg::r12,
    intScratchReg3 = T9900Reg::r13,
    intScratchReg4 = T9900Reg::r14;
    

const std::size_t
    intStackRegs = intStackReg.size ();
    
const std::string 
    globalRuntimeDataName = "__globalruntimedata",
    dblAbsMask = "__dbl_abs_mask";

}  // namespace

        
static std::array<TToken, 6> relOps = {TToken::Equal, TToken::GreaterThan, TToken::LessThan, TToken::GreaterEqual, TToken::LessEqual, TToken::NotEqual};

T9900Generator::T9900Generator (TRuntimeData &runtimeData, bool codeRangeCheck, bool createCompilerListing):
  inherited (runtimeData),
  runtimeData (runtimeData),
  currentLevel (0),
  intStackCount (0),
  xmmStackCount (0),
  dblConstCount (0) {
}

bool T9900Generator::isCalleeSavedReg (const T9900Reg reg) {
    return std::find (intStackReg.begin (), intStackReg.end (), reg) != intStackReg.end ();
}

bool T9900Generator::isCalleeSavedReg (const T9900Operand &op) {
    return op.isReg () && isCalleeSavedReg (op.reg);
}

bool T9900Generator::isCalcStackReg (const T9900Reg reg) {
    return isCalleeSavedReg (reg);
}

bool T9900Generator::isCalcStackReg (const T9900Operand &op) {
    return op.isReg () && isCalcStackReg (op.reg);
}

bool T9900Generator::isSameReg (const T9900Operand &op1, const T9900Operand &op2) {
    return op1.isReg () && op2.isReg () && op1.reg == op2.reg;
}

bool T9900Generator::isSameCalcStackReg (const T9900Operand &op1, const T9900Operand &op2) {
    return isCalcStackReg (op1) && isCalcStackReg (op2) && op1.reg == op2.reg;
}

//bool T9900Generator::isRegisterIndirectAddress (const T9900Operand &op) {
//    return op.isPtr && op.index == TX64Reg::none && op.offset == 0;
//}

void T9900Generator::removeLines (TCodeSequence &code, TCodeSequence::iterator &line, std::size_t count) {
    for (std::size_t i = 0; i < count && line != code.end (); ++i)
        line = code.erase (line);
    for (std::size_t i = 0; i < count + 2 && line != code.begin (); ++i)
        line--;
}


void T9900Generator::removeUnusedLocalLabels (TCodeSequence &code) {
    std::set<std::string> usedLabels;
    for (T9900Operation &op: code) 
        if (op.operation != T9900Op::def_label) {
            if (op.operand1.isLabel ())
                usedLabels.insert (op.operand1.label);
            if (op.operand2.isLabel ())
                usedLabels.insert (op.operand2.label);
        }
    TCodeSequence::iterator line = code.begin ();
    while (line != code.end ())
        if (line->operation == T9900Op::def_label && line->operand1.label.substr (0, 2) == ".l" && usedLabels.find (line->operand1.label) == usedLabels.end ())
            line = code.erase (line);
        else
            ++line;
}

void T9900Generator::optimizePeepHole (TCodeSequence &code) {
    code.push_back (T9900Op::end);
    code.push_back (T9900Op::end);
    code.push_back (T9900Op::end);
    
    TCodeSequence::iterator line = code.begin ();
    while (line->operation != T9900Op::end) {
        
        TCodeSequence::iterator line1 = line;
        ++line1;
        TCodeSequence::iterator line2 = line1;
        ++line2;
        TCodeSequence::iterator line3 = line2;
        ++line3;

/*
        std::cout << line->makeString () << std::endl
                  << line1->makeString () << std::endl
                  << line2->makeString () << std::endl
                  << line3->makeString () << std::endl
                  << "----------------" << std::endl;
*/
        
        T9900Operand &op_1_1 = line->operand1,
                     &op_1_2 = line->operand2;
        T9900Op      &op1 = line->operation;
        std::string &comm_1 = line->comment;
        
        T9900Operand &op_2_1 = line1->operand1,
                     &op_2_2 = line1->operand2;
        T9900Op      &op2 = line1->operation;
        std::string &comm_2 = line1->comment; 
        
        T9900Operand &op_3_1 = line2->operand1,
                     &op_3_2 = line2->operand2;
        T9900Op      &op3 = line2->operation;
        std::string &comm_3 = line2->comment; 
        
        // li reg, imm
        // mov|movb|a|s|c op, *reg
        // ->
        // mov|movb|a|s|c op, @imm
        
        if (op1 == T9900Op::li && (op2 == T9900Op::mov || op2 == T9900Op::movb || op2 == T9900Op::a || op2 == T9900Op::s|| op2 == T9900Op::c) && isSameCalcStackReg (op_1_1, op_2_2) && op_2_2.t == T9900Operand::TAddressingMode::RegInd) {
            op_2_2 = T9900Operand (T9900Reg::r0, op_1_2.val);
            comm_2 = comm_2 + " " + comm_1;
            removeLines (code, line, 1);
        }
        
        // mov|szc op, reg
        // ci reg, 0
        // ->
        // mov op, reg
        else if ((op1 == T9900Op::mov || op1 == T9900Op::szc) && op2 == T9900Op::ci && isSameCalcStackReg (op_1_2, op_2_1) && op_2_2.val == 0) {
            removeLines (code, line1, 1);
            if (line != code.begin ()) --line;
        }
        
        // li reg, imm
        // mov|movb *reg, reg
        // ->
        // mov|movb  @imm, reg
        
        else if (op1 == T9900Op::li && (op2 == T9900Op::mov || op2 == T9900Op::movb) && isSameCalcStackReg (op_1_1, op_2_1) && op_2_1.t == T9900Operand::TAddressingMode::RegInd) {
            op_2_1 = T9900Operand (T9900Reg::r0, op_1_2.val);
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // li reg, label|imm
        // bl *reg
        // ->
        // bl @label|imm
        else if (op1 == T9900Op::li && op2 == T9900Op::bl && isSameCalcStackReg (op_1_1, op_2_1)) {
            op_2_1 = op_1_2;
            removeLines (code, line, 1);
        }
        
        // li reg, label|imm
        // inc/inct/dec/dect *reg
        // ->
        // inc/inct/dec/dect @label|imm
        else if (op1 == T9900Op::li && (op2 == T9900Op::inc || op2 == T9900Op::inct || op2 == T9900Op::dec || op2 == T9900Op::dect) && isSameCalcStackReg (op_1_1, op_2_1)) {
            op_2_1 = T9900Operand (T9900Reg::r0, op_1_2.val);
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // mov op1, reg
        // a|s|c|mov reg, op2
        // ->
        // a|s|c|mov op1, op2
        else if (op1 == T9900Op::mov && (op2 == T9900Op::mov || op2 == T9900Op::a || op2 == T9900Op::s || op2 == T9900Op::c) && isSameCalcStackReg (op_1_2, op_2_1)) {
            op_2_1 = op_1_1;
            comm_2 = comm_1 + " " + comm_2;
            removeLines (code, line, 1);
        }
        
        // mov op1, reg
        // c op2, reg
        // ->
        // a|s|c op2, op1
        else if (op1 == T9900Op::mov && op2 == T9900Op::c && isSameCalcStackReg (op_1_2, op_2_2)) {
            op_2_2 = op_1_1;
            comm_2 = comm_1 + " " + comm_2;
            removeLines (code, line, 1);
        }
        
        // movb op1, reg
        // srl reg, 8
        // movb @>8301 + 2*reg, op2
        // ->
        // movb op1, op2
        else if (op1 == T9900Op::movb && op2 == T9900Op::srl && op3 == T9900Op::movb &&
            isSameCalcStackReg (op_1_2, op_2_1) && op_3_1.isIndexed () && op_3_1.val == 0x8301 + 2 * static_cast<int> (op_1_2.reg)) {
            op_1_2 = op_3_2;
            comm_1.append ( " " + comm_3);
            removeLines (code, line1, 2);
        }
        
        // li reg, imm1
        // mov reg, op
        // li reg, imm1
        // ->
        // remove redundant load
        else if (op1 == T9900Op::li && op2 == T9900Op::mov && op3 == T9900Op::li && op_1_2.val == op_3_2.val &&
                 isSameCalcStackReg (op_1_1, op_2_1) && isSameCalcStackReg (op_2_1, op_3_1)) {
            removeLines (code, line2, 1);
        }
        
        // movb op, reg
        // srl reg, 8
        // ci reg, 0
        // ->
        // movb op, reg
        else if (op1 == T9900Op::movb && op2 == T9900Op::srl && op3 == T9900Op::ci && !op_3_2.val &&
            isSameCalcStackReg (op_1_2, op_2_1) && isSameCalcStackReg (op_2_1, op_3_1))
            removeLines (code, line1, 2);
            
        // li reg, 1..2
        // a|s reg, op
        // ->
        // dect..inct op
        else if (op1 == T9900Op::li && op_1_2.isImm () && (op_1_2.val == 1 || op_1_2.val == 2) && (op2 == T9900Op::a || op2 == T9900Op::s) && isSameCalcStackReg (op_1_1, op_2_1)) {
            op_2_1 = op_2_2;
            op_2_2 = T9900Operand ();
            op2 = op2 == T9900Op::a ? 
                            op_1_2.val == 1 ? T9900Op::inc : T9900Op::inct :
                            op_1_2.val == 1 ? T9900Op::dec : T9900Op::dect;
            removeLines (code, line, 1);
        }
        
        // li reg, imm
        // mov reg, r0
        // sla/sra op, r0
        // ->
        // sla/sra op, imm
        else if (op1 == T9900Op::li && op2 == T9900Op::mov && (op3 == T9900Op::sla || op3 == T9900Op::sra) && op_3_2.isReg ()) {
            op_3_2 = op_1_2;
            removeLines (code, line, 2);
        }
        
        // li reg, 0
        // mov reg, op
        // ->
        // clr @op
        else if (op1 == T9900Op::li && !op_1_2.val && op2 == T9900Op::mov && isSameCalcStackReg (op_1_1, op_2_1)) {
            op2 = T9900Op::clr;
            op_2_1 = op_2_2;
            op_2_2 = T9900Operand ();
            removeLines (code, line, 1);
        }
        
        // li reg, imm
        // a op, reg
        // . x, *reg
        // ->
        // mov op, reg
        // . x, @imm(reg)
        else if (op1 == T9900Op::li && op2 == T9900Op::a && isSameCalcStackReg (op_1_1, op_2_2) &&
            isSameCalcStackReg (op_2_2, op_3_2) && op_3_2.t == T9900Operand::TAddressingMode::RegInd) {
            op2 = T9900Op::mov;
            op_3_2 = T9900Operand (op_1_1.reg, op_1_2.val);
            comm_3 = comm_3 + " -> " + comm_1 + " + " + comm_2;
            removeLines (code, line, 1);
        }
        
        // li reg1, imm
        // inv reg1
        // szc reg1, reg2
        // ->
        // andi reg2, imm
        else if (op1 == T9900Op::li && op2 == T9900Op::inv && op3 == T9900Op::szc && isSameCalcStackReg (op_1_1, op_2_1) &&
                 isSameCalcStackReg (op_2_1, op_3_1) && op_3_2.t == T9900Operand::TAddressingMode::Reg) {
            op3 = T9900Op::andi;
            op_3_1 = op_3_2;
            op_3_2 = op_1_2;
            removeLines (code, line, 2);
        }
        
        // li reg1, imm
        // a reg1, reg2
        // ->
        // ai reg2, imm
        else if (op1 == T9900Op::li && op2 == T9900Op::a && isSameCalcStackReg (op_1_1, op_2_1) && !isSameCalcStackReg (op_2_1, op_2_2)) {
            op1 = T9900Op::ai;
            op_1_1 = op_2_2;
            removeLines (code, line1, 1);
            if (line != code.begin ()) --line;
        }
        
        // inv reg
        // inv reg
        // ->
        // remove instructions
        else if (op1 == T9900Op::inv && op1 == op2 && isSameCalcStackReg (op_1_1, op_2_1)) {
            removeLines (code, line, 2);
        }
        
        // ai reg, imm1
        // ai reg, imm2
        // ->
        // ai reg, imm1 + imm2
        else if (op1 == T9900Op::ai && op2 == T9900Op::ai && isSameCalcStackReg (op_1_1, op_2_1)) {
            op_2_2.val = (op_2_2.val + op_1_2.val) & 0xffff;
            removeLines (code, line, 1);
            comm_2 = comm_1;
        }
        
        // Access via base pointer
        // mov r9, reg
        // ai reg, imm
        // ...
        else if (op1 == T9900Op::mov && op2 == T9900Op::ai && op_1_1.t == T9900Operand::TAddressingMode::Reg && op_1_1.reg == T9900Reg::r9 && isSameCalcStackReg (op_1_2, op_2_1)) {
            // mov|a|s *reg, op | inc/inct/dec/dect *reg
            // ->
            // mov|movb|a|s @imm(r9), op | inc/inct/dec/dect @imm(r9)
            if ((op3 == T9900Op::a || op3 == T9900Op::s || op3 == T9900Op::mov || op3 == T9900Op::movb ||op3 == T9900Op::inc || op3 == T9900Op::inct || op3 == T9900Op::dec || op3 == T9900Op::dect) && 
                isSameCalcStackReg (op_2_1, op_3_1) && op_3_1.t == T9900Operand::TAddressingMode::RegInd) {
                op_3_1 = T9900Operand (T9900Reg::r9, op_2_2.val);
                comm_3 = comm_2;
                removeLines (code, line, 2);
            }
            // mov|movb|a|s op, *reg
            // ->
            // mov|movb|a|s op, @imm(r9)
            else if ((op3 == T9900Op::a || op3 == T9900Op::s || op3 == T9900Op::mov || op3 == T9900Op::movb) && op_1_1.isReg () && isSameCalcStackReg (op_2_1, op_3_2) && op_3_2.t == T9900Operand::TAddressingMode::RegInd) {
                op_3_2 = T9900Operand (T9900Reg::r9, op_2_2.val);
                comm_3 = comm_2;
                removeLines (code, line, 2);
            } else
                ++line;
        } else
            ++line;
    }
    
    while (!code.empty () && code.back ().operation == T9900Op::end)
        code.pop_back ();
}

/*
void T9900Generator::replaceLabel (TX64Operation &op, TX64Operand &operand, std::size_t offset) {
    if (operand.isLabel) {
        std::unordered_map<std::string, std::size_t>::iterator it = labelDefinitions.find (operand.label);
        if (it != labelDefinitions.end ()) {
            op.comment = operand.label;
            operand = TX64Operand (it->second);
        } else {
            it = relLabelDefinitions.find (operand.label);
            if (it != relLabelDefinitions.end ()) {
                op.comment = operand.label;
                operand.imm = it->second - offset;
            }
        }
    }
}
*/

/*
void T9900Generator::assemblePass (int pass, std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &listing) {
    std::size_t offset = 0;
    std::vector<std::uint8_t> opcode;
    
    if (pass == 2 && generateListing) {
        listing.push_back ("bits 64");
        for (const std::pair<const std::string, std::size_t> &v: labelDefinitions)
            listing.push_back (v.first + " equ 0x" + toHexString (v.second));
    }
    
    for (const TX64Operation &op: program) {
        TX64Operation encode = op;
        if (pass == 1 && encode.operation == TX64Op::def_label && encode.operand1.isLabel)
            relLabelDefinitions [encode.operand1.label] = offset;
        if (encode.operation < TX64Op::def_label) {
            replaceLabel (encode, encode.operand1, offset);
            replaceLabel (encode, encode.operand2, offset);
        } 
        opcode.clear ();
        encode.outputCode (opcode, offset);
        
        if (pass == 2) {
            opcodes.insert (opcodes.end (), opcode.begin (), opcode.end ());
            if (generateListing) {
                std::string line;
                if (!opcode.empty ()) {
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << std::setfill ('0') << std::setw (8) << offset << ": ";
                    for (std::uint8_t v: opcode)
                        ss << std::setw (2) << static_cast<int> (v);
                    line = ss.str ();
                }
                line.resize (35, ' ');
//                listing.push_back (line + op.makeString ());
                listing.push_back (op.makeString ());
            }
        }
        
        offset += opcode.size ();
    }
}    
*/

void T9900Generator::getAssemblerCode (std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &listing) {
    opcodes.clear ();
    listing.clear ();
    
    for (T9900Operation &op: program)
        listing.push_back (op.makeString ());
}

void T9900Generator::setOutput (TCodeSequence *output) {
    currentOutput = output;
}

void T9900Generator::outputCode (const T9900Operation &l) {
    currentOutput->push_back (l);
}

void T9900Generator::outputCode (const T9900Op op, const T9900Operand op1, const T9900Operand op2, const std::string &comment) {
    outputCode (T9900Operation (op, op1, op2, comment));
}

void T9900Generator::outputLabel (const std::string &label) {
    outputCode (T9900Op::def_label, label);
}

void T9900Generator::outputGlobal (const std::string &name, const std::size_t size) {
    globalDefinitions.push_back ({name, size});
}

void T9900Generator::outputComment (const std::string &s) {
    outputCode (T9900Op::comment, T9900Operand (), T9900Operand (), s);
}

void T9900Generator::outputLocalJumpTables () {
    if (!jumpTableDefinitions.empty ()) {
        outputComment ("jump tables for case statements");
        for (const TJumpTable &it: jumpTableDefinitions) {
            outputLabel (it.tableLabel);
            for (const std::string &s: it.jumpLabels)
                outputCode (T9900Op::data, T9900Operand (s.empty () ? it.defaultLabel : s));
        }
        jumpTableDefinitions.clear ();
    }
}

void T9900Generator::outputGlobalConstants () {
    if (!stringDefinitions.empty ()) {
        outputComment (std::string ());
        outputComment ("String Constants");
        for (const TStringDefinition &s: stringDefinitions)
            outputCode (T9900Op::stri, T9900Operand (s.label), T9900Operand (s.val));
    }
/*    
    for (const TConstantDefinition &c: constantDefinitions) {
        union {
            double d;
            std::uint64_t n;
        };
        outputLabel (c.label);
        d = c.val;
        outputCode (TX64Op::data_dq, n, TX64Operand (), std::to_string (d));
    }
    if (!setDefinitions.empty ()) {
        outputComment ("; Set Constants");
        for (const TSetDefinition &s: setDefinitions) {
            outputLabel (s.label);
            for (std::uint64_t v: s.val)
                outputCode (TX64Op::data_dq, v);
        }    
    }
*/    
}

std::string T9900Generator::registerConstant (double v) {
    for (TConstantDefinition &c: constantDefinitions)
        if (c.val == v)
            return c.label;
    constantDefinitions.push_back ({"__dbl_cnst_" + std::to_string (dblConstCount++), v});
    return constantDefinitions.back ().label;
}

std::string T9900Generator::registerConstant (const std::array<std::uint64_t, TConfig::setwords> &v) {
    setDefinitions.push_back ({"__set_cnst_" + std::to_string (dblConstCount++), v});
    return setDefinitions.back ().label;
}

std::string T9900Generator::registerConstant (const std::string &s) {
    for (TStringDefinition &c: stringDefinitions)
        if (c.val == s)
            return c.label;
    stringDefinitions.push_back ({"__str_cnst_" + std::to_string (dblConstCount++), s});
    return stringDefinitions.back ().label;
}

void T9900Generator::registerLocalJumpTable (const std::string &tableLabel, const std::string &defaultLabel, std::vector<std::string> &&jumpLabels) {
    jumpTableDefinitions.emplace_back (TJumpTable {tableLabel, defaultLabel, std::move (jumpLabels)});
}

void T9900Generator::outputLabelDefinition (const std::string &label, const std::size_t value) {
    labelDefinitions [label] = value;
}

void T9900Generator::generateCode (TTypeCast &typeCast) {
    TExpressionBase *expression = typeCast.getExpression ();
    TType *destType = typeCast.getType (),
          *srcType = expression->getType ();
    
    visit (expression);
    
    if (srcType == &stdType.Int64 && destType == &stdType.Real) {
        //
    }  else if (srcType == &stdType.String && destType->isPointer ()) {
        //
    }
}

void T9900Generator::generateCode (TExpression &comparison) {
    outputBinaryOperation (comparison.getOperation (), comparison.getLeftExpression (), comparison.getRightExpression ());
}

void T9900Generator::outputBooleanCheck (TExpressionBase *expr, const std::string &label, bool branchOnFalse) {
    static const std::map<TToken, std::vector<T9900Op>> intTrueJmp = {
        {TToken::Equal, {T9900Op::jne}}, 
        {TToken::GreaterThan, {T9900Op::jlt, T9900Op::jeq}},
        {TToken::LessThan, {T9900Op::jgt, T9900Op::jeq}}, 
        {TToken::GreaterEqual, {T9900Op::jlt}}, 
        {TToken::LessEqual, {T9900Op::jgt}},
        {TToken::NotEqual, {T9900Op::jeq}}
    };
/*    
    static const std::map<TToken, TX64Op> realFalseJmp = {
        {TToken::Equal, TX64Op::jne}, {TToken::GreaterThan, TX64Op::jbe}, {TToken::LessThan, TX64Op::jae}, 
        {TToken::GreaterEqual, TX64Op::jb}, {TToken::LessEqual, TX64Op::ja}, {TToken::NotEqual, TX64Op::je}
    };
*/    
    static const std::map<TToken, std::vector<T9900Op>> intFalseJmp = {
        {TToken::Equal, {T9900Op::jeq}}, 
        {TToken::GreaterThan, {T9900Op::jgt}},
        {TToken::LessThan, {T9900Op::jlt}}, 
        {TToken::GreaterEqual, {T9900Op::jgt, T9900Op::jeq}}, 
        {TToken::LessEqual, {T9900Op::jlt, T9900Op::jeq}},
        {TToken::NotEqual, {T9900Op::jne}}
    };
/*    
    static const std::map<TToken, TX64Op> realTrueJmp = {
        {TToken::Equal, TX64Op::je}, {TToken::GreaterThan, TX64Op::ja}, {TToken::LessThan, TX64Op::jb}, 
        {TToken::GreaterEqual, TX64Op::jae}, {TToken::LessEqual, TX64Op::jbe}, {TToken::NotEqual, TX64Op::jne}
    };
*/    
    
    if (TPrefixedExpression *prefExpr = dynamic_cast<TPrefixedExpression *> (expr))
        if (prefExpr->getOperation () == TToken::Not) {
            branchOnFalse = !branchOnFalse;
            expr = prefExpr->getExpression ();
        }
    
    if (expr->isFunctionCall ()) {
        TFunctionCall *functionCall = static_cast<TFunctionCall *> (expr);
        if (static_cast<TRoutineValue *> (functionCall->getFunction ())->getSymbol ()->getExtSymbolName () == "rt_in_set") {
/*        
            visit (functionCall->getArguments () [0]);
            visit (functionCall->getArguments () [1]);
            const TX64Reg r1 = fetchReg (intScratchReg2);
            const TX64Reg r2 = fetchReg (intScratchReg1);
            std::int64_t minval = 0, maxval = std::numeric_limits<std::int64_t>::max ();
            getSetTypeLimit (functionCall->getArguments () [0], minval, maxval);
            if (minval < 0 || maxval >= static_cast<std::int64_t> (TConfig::setLimit)) {
                outputCode (TX64Op::cmp, r2, 256);
                outputCode (TX64Op::jae, label);
            }
            outputCode (TX64Op::bt, TX64Operand (r1, 0), r2);
            outputCode (branchOnFalse ? TX64Op::jnc : TX64Op::jc, label);
            return;
*/            
        }
    }
    
    TExpression *condition = dynamic_cast<TExpression *> (expr);
    
    if (condition && (condition->getLeftExpression ()->getType ()->isEnumerated () || condition->getLeftExpression ()->getType ()->isPointer ())) {
        visit (condition->getLeftExpression ());
        TExpressionBase *right = condition->getRightExpression ();
        if (right->isConstant ())
            outputCode (T9900Op::ci, fetchReg (intScratchReg1), T9900Operand (static_cast<TConstantValue *> (right)->getConstant ()->getInteger ()));
        else {
            visit (condition->getRightExpression ());
            const T9900Reg r2 = fetchReg (intScratchReg1);
            const T9900Reg r1 = fetchReg (intScratchReg2);
            outputCode (T9900Operation (T9900Op::c, r1, r2));
        }
        for (T9900Op op: (branchOnFalse ? intFalseJmp : intTrueJmp).at (condition->getOperation ()))
            outputCode (op, T9900Operand ("!"));
        outputCode (T9900Op::b, label);
        outputCode (T9900Op::def_label, T9900Operand ("!"));
    } else if (condition && condition->getLeftExpression ()->getType ()->isReal ()) {
        //
    } else {
        visit (expr);
        const T9900Reg reg = fetchReg (intScratchReg1);
        outputCode (T9900Op::ci, reg, 0);
        outputCode (branchOnFalse ? T9900Op::jne : T9900Op::jeq, T9900Operand ("!"));
        outputCode (T9900Op::b, label);
        outputCode (T9900Op::def_label, T9900Operand ("!"));
    }
}

void T9900Generator::outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    visit (left);
    T9900Reg reg = fetchReg (intScratchReg1);
    outputCode (T9900Op::ci, reg, 1);
    const std::string doneLabel = getNextLocalLabel ();
    outputCode (T9900Operation (operation == TToken::Or ? T9900Op::jeq : T9900Op::jne, doneLabel));;
    visit (right);
    reg = fetchReg (intScratchReg1);
    outputLabel (doneLabel);
    saveReg (reg);
}

void T9900Generator::outputPointerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    visit (left);
    visit (right);   	
    const std::size_t basesize = std::max<std::size_t> (left->getType ()->getBaseType ()->getSize (), 1);
    if (operation == TToken::Add) {
        loadReg (T9900Reg::r12);
        const T9900Reg r1 = fetchReg (intScratchReg1);
        codeMultiplyConst (T9900Reg::r12, basesize);
        outputCode (T9900Op::a, T9900Reg::r12, r1);
        saveReg (r1);
    } else {
        const T9900Reg r2 = fetchReg (intScratchReg1);
        for (std::size_t i = 1; i < 16; ++i)
            if (basesize == 1ull << i) {
                const T9900Reg r1 = fetchReg (intScratchReg2);
                outputCode (T9900Op::s, r2, r1);
                outputCode (T9900Op::li, T9900Reg::r0, i);
                outputCode (T9900Op::sra, r1, T9900Reg::r0);
                saveReg (r1);
                return;
            }
        outputCode (T9900Op::clr, T9900Reg::r12);
        loadReg (T9900Reg::r13);
        outputCode (T9900Op::s, r2, T9900Reg::r12);
        outputCode (T9900Op::li, T9900Reg::r14, basesize);
        outputCode (T9900Op::div, T9900Reg::r14, T9900Reg::r12);
        saveReg (T9900Reg::r13);
    }
}

void T9900Generator::outputIntegerCmpOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    static const std::map<TToken, std::vector<T9900Op>> cmpOperation = {
        {TToken::Equal, {T9900Op::jne}}, 
        {TToken::GreaterThan, {T9900Op::jlt, T9900Op::jeq}},
        {TToken::LessThan, {T9900Op::jgt, T9900Op::jeq}}, 
        {TToken::GreaterEqual, {T9900Op::jlt}}, 
        {TToken::LessEqual, {T9900Op::jgt}},
        {TToken::NotEqual, {T9900Op::jeq}}
    };
        
    bool secondFirst = left->isSymbol () || left->isConstant () || left->isLValueDereference ();
    T9900Reg r1, r2, r3, r4;
    
    if (secondFirst) {
        visit (right);
        visit (left);
        r1 = fetchReg (intScratchReg2);
        r3 = r2 = fetchReg (intScratchReg1);
    } else {
        visit (left);
        visit (right);   	
        r2 = fetchReg (intScratchReg1);
        r3 = r1 = fetchReg (intScratchReg2);
    }
    outputCode (T9900Op::c, r1, r2);
    outputCode (T9900Op::clr, r3);
    for (T9900Op op: cmpOperation.at (operation))
        outputCode (op, T9900Operand ("!"));
    outputCode (T9900Op::inc, r3);
    outputCode (T9900Op::def_label, T9900Operand ("!"));
    saveReg (r3);
}

void T9900Generator::outputIntegerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    if (operation == TToken::DivInt || operation == TToken::Mod) {
        visit (right);
        T9900Reg reg = getSaveReg (intScratchReg2);
        saveReg (reg);
        visit (left);
        T9900Reg rl = fetchReg (intScratchReg1);
        outputCode (T9900Op::clr, reg);
        fetchReg (intScratchReg2);
        T9900Reg rr = fetchReg (intScratchReg3);
        outputCode (T9900Op::div, rr, reg);
        if (operation == TToken::DivInt)
            outputCode (T9900Op::mov, reg, rr);
        else
            outputCode (T9900Op::mov, rl, rr);
        saveReg (rr);
        return;
    }


    visit (left);
    visit (right);
    
    switch (operation) {
        case TToken::DivInt:
        case TToken::Mod: {
            T9900Reg r = fetchReg (intScratchReg1);
            loadReg (intScratchReg3);
            outputCode (T9900Op::clr, intScratchReg2);
            outputCode (T9900Op::div, r, intScratchReg2);
            saveReg (operation == TToken::DivInt ? intScratchReg2 : intScratchReg3);
            break; }
        case TToken::Mul: {
            T9900Reg right = fetchReg (intScratchReg2),
                     left = fetchReg (intScratchReg1);
            outputCode (T9900Op::mpy, left, right);
            outputCode (T9900Op::mov, static_cast<T9900Reg> (static_cast<std::size_t> (right) + 1), left);
            saveReg (left);
            break; }
        case TToken::Shl:
        case TToken::Shr: {
            loadReg (T9900Reg::r0);
            T9900Reg r = fetchReg (intScratchReg2);
            outputCode (operation == TToken::Shl ? T9900Op::sla : T9900Op::sra, r, T9900Reg::r0);
            saveReg (r);
            break; }
        default: {
            T9900Reg right = fetchReg (intScratchReg1),
                     left = fetchReg (intScratchReg2);
            static const std::map<TToken, T9900Op> opMap = {
                {{TToken::Add, T9900Op::a}, {TToken::Sub, T9900Op::s}, {TToken::Or, T9900Op::soc}, 
                 {TToken::Xor, T9900Op::xor_}, {TToken::And, T9900Op::szc}}
            };
            if (operation == TToken::And)
                outputCode (T9900Op::inv, right);
            outputCode (opMap.at (operation), right, left);
            saveReg (left); 
            break; }
    }
}

void T9900Generator::outputFloatOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
/*
    static const std::map<TToken, TX64Op> dblOperation = {
        {TToken::Add, TX64Op::addsd}, {TToken::Sub, TX64Op::subsd}, {TToken::Mul, TX64Op::mulsd},
        {TToken::Div, TX64Op::divsd},
        {TToken::Equal, TX64Op::setz}, {TToken::GreaterThan, TX64Op::seta}, {TToken::LessThan, TX64Op::setb}, 
        {TToken::GreaterEqual, TX64Op::setae}, {TToken::LessEqual, TX64Op::setbe}, {TToken::NotEqual, TX64Op::setnz}
    };
    TX64Reg r1, r2;
    
    bool secondFirst = (left->isSymbol () || left->isConstant () || left->isLValueDereference ()) && operation != TToken::Sub && operation != TToken::Div;
    if (secondFirst) {
        visit (right);
        visit (left);
        r1 = fetchXmmReg (xmmScratchReg1);
        r2 = fetchXmmReg (xmmScratchReg2);
    } else {
        visit (left);
        visit (right);   	
        r2 = fetchXmmReg (xmmScratchReg2);
        r1 = fetchXmmReg (xmmScratchReg1);
    }
    
    if (std::find (std::begin (relOps), std::end (relOps), operation) == std::end (relOps)) {
        if (secondFirst)
            std::swap (r1, r2);
        outputCode (TX64Operation (dblOperation.at (operation), r1, r2));
        saveXmmReg (r1);
    } else {
        TX64Reg reg = getSaveReg (intScratchReg1);
        outputCode (TX64Op::comisd, r1, r2);
        outputCode (dblOperation.at (operation), TX64Operand (reg, TX64OpSize::bit8));
        outputCode (TX64Op::movzx, TX64Operand (reg, TX64OpSize::bit32), TX64Operand (reg, TX64OpSize::bit8));
        saveReg (reg);
    }
*/    
}

void T9900Generator::outputBinaryOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    const TType *lType = left->getType ();
    
    if (lType == &stdType.Boolean && (operation == TToken::And || operation == TToken::Or)) 
        outputBooleanShortcut (operation, left, right);
    else if (lType->isPointer () && (operation == TToken::Add || operation == TToken::Sub)) 
        outputPointerOperation (operation, left, right);
    else if (lType->isEnumerated () || lType->isPointer ()) 
        if (std::find (std::begin (relOps), std::end (relOps), operation) != std::end (relOps)) 
            outputIntegerCmpOperation (operation, left, right);
        else
           outputIntegerOperation (operation, left, right);
    else
        outputFloatOperation (operation, left, right);
}    

void T9900Generator::generateCode (TPrefixedExpression &prefixed) {
    const TType *type = prefixed.getExpression ()->getType ();
    if (type == &stdType.Real) {
/*    
        TX64Reg resreg = getSaveXmmReg (xmmScratchReg1);
        outputCode (TX64Operation (TX64Op::pxor, resreg, resreg));
        saveXmmReg (resreg);
        visit (prefixed.getExpression ());
        const TX64Reg r2 = fetchXmmReg (xmmScratchReg1),
                      r1 = fetchXmmReg (xmmScratchReg2);
        outputCode (TX64Operation (TX64Op::subsd, r1, r2));
        saveXmmReg (r1);
*/        
    } else {
        visit (prefixed.getExpression ());
        const T9900Reg reg = fetchReg (intScratchReg1);
        if (prefixed.getOperation () == TToken::Sub)
            outputCode (T9900Op::neg, reg);
        else if (prefixed.getOperation () == TToken::Not) 
            if (type == &stdType.Boolean) {
                outputCode (T9900Op::li, intScratchReg2, 1);
                outputCode (T9900Op::xor_, intScratchReg2,  reg);
            } else
                outputCode (T9900Op::inv, reg);
        else {
            std::cout << "Internal Error: invalid prefixed expression" << std::endl;
            exit (1);
        }
        saveReg (reg);
    }
}

void T9900Generator::generateCode (TSimpleExpression &expression) {
    outputBinaryOperation (expression.getOperation (), expression.getLeftExpression (), expression.getRightExpression ());
}

void T9900Generator::generateCode (TTerm &term) {
    outputBinaryOperation (term.getOperation (), term.getLeftExpression (), term.getRightExpression ());
}

void T9900Generator::codeInlinedFunction (TFunctionCall &functionCall) {
/*
    TExpressionBase *function = functionCall.getFunction ();
    const std::vector<TExpressionBase *> &args = functionCall.getArguments ();
    const std::string s = static_cast<TRoutineValue *> (function)->getSymbol ()->getExtSymbolName ();

    if (s == "rt_dbl_abs") {
        visit (args [0]);
        const TX64Reg reg = fetchXmmReg (xmmScratchReg1);
        outputCode (TX64Op::andpd, reg, TX64Operand (dblAbsMask, true));
        saveXmmReg (reg);
    } else if (s == "sqrt") {
        visit (args [0]);
        const TX64Reg reg = fetchXmmReg (xmmScratchReg1);
        outputCode (TX64Op::sqrtsd, reg, reg);
        saveXmmReg (reg);
    } else if (s == "ntohs" || s == "htons") {
        visit (args [0]);
        const TX64Reg reg = fetchReg (intScratchReg1);
        outputCode (TX64Op::ror, TX64Operand (reg, TX64OpSize::bit16), 8);
        saveReg (reg);
    } else if (s == "rt_in_set") {
        visit (args [0]); visit (args [1]);
        const TX64Reg r1 = fetchReg (intScratchReg2);
        const TX64Reg r2 = fetchReg (intScratchReg1);
        const std::string l1 = getNextLocalLabel (), l2 = getNextLocalLabel ();
        
        std::int64_t minval = 0, maxval = std::numeric_limits<std::int64_t>::max ();
        getSetTypeLimit (args [0], minval, maxval);
        if (minval < 0 || maxval >= static_cast<std::int64_t> (TConfig::setLimit)) {
            outputCode (TX64Op::cmp, r2, 256);
            outputCode (TX64Op::jb, l1);
            outputCode (TX64Op::mov, r1, 0);
            outputCode (TX64Op::jmp, l2);
            outputLabel (l1);
        }
        outputCode (TX64Op::bt, TX64Operand (r1, 0), r2);
        outputCode (TX64Op::setc, TX64Operand (r1, TX64OpSize::bit8));
        outputCode (TX64Op::movsx, r1, TX64Operand (r1, TX64OpSize::bit8));
        
        outputLabel (l2);
        saveReg (r1);
    }
*/    
}

bool T9900Generator::isFunctionCallInlined (TFunctionCall &functionCall) {
    TExpressionBase *function = functionCall.getFunction ();
    if (function->isRoutine ()) {
        const std::string s = static_cast<TRoutineValue *> (function)->getSymbol ()->getExtSymbolName ();
        return s == "rt_dbl_abs" || s == "sqrt" || s == "ntohs" || s == "htons" || s == "rt_in_set";
    } else
        return false;        
}

bool T9900Generator::isReferenceCallerCopy (const TType *type) {
    return false;
}

void T9900Generator::generateCode (TFunctionCall &functionCall) {
    if (isFunctionCallInlined (functionCall)) {
        codeInlinedFunction (functionCall);
        return;
    }

    TExpressionBase *function = functionCall.getFunction ();
    const std::vector<TExpressionBase *> &args = functionCall.getArguments ();

    struct TParameterDescription {
        TType *type;
        TExpressionBase *actualParameter;
        bool isInteger;
        std::size_t offset;
    };
    std::vector<TParameterDescription> parameterDescriptions;
/*    
    for (const TSymbol *s: static_cast<TRoutineType *> (function->getType ())->getParameter ()) {
        std::cout << s->getName () << ": " << s->getType ()->getName () << std::endl;
    }
*/    
    bool usesReturnValuePointer = true;
    TExpressionBase *returnStorage = functionCall.getReturnStorage ();
    
    std::vector<TExpressionBase *>::const_iterator it = args.begin ();
    std::size_t offCount = 2 * (returnStorage != nullptr);
    for (const TSymbol *s: static_cast<TRoutineType *> (function->getType ())->getParameter ()) {
        TParameterDescription pd (s->getType (), *it, classifyType (pd.type) == TParameterLocation::IntReg, offCount);
        parameterDescriptions.push_back (pd);
        offCount += (s->getType ()->getSize () + 1) & ~1;
        ++it;
    }
/*    
    std::cout << "Function call:" << std::endl << "  Stack: " << stackCount << std::endl;
    for (TParameterDescription &pd: parameterDescriptions) {
        std::cout << pd.type->getName () << " -> ";
        if (pd.intRegNr != -1) 
            std::cout << x64RegName [static_cast<int> (intParaReg [pd.intRegNr])];
        if (pd.xmmRegNr != -1) 
            std::cout << x64RegName [static_cast<int> (xmmParaReg [pd.xmmRegNr])];
        if (pd.stackOffset != -1)
            std::cout << "SP + " << pd.stackOffset;
        std::cout << " late: " << pd.evaluateLate;
        std::cout << std::endl;
    }
    std::cout << std::endl;
*/

    if (offCount)
        outputCode (T9900Op::ai, T9900Reg::r10, -offCount);

    std::size_t stackCount = 0;
    for (ssize_t i = parameterDescriptions.size () - 1; i >= 0; --i) {
        visit (parameterDescriptions [i].actualParameter);
        if (parameterDescriptions [i].isInteger) {
            const T9900Reg reg = fetchReg (intScratchReg1);
            if (parameterDescriptions [i].type->getSize () == 1)
                outputCode (T9900Op::swpb, reg);            
            if (parameterDescriptions [i].offset)
                outputCode (T9900Op::mov, reg, T9900Operand (T9900Reg::r10, parameterDescriptions [i].offset));
            else
                outputCode (T9900Op::mov, reg, T9900Operand (T9900Reg::r10, T9900Operand::TAddressingMode::RegInd));
            stackCount += 2;
        } else {
            T9900Reg reg = getSaveReg (intScratchReg1);
            outputCode (T9900Op::mov, T9900Reg::r10, reg);
            outputCode (T9900Op::ai, reg, parameterDescriptions [i].offset);
            saveReg (reg);
            codeMove (parameterDescriptions [i].type);
        }
    }
    
    if (usesReturnValuePointer && returnStorage) {
        visit (returnStorage);
        const T9900Reg reg = fetchReg (intScratchReg1);
        outputCode (T9900Op::mov, reg, T9900Operand (T9900Reg::r10, T9900Operand::TAddressingMode::RegInd));
//        codePush (reg);
        stackCount += 2;
    }
    
    visit (function);
    const T9900Reg reg = fetchReg (intScratchReg1);
    outputCode (T9900Op::bl, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd));
//    if (stackCount)
//        outputCode (T9900Op::ai, T9900Reg::r10, stackCount);

    if (functionCall.getType () != &stdType.Void && !functionCall.isIgnoreReturn ())
        visit (functionCall.getReturnStorageDeref ());
}

void T9900Generator::generateCode (TConstantValue &constant) {
    if (constant.getType ()->isSet ()) {
        //    
    } else if (constant.getType () == &stdType.Real) {
        //
    } else if (constant.getConstant ()->getType () == &stdType.String) {
        T9900Reg reg = getSaveReg (intScratchReg1);
        const std::string label = registerConstant (constant.getConstant ()->getString ());
        outputCode (T9900Op::li, reg, T9900Operand (label));
        saveReg (reg);
    } else {
        T9900Reg reg = getSaveReg (intScratchReg1);
        const std::int16_t n = constant.getConstant ()->getInteger ();
        outputCode (T9900Op::li, reg, n);
        saveReg (reg);
    }
}

void T9900Generator::generateCode (TRoutineValue &routineValue) {
    T9900Reg reg = getSaveReg (intScratchReg1);
    TSymbol *s = routineValue.getSymbol ();
    if (s->checkSymbolFlag (TSymbol::External))
        outputCode (T9900Operation (T9900Op::li, reg, s->getExtSymbolName ()));
    else
        outputCode (T9900Operation (T9900Op::li, reg, s->getOverloadName ()));
    saveReg (reg);
}

void T9900Generator::codeSymbol (const TSymbol *s, const T9900Reg reg) {
    if (s->getLevel () == 1)	// global
        outputCode (T9900Op::li, reg, s->getOffset (), s->getOverloadName ());
    else if (s->getLevel () == currentLevel) {
        outputCode (T9900Op::mov, T9900Reg::r9, reg), 
        outputCode (T9900Op::ai, reg, s->getOffset (), s->getName ());
    } else {
        outputCode (T9900Op::mov, T9900Operand (T9900Reg::r9, 2 * (s->getLevel () - currentLevel + 1)), reg);
        outputCode (T9900Op::ai, reg, s->getOffset (), s->getName ());
    }
}

void T9900Generator::codeStoreMemory (TType *const t, T9900Operand destMem, T9900Reg srcReg) {
    if (t == &stdType.Uint8 || t == &stdType.Int8) {
        // WP should always be at 8300 so code direct mov?
        outputCode (T9900Op::movb, T9900Operand (T9900Reg::r0, 0x8300 + 2 * static_cast<int> (srcReg) + 1), destMem, "low R" + std::to_string (static_cast<std::size_t> (srcReg)));
//        outputCode (T9900Op::swpb, srcReg);
//        outputCode (T9900Op::movb, srcReg, destMem);
//        outputCode (T9900Op::swpb, srcReg);
    } else if (t == &stdType.Uint16 || t == &stdType.Int16) {
        outputCode (T9900Op::mov, srcReg, destMem);
    } else if (t == &stdType.Uint32 || t == &stdType.Int32) {
        //
    } else if (t == &stdType.Int64) {
        //
    } else if (t == &stdType.Real) {
        //
    } else if (t == &stdType.Single) {
        //
    }
}

void T9900Generator::codeSignExtension (TType *const t, const T9900Reg destReg, T9900Operand srcOperand) {
    if (t == &stdType.Int8) {
        outputCode (T9900Op::movb, srcOperand, destReg);
        outputCode (T9900Op::sra, destReg, 8);
    } else if (t == &stdType.Uint8) {
        outputCode (T9900Op::movb, srcOperand, destReg);
        outputCode (T9900Op::srl, destReg, 8);
    } else
        outputCode (T9900Op::mov, srcOperand, destReg);
}

void T9900Generator::codeLoadMemory (TType *const t, const T9900Reg regDest, const T9900Operand srcMem) {
    if (t->isSubrange ())
        codeSignExtension (t, regDest, srcMem);
    else if (t == &stdType.Int64) {
        //
    } else if (t == &stdType.Real) {
        //
    } else if (t == &stdType.Single) {
        //
    }
}

void T9900Generator::codeMultiplyConst (const T9900Reg reg, const std::size_t n) {
    if (n == 1)
        return;
    else if (n >= 2)
        for (std::size_t i = 1; i < 16; ++i)
            if (n == 1ull << i) {
                outputCode (T9900Op::sla, reg, i);
                return;
            }
    if (n == 0) 
        outputCode (T9900Op::clr, reg);
    else {
        outputCode (T9900Op::li, intScratchReg1, n);
        outputCode (T9900Op::mpy, reg, intScratchReg2);
        outputCode (T9900Op::mov, intScratchReg3, reg);
    }
}

void T9900Generator::codeMove (const TType *type) {
    std::size_t n = type->getSize ();
    if (n) {
        loadReg (intScratchReg3);
        loadReg (intScratchReg2);
        outputCode (T9900Op::li, intScratchReg4, n);
        if (type->isShortString ())
            outputCode (T9900Op::bl, T9900Operand ("_rt_copy_str"));
        else
            outputCode (T9900Op::bl, T9900Operand ("_rt_copy_mem"));
    }
}

void T9900Generator::codeRuntimeCall (const std::string &funcname, const T9900Reg globalDataReg, const std::vector<std::pair<T9900Reg, std::size_t>> &additionalArgs) {
/*
    for (const std::pair<TX64Reg, std::size_t> &arg: additionalArgs)
        outputCode (TX64Op::mov, arg.first, arg.second);
    codeSymbol (globalRuntimeDataSymbol, TX64Reg::rax);
    outputCode (TX64Op::mov, globalDataReg, TX64Operand (TX64Reg::rax, 0));
    outputCode (TX64Op::mov, TX64Reg::rax, funcname);
    outputCode (TX64Op::call, TX64Reg::rax);
*/    
}

void T9900Generator::codePush (const T9900Operand op) {
    outputCode (T9900Op::dect, T9900Reg::r10);
    outputCode (T9900Op::mov, op, T9900Operand (T9900Reg::r10, T9900Operand::TAddressingMode::RegInd));
}

void T9900Generator::codePop (const T9900Operand op) {
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r10, T9900Operand::TAddressingMode::RegIndInc), op);
}

void T9900Generator::saveReg (const T9900Reg reg) {
    if (intStackCount < intStackRegs) {
        if (reg != intStackReg [intStackCount])
            outputCode (T9900Op::mov, reg, intStackReg [intStackCount]);
    } else
        codePush (reg);
    ++intStackCount;
}

void T9900Generator::loadReg (const T9900Reg reg) {
    --intStackCount;
    if (intStackCount >= intStackRegs) 
        codePop (reg);
    else
        outputCode (T9900Op::mov, intStackReg [intStackCount], reg);
}

T9900Reg T9900Generator::fetchReg (T9900Reg reg) {
    --intStackCount;
    if (intStackCount >= intStackRegs) {
        codePop (reg);
        return reg;
    } else
        return intStackReg [intStackCount];
}

T9900Reg T9900Generator::getSaveReg (T9900Reg reg) {
    if (intStackCount >= intStackRegs) 
        return reg;
    return intStackReg [intStackCount];
}

void T9900Generator::clearRegsUsed () {
    std::fill (regsUsed.begin (), regsUsed.end (), false);
}

void T9900Generator::setRegUsed (T9900Reg r) {
    regsUsed [static_cast<std::size_t> (r)] = true;
}

bool T9900Generator::isRegUsed (const T9900Reg r) const {
    return regsUsed [static_cast<std::size_t> (r)];
}

void T9900Generator::codeModifySP (ssize_t n) {
    if (n)
        outputCode (T9900Op::ai, T9900Reg::r10, n);
}

void T9900Generator::generateCode (TVariable &variable) {
    const T9900Reg reg = getSaveReg (intScratchReg1);
    codeSymbol (variable.getSymbol (), reg);
    saveReg (reg);
}

void T9900Generator::generateCode (TReferenceVariable &referenceVariable) {
    const T9900Reg reg = getSaveReg (intScratchReg1);
    codeSymbol (referenceVariable.getSymbol (), reg);
    outputCode (T9900Op::mov, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd), reg);
    saveReg (reg);
}

void T9900Generator::generateCode (TLValueDereference &lValueDereference) {
    TExpressionBase *lValue = lValueDereference.getLValue ();
    TType *type = getMemoryOperationType (lValueDereference.getType ());
    
    visit (lValue);
    // leave pointer on stack if not scalar type
    
    if (type != &stdType.UnresOverload) {
        if (type == &stdType.Real || type == &stdType.Single) {
            //
        } else {
            const T9900Reg reg = fetchReg (intScratchReg1);
            codeLoadMemory (type, reg, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd));
            saveReg (reg);
        }
    }
}

void T9900Generator::generateCode (TArrayIndex &arrayIndex) {
    TExpressionBase *base = arrayIndex.getBaseExpression (),
                    *index = arrayIndex.getIndexExpression ();
    const TType *type = base->getType ();
    const ssize_t minVal = type->isPointer () ? 0 :  static_cast<TEnumeratedType *> (index->getType ())->getMinVal ();
    const std::size_t baseSize = arrayIndex.getType ()->getSize ();
    
    if (index->isTypeCast ())
        index = static_cast<TTypeCast *> (index)->getExpression ();
        
    if (type == &stdType.String) {
        // cannot be used - remove AnsiString from predefined types for 9900
    } else {
        visit (base);
        if (type->isPointer ()) {
            // TODO: unify with TPointerDereference
            const T9900Reg reg = fetchReg (intScratchReg1);
            outputCode (T9900Op::mov, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd), reg);
            saveReg (reg);
        } 
            
        visit (index);
        const T9900Reg indexReg = fetchReg (intScratchReg1);
        if (minVal) 
            outputCode (T9900Op::ai, indexReg, -minVal);
        codeMultiplyConst (indexReg, baseSize);
        
        const T9900Reg baseReg = fetchReg (intScratchReg2);
        outputCode (T9900Op::a, indexReg, baseReg);
        saveReg (baseReg);
    }
}

void T9900Generator::generateCode (TRecordComponent &recordComponent) {
    visit (recordComponent.getExpression ());
    TRecordType *recordType = static_cast<TRecordType *> (recordComponent.getExpression ()->getType ());
    if (const TSymbol *symbol = recordType->searchComponent (recordComponent.getComponent ())) {
        if (symbol->getOffset ()) {
            const T9900Reg reg = fetchReg (intScratchReg1);
            outputCode (T9900Op::ai, reg, symbol->getOffset (), "." + std::string (symbol->getName ()));
            saveReg (reg);
        }
    } else {
        // INternal Error: Component not found !!!!
    }
}

void T9900Generator::generateCode (TPointerDereference &pointerDereference) {
    TExpressionBase *base = pointerDereference.getExpression ();
    visit (base);
    if (base->isLValue ()) {
        // as in array: should have method deref-stacktop
        const T9900Reg reg = fetchReg (intScratchReg1);
        outputCode (T9900Op::mov, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd), reg);
        saveReg (reg);
    }
}

void T9900Generator::codeIncDec (TPredefinedRoutine &predefinedRoutine) {
    bool isIncOp = predefinedRoutine.getRoutine () == TPredefinedRoutine::Inc;
    const std::vector<TExpressionBase *> &arguments = predefinedRoutine.getArguments ();
    TType *type = arguments [0]->getType ();
    
    ssize_t n = type->isPointer () ? static_cast<const TPointerType *> (type)->getBaseType ()->getSize () : 1;
    T9900Reg value;    
    
    visit (arguments [0]);
    if (arguments.size () > 1) {
        visit (arguments [1]);
        value = fetchReg (intScratchReg2);
        if (n > 1)
            codeMultiplyConst (value, n);
    } else {
        value = getSaveReg (intScratchReg2);
        outputCode (T9900Op::li, value, n);
    }

    T9900Operand varAddr (fetchReg (intScratchReg1), T9900Operand::TAddressingMode::RegInd);
    
    if (arguments [0]->getType ()->getSize () == 1) {
        outputCode (T9900Op::swpb, value);
        outputCode (isIncOp ? T9900Op::ab : T9900Op::sb, value, varAddr);
    } else
        outputCode (isIncOp ? T9900Op::a : T9900Op::s, value, varAddr);
    // TODO: range check
        
}

void T9900Generator::generateCode (TPredefinedRoutine &predefinedRoutine) {
    TPredefinedRoutine::TRoutine predef = predefinedRoutine.getRoutine ();
    if (predef == TPredefinedRoutine::Inc || predef == TPredefinedRoutine::Dec)
        codeIncDec (predefinedRoutine);
    else {
        const std::vector<TExpressionBase *> &arguments = predefinedRoutine.getArguments ();
        for (TExpressionBase *expression: arguments)
            visit (expression);

        bool isIncOp = false;    
        T9900Reg reg;
        switch (predefinedRoutine.getRoutine ()) {
            case TPredefinedRoutine::Inc:
            case TPredefinedRoutine::Dec:
                // alreay handled
                break;
            case TPredefinedRoutine::Odd:
                reg = fetchReg (intScratchReg1);
                outputCode (T9900Op::andi, reg, 1);
                saveReg (reg);
                break;
            case TPredefinedRoutine::Succ:
                isIncOp = true;
                // fall through
            case TPredefinedRoutine::Pred:
                reg = fetchReg (intScratchReg1);
                outputCode (isIncOp ? T9900Op::inc : T9900Op::dec, reg);
                saveReg (reg);
                // TODO : range check
                break;
            case TPredefinedRoutine::Exit:
                outputCode (T9900Op::b, endOfRoutineLabel);
                break;
        }
    }
}

void T9900Generator::generateCode (TAssignment &assignment) {
    TExpressionBase *lValue = assignment.getLValue (),
                    *expression = assignment.getExpression ();
    TType *type = lValue->getType (),
          *st = getMemoryOperationType (type);
         
    visit (expression);
    visit (lValue);
    if (st != &stdType.UnresOverload) {
        T9900Operand operand = T9900Operand (fetchReg (intScratchReg1), T9900Operand::TAddressingMode::RegInd);
        if (st == &stdType.Real || st == &stdType.Single) {
        /*
            codeStoreMemory (st, operand, fetchXmmReg (xmmScratchReg1));
        */            
        } else
            codeStoreMemory (st, operand, fetchReg (intScratchReg1));
    } else 
        codeMove (type);
}

void T9900Generator::generateCode (TRoutineCall &routineCall) {
    visit (routineCall.getRoutineCall ());
}

void T9900Generator::generateCode (TIfStatement &ifStatement) {
    const std::string l1 = getNextLocalLabel ();
    outputBooleanCheck (ifStatement.getCondition (), l1);
    visit (ifStatement.getStatement1 ());
    if (TStatement *statement2 = ifStatement.getStatement2 ()) {
        const std::string l2 = getNextLocalLabel ();
        outputCode (T9900Op::b, l2);
        outputLabel (l1);
        visit (statement2);
        outputLabel (l2);
    } else
        outputLabel (l1);
}

void T9900Generator::generateCode (TCaseStatement &caseStatement) {
    const int maxCaseJumpList = 32;
    
    visit (caseStatement.getExpression ());
    T9900Reg reg = fetchReg (intScratchReg2);
    
    const TCaseStatement::TCaseList &caseList = caseStatement.getCaseList ();
    const std::int64_t minLabel = caseStatement.getMinLabel (), maxLabel = caseStatement.getMaxLabel ();
    std::string endLabel = getNextLocalLabel ();
    
    if (maxLabel - minLabel >= maxCaseJumpList) {
        std::int64_t last = 0;
        outputComment ("no-opt");
        for (const TCaseStatement::TSortedEntry &e: caseStatement.getSortedLabels ()) {
            outputCode (T9900Op::ai, reg, last - e.label.a);
            last = e.label.a;
            if (e.label.a == e.label.b) {
                outputCode (T9900Op::jne, T9900Operand ("!"));
                outputCode (T9900Op::b, e.jumpLabel->getOverloadName ());
            } else {
                outputCode (T9900Op::ci, reg, e.label.b - last);
                outputCode (T9900Op::jh, T9900Operand ("!"));
                outputCode (T9900Op::b, e.jumpLabel->getOverloadName ());
            }
            outputLabel ("!");
        }
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            visit (defaultStatement);
        }
        outputCode (T9900Op::b, endLabel);

    } else {
        const std::string evalTableLabel = getNextCaseLabel ();
        std::string defaultLabel = endLabel;
        if (minLabel) 
            outputCode (T9900Op::ai, reg, -minLabel);
        outputComment ("no-opt");
        outputCode (T9900Op::ci, reg, maxLabel - minLabel);
        
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            defaultLabel = getNextCaseLabel ();
            outputCode (T9900Op::jh, T9900Operand (defaultLabel));
            outputCode (T9900Op::b, T9900Operand (evalTableLabel));
            outputLabel (defaultLabel);
            visit (defaultStatement);
            outputCode (T9900Op::b, endLabel);
        }  else {
            outputCode (T9900Op::jle, T9900Operand ("!"));
            outputCode (T9900Op::b, endLabel);
            outputLabel ("!");
        }
        
        std::vector<std::string> jumpTable (maxLabel - minLabel + 1);
        for (const TCaseStatement::TCase &c: caseList) {
            for (const TCaseStatement::TLabel &label: c.labels)
                for (std::int64_t n = label.a; n <= label.b; ++n)
                    jumpTable [n - minLabel] = c.jumpLabel->getOverloadName ();
        }
        outputLabel (evalTableLabel);
        const std::string tableLabel = getNextLocalLabel ();
        outputCode (T9900Op::sla, reg, 1);
        outputCode (T9900Op::ai, reg, T9900Operand (tableLabel));
        outputCode (T9900Op::mov, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd), reg);
        outputCode (T9900Op::b, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd));

        registerLocalJumpTable (tableLabel, defaultLabel, std::move (jumpTable));
    }
    
    for (const TCaseStatement::TCase &c: caseList) {
        outputLabel (c.jumpLabel->getOverloadName ());
        visit (c.statement);
        outputCode (T9900Op::b, endLabel);
    }    
    outputLabel (endLabel);
}

void T9900Generator::generateCode (TStatementSequence &statementSequence) {
    for (TStatement *statement: statementSequence.getStatements ())
        visit (statement);
}

void T9900Generator::outputCompare (const T9900Operand &var, const std::int64_t n) {
    outputCode (T9900Op::ci, var, n);
}

void T9900Generator::generateCode (TLabeledStatement &labeledStatement) {
    outputLabel (".lbl_" + labeledStatement.getLabel ()->getOverloadName ());
    visit (labeledStatement.getStatement ());
}

void T9900Generator::generateCode (TGotoStatement &gotoStatement) {
    if (TExpressionBase *condition = gotoStatement.getCondition ())
        outputBooleanCheck (condition, ".lbl_" + gotoStatement.getLabel ()->getOverloadName (), false);
    else
        outputCode (T9900Op::b, ".lbl_" + gotoStatement.getLabel ()->getOverloadName ());
}
    
void T9900Generator::generateCode (TBlock &block) {
    generateBlock (block);
}

void T9900Generator::generateCode (TUnit &unit) {
}

void T9900Generator::generateCode (TProgram &program) {
    stackPositions = 0;
    TSymbolList &globalSymbols = program.getBlock ()->getSymbols ();
    globalRuntimeDataSymbol = globalSymbols.searchSymbol ("__globalruntimedata");
    
    // TODO: error if not found !!!!
    generateBlock (*program.getBlock ());
    
    setOutput (&this->program);
    outputGlobalConstants ();
//    std::cout << "Data size is: " << globalSymbols.getLocalSize () << std::endl;
    
}

void T9900Generator::initStaticRoutinePtr (std::size_t addr, const TRoutineValue *routineValue) {
/*
    outputCode (TX64Op::lea, TX64Reg::rax, TX64Operand (routineValue->getSymbol ()->getOverloadName (), true));
    outputCode (TX64Op::mov, TX64Operand (TX64Reg::none, reinterpret_cast<std::uint64_t> (addr)), TX64Reg::rax);
*/    
}
    
void T9900Generator::beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &symbolList, const std::set<T9900Reg> &saveRegs, bool hasStackFrame) {
//    if (level > 1) {    
        outputComment (std::string ());
        for (const std::string &s: createSymbolList (routineName, level, symbolList, {}))
            outputComment (s);
        outputComment (std::string ());
//    }
    
    // TODO: resolve overload !
    outputLabel (routineName);

    if (level > 1) {    
        if (hasStackFrame) {    
            codePush (T9900Reg::r11);
            codePush (T9900Reg::r9);
            if (level > 2)
                outputCode (T9900Op::mov, T9900Reg::r9, intScratchReg2);
            outputCode (T9900Op::mov, T9900Reg::r10, T9900Reg::r9);
            
            if (level > 1) {
                for (std::size_t i = 0; i < level - 2; ++i)
                    codePush (T9900Operand (intScratchReg2, -2 * i));
//                codePush (T9900Reg::r9);
                if (symbolList.getLocalSize ())
                    outputCode (T9900Op::ai, T9900Reg::r10, -symbolList.getLocalSize (), "local vars");
            }
        }
        for (T9900Reg reg: saveRegs)
            codePush (reg);
    } else
        outputCode (T9900Op::li, T9900Reg::r10, 65536 - symbolList.getLocalSize (), "init stack ptr");
}

void T9900Generator::endRoutineBody (std::size_t level, TSymbolList &symbolList, const std::set<T9900Reg> &saveRegs, bool hasStackFrame) {
    if (level > 1) {
        for (std::set<T9900Reg>::reverse_iterator it = saveRegs.rbegin (); it != saveRegs.rend (); ++it)
            codePop (*it);

        if (hasStackFrame) {
            outputCode (T9900Op::mov, T9900Reg::r9, T9900Reg::r10);
            codePop (T9900Reg::r9);
            codePop (T9900Reg::r11);
        }
        if (symbolList.getParameterSize ())
            outputCode (T9900Op::ai, T9900Reg::r10, symbolList.getParameterSize ());
        outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
    } else
        outputCode (T9900Op::blwp, 0);
}

TCodeGenerator::TParameterLocation T9900Generator::classifyType (const TType *type) {
    if (type->isEnumerated () || type->isPointer () || type->isReference () || type->isRoutine ()) 
        return TParameterLocation::IntReg;
    if (type == &stdType.Real || type == &stdType.Single)
        return TParameterLocation::FloatReg;
    else
        return TParameterLocation::Stack;
}

TCodeGenerator::TReturnLocation T9900Generator::classifyReturnType (const TType *type) {
    return TReturnLocation::Reference;
}

void T9900Generator::assignParameterOffsets (TBlock &block) {
    TSymbolList &symbolList = block.getSymbols ();
    
    std::size_t valueParaOffset = 4;
    for (TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Parameter)) {
            TType *type = s->getType ();
            alignType (type);
            s->setOffset (valueParaOffset);
            valueParaOffset += ((type->getSize () + 1) & ~1);
        }
    symbolList.setParameterSize (valueParaOffset - 4);
}

void T9900Generator::assignStackOffsets (TBlock &block) {
    TSymbolList &symbolList = block.getSymbols ();
    
    const std::size_t alignment = 1,
                      level = symbolList.getLevel (),
                      displaySize = (level - 1) * 2;
    ssize_t pos = 0;
    
    assignParameterOffsets (block);
    inherited::assignStackOffsets (pos, symbolList, false);
    pos = (pos + alignment) & ~alignment;
    
    for (TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Alias))
            s->setAliasData ();
        else if (s->checkSymbolFlag (TSymbol::Variable) && s->getRegister () == TSymbol::InvalidRegister)
            s->setOffset (s->getOffset () - pos - displaySize + 2);

    symbolList.setLocalSize (pos);
}

void T9900Generator::assignRegisters (TSymbolList &blockSymbols) {
}

void T9900Generator::assignGlobalVariables (TSymbolList &blockSymbols) {
    std::size_t globalSize = blockSymbols.getLocalSize (), firstSymbolOffset,
                startAddress = 65536 - globalSize;	// TODO: check memory full!!!!
    bool firstSymbol = true;
    for (TSymbol *s: blockSymbols)
        if ((s->checkSymbolFlag (TSymbol::Variable) || s->checkSymbolFlag (TSymbol::StaticVariable)) && (!s->checkSymbolFlag (TSymbol::Absolute) ||s->checkSymbolFlag (TSymbol::Alias))) {
            if (firstSymbol) {
                firstSymbolOffset = s->getOffset ();
                firstSymbol = false;
            }
            s->setOffset (startAddress + s->getOffset () - firstSymbolOffset);
    }
}

void T9900Generator::codeBlock (TBlock &block, bool hasStackFrame, TCodeSequence &blockStatements) {
    TSymbolList &blockSymbols = block.getSymbols ();
    const std::size_t level = blockSymbols.getLevel ();
    
    TCodeSequence blockPrologue, blockEpilogue, blockCode, globalInits;

    stackPositions = 0;
    intStackCount = 0;
    xmmStackCount = 0;
    currentLevel =  blockSymbols.getLevel ();
    endOfRoutineLabel = getNextLocalLabel ();

    setOutput (&globalInits);
    if (blockSymbols.getLevel () == 1) {
        assignGlobalVariables (blockSymbols);
        initStaticGlobals (blockSymbols);
    }
    
    setOutput (&blockCode);
    
    if (!globalInits.empty ()) 
        outputCode (T9900Op::bl, T9900Operand ("$init_static"));
    visit (block.getStatements ());
    
    outputLabel (endOfRoutineLabel);
//    logOptimizer = block.getSymbol ()->getOverloadName () == "gettapeinput_$182";
    
    removeUnusedLocalLabels (blockCode);
    optimizePeepHole (blockCode);
//    tryLeaveFunctionOptimization (blockCode);
    
    // check which callee saved registers are used after peep hole optimization
    std::set<T9900Reg> saveRegs;
    for (T9900Operation &op: blockCode) {
        if (isCalleeSavedReg (op.operand1))
            saveRegs.insert (op.operand1.reg);
        if (isCalleeSavedReg (op.operand2))
            saveRegs.insert (op.operand2.reg);
    }
    
    stackPositions = 0;    
    
    setOutput (&blockPrologue);    
    beginRoutineBody (block.getSymbol ()->getOverloadName (), blockSymbols.getLevel (), blockSymbols, saveRegs, hasStackFrame);
    optimizePeepHole (blockPrologue);
    
    setOutput (&blockEpilogue);
    endRoutineBody (blockSymbols.getLevel (), blockSymbols, saveRegs, hasStackFrame);
    optimizePeepHole (blockEpilogue);
    outputLocalJumpTables ();
    
    if (!globalInits.empty ()) {
        optimizePeepHole (globalInits);
        outputLabel ("$init_static");
        std::move (globalInits.begin (), globalInits.end (), std::back_inserter (blockEpilogue));
        outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
    }
    
    blockStatements.clear ();    
    // blockStatements.reserve (blockPrologue.size () + blockCode.size () + blockEpilogue.size ());
    std::move (blockPrologue.begin (), blockPrologue.end (), std::back_inserter (blockStatements));
    std::move (blockCode.begin (), blockCode.end (), std::back_inserter (blockStatements));
    std::move (blockEpilogue.begin (), blockEpilogue.end (), std::back_inserter (blockStatements));
    
}

void T9900Generator::generateBlock (TBlock &block) {
//    std::cout << "Entering: " << block.getSymbol ()->getName () << std::endl;

    TSymbolList &blockSymbols = block.getSymbols ();
    TCodeSequence blockStatements;
    
    assignStackOffsets (block);
    clearRegsUsed ();
    codeBlock (block, true, blockStatements);
/*    
    if (blockSymbols.getLevel () > 1) {
        bool functionCalled = std::find_if (blockStatements.begin (), blockStatements.end (), [] (const T9900Operation &op) { return op.operation == T9900Op::bl; }) != blockStatements.end ();
        if (!functionCalled) {
            assignRegisters (blockSymbols);
            assignStackOffsets (block);
            bool stackFrameNeeded = std::find_if (blockSymbols.begin (), blockSymbols.end (), [] (const TSymbol *s) {
                return (s->checkSymbolFlag (TSymbol::Parameter) || s->checkSymbolFlag (TSymbol::Variable)) && s->getRegister () == TSymbol::InvalidRegister; }) != blockSymbols.end ();
            codeBlock (block, stackFrameNeeded || block.isDisplayNeeded (), blockStatements);
//            std::cout << "Leave function: " << block.getSymbol ()->getName () << std::endl;
//            std::cout << "  RCX/RDX/REP MOVSB used = " << isRegUsed (TX64Reg::rcx) << ' ' << isRegUsed (TX64Reg::rdx) << ' ' << moveUsed << std::endl;
        }
    }
*/    
    
//    trySingleReplacements (blockStatements);
    std::move (blockStatements.begin (), blockStatements.end (), std::back_inserter (program));
    
    for (TSymbol *s: blockSymbols) 
        if (s->checkSymbolFlag (TSymbol::Routine)) {
            if (s->checkSymbolFlag (TSymbol::External)) {
                // nothing to do yet
            } else 
                visit (s->getBlock ());
        }

}

} // namespace statpascal

