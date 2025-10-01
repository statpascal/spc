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
#include <algorithm>
#include <filesystem>

#include "tms9900gen.hpp"

/*

9900 calling conventions 

r0:	   scratch reg; used for shift/rotate count
r1 - r8:   used as calculator stack; saved by callee (r8 only used for MPY/DIV)
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

const std::uint16_t workspace = 0x8300;
                    
const std::string farCall = "__far_call_2",
                  farCallCode = "__far_call_1",
                  farRet = "__far_ret",
                  copySet = "__copy_set_const",
                  copyStr = "__copy_str_const";

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
    dblAbsMask = "__dbl_abs_mask";

}  // namespace

        
static std::array<TToken, 6> relOps = {TToken::Equal, TToken::GreaterThan, TToken::LessThan, TToken::GreaterEqual, TToken::LessEqual, TToken::NotEqual};

T9900Generator::T9900Generator (TRuntimeData &runtimeData, bool codeRangeCheck, bool createCompilerListing):
  inherited (runtimeData),
  currentLevel (0),
  intStackCount (0),
  dblConstCount (0) {
}

bool T9900Generator::isCalleeSavedReg (const T9900Reg reg) {
    return std::find (intStackReg.begin (), intStackReg.end (), reg) != intStackReg.end ();
}

bool T9900Generator::isCalleeSavedReg (const T9900Operand &op) {
    return op.usesReg () && isCalleeSavedReg (op.reg);
}

bool T9900Generator::isCalcStackReg (const T9900Reg reg) {
    return isCalleeSavedReg (reg);
}

bool T9900Generator::isCalcStackReg (const T9900Operand &op) {
    return op.usesReg () && isCalcStackReg (op.reg);
}

bool T9900Generator::isSameReg (const T9900Operand &op1, const T9900Operand &op2) {
    return op1.usesReg () && op2.usesReg () && op1.reg == op2.reg;
}

bool T9900Generator::isSameCalcStackReg (const T9900Operand &op1, const T9900Operand &op2) {
    return isCalcStackReg (op1) && isCalcStackReg (op2) && op1.reg == op2.reg;
}

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
        if (line->operation == T9900Op::def_label && line->operand1.label.substr (0, 3) == "__l" && usedLabels.find (line->operand1.label) == usedLabels.end ())
            line = code.erase (line);
        else
            ++line;
}

bool rangeOk (std::int64_t a, std::int64_t b) {
    return std::abs (a - b) < 220;
}

void T9900Generator::removeJmpLines (TCodeSequence &code, TCodeSequence::iterator it, std::size_t count) {
    while (count-- > 0) 
        it = code.erase (it);
}
    
void T9900Generator::mergeLabel (TCodeSequence &code, TCodeSequence::iterator line) {
    const std::string 
        s1 = line->operand1.label,
        s2 = std::next (line)->operand1.label;
    auto replace = [&] (T9900Operand &op) { if (op.isLabel () && op.label == s2) op.label = s1; };
    for (T9900Operation &op: code) {
        replace (op.operand1);
        replace (op.operand2);
    }
/*    
        if (op.operand1.isLabel () && op.operand1.label == s2)
            op.operand1.label = s1;
        if (op.operand2.isLabel () && op.operand2.label == s2)
            op.operand2.label = s1;
    }
*/    
    code.erase (std::next (line));
}

void T9900Generator::mergeMultipleLabels (TCodeSequence &code) {
    TCodeSequence::iterator line = code.begin ();
    while (line->operation != T9900Op::end) 
        if (line->operation == T9900Op::def_label && std::next (line)->operation == T9900Op::def_label)
            mergeLabel (code, line);
        else
            ++line;
}

void T9900Generator::optimizeJumps (TCodeSequence &code) {
    code.push_back (T9900Op::end);
    code.push_back (T9900Op::end);
    code.push_back (T9900Op::end);
    
    mergeMultipleLabels (code);
    bool modified;
    do {
        std::int64_t offset = 0;
        jmpLabels.clear ();
        for (const T9900Operation &op: code) {
            offset += op.getSize (offset);
            if (op.operation == T9900Op::def_label)
                jmpLabels [op.operand1.label] = offset;
        }
        
        offset = 0;
        modified = false;
        TCodeSequence::iterator line = code.begin ();
        while (!modified && line->operation != T9900Op::end) {
            using enum statpascal::T9900Op;
            offset += line->getSize (offset);
            if ((line->operation >= T9900Op::jmp && line->operation <= T9900Op::jop) || line->operation == T9900Op::b) {
                T9900Op op [4];
                std::string label [4];
                TCodeSequence::iterator it = line;
                for (int i = 0; i < 4; ++i) {
                    op [i] = it->operation;
                    if (it->operand1.isLabel ())
                        label [i] = it->operand1.label;
                    else if (op [i] != T9900Op::end)
                        op [i] = T9900Op::comment;	// do not touch
                    ++it;
                }
                
                // handle jump to jump
                if (!label [0].empty ()) 
                    for (TCodeSequence::iterator it1 = code.begin (); it1->operation != T9900Op::end; ++it1)
                        if (it1->operation == def_label && it1->operand1.label == label [0]) {
                            it1 = std::next (it1);
                            if ((it1->operation == b || it1->operation == jmp) && (op [0] == b || rangeOk (offset, jmpLabels [it1->operand1.label]))) {
                                label [0] = line->operand1.label = it1->operand1.label;
                                modified = true;
                                break;
                            }
                        }

                if ((op [0] == jmp || op [0] == b) && op [1] != def_label && op [1] != end) {
                    removeJmpLines (code, std::next (line), 1);
                    modified = true;
                } else if (op [0] == jmp && op [1] == def_label && label [0] == label [1]) {
                    removeJmpLines (code, line, 1);
                    modified = true;
                } else if ((op [0] == jeq || op [0] == jne) && (op [1] == b || op [1] == jmp) && op [2] == def_label && label [0] == label [2] && rangeOk (offset, jmpLabels [label [1]])) {
                    line->operation = op [0] == jeq ? jne : jeq;
                    line->operand1 = label [1];
                    removeJmpLines (code, std::next (line), 1);
                    modified = true;
                } else if (op [0] == jlt && op [1] == jeq && op [2] == b && op [3] == def_label &&
                    label [0] == label [3] && label [1] == label [3] && rangeOk (offset, jmpLabels [label [2]])) {
                    line->operation = jgt;
                    line->operand1 = label [2];
                    removeJmpLines (code, std::next (line), 3);
                    modified = true;
                } else if (op [0] == b && rangeOk (offset, jmpLabels [label [0]]) && label [0] != farRet) {
                    line->operation = jmp;
                    line->operand1 = label [0];
                    modified = true;
                } 
            }
            ++line;
        }
    } while (modified);
    
    while (!code.empty () && code.back ().operation == T9900Op::end)
        code.pop_back ();
}

void T9900Generator::optimizeSingleLine (TCodeSequence &code) {
    for (T9900Operation &line: code) {
        if (line.operation == T9900Op::li && !line.operand2.isLabel ()) {
            if (!line.operand2.val) {
                line.operation = T9900Op::clr;
                line.operand2 = T9900Operand ();
            } else if (line.operand2.val == 0xffff) {
                line.operation = T9900Op::seto;
                line.operand2 = T9900Operand ();
            }
        } else if (line.operation == T9900Op::ai && !line.operand2.isLabel () && static_cast<std::int16_t> (line.operand2.val) >= -2 && static_cast<std::int16_t> (line.operand2.val) <= 2 && line.operand2.val) {
            T9900Op ops [5] = {T9900Op::dect, T9900Op::dec, T9900Op::ai, T9900Op::inc, T9900Op::inct};
            line.operation = ops [static_cast<std::int16_t> (line.operand2.val) + 2];
            line.operand2 = T9900Operand ();
        }
        if (line.operand1.isValid () && line.operand1.isIndexed () && !line.operand1.isLabel () && !line.operand1.val)
            line.operand1.t = T9900Operand::TAddressingMode::RegInd;
        if (line.operand2.isValid () && line.operand2.isIndexed () && !line.operand2.isLabel () && !line.operand2.val)
            line.operand2.t = T9900Operand::TAddressingMode::RegInd;
    }
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
        
        T9900Operand &op_4_1 = line3->operand1,
                     &op_4_2 = line3->operand2;
        T9900Op      &op4 = line3->operation;
//        std::string &comm_4 = line3->comment; 
        
        if (op1 == T9900Op::li && op2 == T9900Op::li && op3 == T9900Op::mov && op4 == T9900Op::mov &&
            isSameCalcStackReg (op_1_1, op_4_1) && isSameCalcStackReg (op_2_1, op_3_1)) {
            op_2_1 = op_3_2;
            op_1_1 = op_4_2;
            removeLines (code, line2, 2);
        }
        
        // li reg, imm
        // mov|movb|a|s|c op, *reg
        // ->
        // mov|movb|a|s|c op, @imm
        
        else if (op1 == T9900Op::li && (op2 == T9900Op::mov || op2 == T9900Op::movb || op2 == T9900Op::a || op2 == T9900Op::s|| op2 == T9900Op::c) && isSameCalcStackReg (op_1_1, op_2_2) && op_2_2.t == T9900Operand::TAddressingMode::RegInd) {
            op_2_2 = T9900Operand (T9900Reg::r0, op_1_2.val);
            comm_2 = comm_2 + " " + comm_1;
            removeLines (code, line, 1);
        }
        
        // li reg, imm
        // swpb reg
        //->
        // li reg, swap (imm)
        else if (op1 == T9900Op::li && op2 == T9900Op::swpb && !op_1_2.isLabel () && isSameReg (op_1_1, op_2_1)) {
            op_1_2.val = ((op_1_2.val & 0xff) << 8) | (op_1_2.val >> 8);
            removeLines (code, line1, 1);
        }
        
        // li reg1, imm
        // add reg1, reg2
        // ->
        // ai reg2, imm
        else if (op1 == T9900Op::li && op2 == T9900Op::a && isSameCalcStackReg (op_1_1, op_2_1) && op_2_2.t == T9900Operand::TAddressingMode::Reg) {
            op2 = T9900Op::ai;
            op_2_1 = op_2_2;
            op_2_2 = op_1_2;
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
        
        // ai reg, 0
        // -> 
        // remove
        else if (op1 == T9900Op::ai && !op_1_2.isLabel () && !op_1_2.val)
            removeLines (code, line, 1);
            
        // ai reg, imm
        // mov *reg, op
        // ->
        // mov @imm(reg),op
        
        else if (op1 == T9900Op::ai && !op_1_2.isLabel () && op2 == T9900Op::mov && isSameCalcStackReg (op_1_1, op_2_1) && op_2_1.t == T9900Operand::TAddressingMode::RegInd) {
            op_2_1 = T9900Operand (op_1_1.reg, op_1_2.val);
            removeLines (code, line, 1);
        }
        
        // ai reg, imm
        // mov op, *reg
        // ->
        // mov op, @imm(reg)
        
        else if (op1 == T9900Op::ai && !op_1_2.isLabel () && op2 == T9900Op::mov && isSameCalcStackReg (op_1_1, op_2_2) && op_2_2.t == T9900Operand::TAddressingMode::RegInd) {
            op_2_2 = T9900Operand (op_1_1.reg, op_1_2.val);
            removeLines (code, line, 1);
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
            op_2_1.t = T9900Operand::TAddressingMode::Memory;
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
        else if (op1 == T9900Op::mov && (op2 == T9900Op::mov || op2 == T9900Op::a || op2 == T9900Op::s || op2 == T9900Op::c) && isSameCalcStackReg (op_1_2, op_2_1) && op_2_1.t == T9900Operand::TAddressingMode::Reg) {
            op_2_1 = op_1_1;
            comm_2 = comm_1 + " " + comm_2;
            removeLines (code, line, 1);
        }
        
        // mov op1, reg
        // c op2, reg
        // ->
        // a|s|c op2, op1
        else if (op1 == T9900Op::mov && op2 == T9900Op::c && isSameCalcStackReg (op_1_2, op_2_2) && op_2_2.t == T9900Operand::TAddressingMode::Reg) {
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
            isSameCalcStackReg (op_1_2, op_2_1) && op_3_1.isMemory () && op_3_1.val == 0x8301 + 2 * static_cast<int> (op_1_2.reg)) {
            op_1_2 = op_3_2;
            comm_1.append ( " " + comm_3);
            removeLines (code, line1, 2);
        }
        
        // li reg, >ffff
        // mov reg, op
        // ->
        // seto op
        else if (op1 == T9900Op::li && op2 == T9900Op::mov && !op_1_2.isLabel () && op_1_2.val == 0xffff && isSameCalcStackReg (op_1_1, op_2_1)) {
            op2 = T9900Op::seto;
            op_2_1 = op_2_2;
            op_2_2 = T9900Operand ();
            removeLines (code, line, 1);
        }
        
        // li reg, 0
        // mov reg, op
        // ->
        // clr @op
        else if (op1 == T9900Op::li && !op_1_2.val && !op_1_2.isLabel () && op2 == T9900Op::mov && isSameCalcStackReg (op_1_1, op_2_1)) {
            op2 = T9900Op::clr;
            op_2_1 = op_2_2;
            op_2_2 = T9900Operand ();
            removeLines (code, line, 1);
        }
        
        // li reg, imm1
        // mov reg, op
        // li reg, imm1
        // ->
        // remove redundant load
        else if (op1 == T9900Op::li && op2 == T9900Op::mov && op3 == T9900Op::li && op_1_2.isImm () && op_3_2.isImm () && op_1_2.getValue () == op_3_2.getValue () && !op_1_2.isLabel () && !op_3_2.isLabel () &&
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
        // jeq ll
        // sla/sra op, r0
        // ->
        // sla/sra op, imm
        else if (op1 == T9900Op::li && !op_1_2.isLabel () && op2 == T9900Op::mov && op3 == T9900Op::jeq && (op4 == T9900Op::sla || op4 == T9900Op::sra) && op_4_2.usesReg () &&
                isSameCalcStackReg (op_1_1, op_2_1) && op_2_1.t == T9900Operand::TAddressingMode::Reg && op_2_2.usesReg () && op_2_2.reg == T9900Reg::r0) {
            if (op_1_2.val) {
                op_4_2 = op_1_2;
                removeLines (code, line, 3);
            } else
                removeLines (code, line, 4);
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
        else if (op1 == T9900Op::li && !op_1_2.isLabel () && op2 == T9900Op::inv && op3 == T9900Op::szc && isSameCalcStackReg (op_1_1, op_2_1) &&
                 isSameCalcStackReg (op_2_1, op_3_1) && op_3_2.t == T9900Operand::TAddressingMode::Reg) {
            op3 = T9900Op::andi;
            op_3_1 = op_3_2;
            op_3_2 = op_1_2;
            removeLines (code, line, 2);
        }
        
        // li reg, imm1
        // sla reg, imm2
        // ->
        // li reg, imm1 << imm2
        else if (op1 == T9900Op::li && op2 == T9900Op::sla && isSameCalcStackReg (op_1_1, op_2_1) &&
                 !op_1_2.isLabel () && !op_2_2.isLabel () && op_2_2.val > 0) {
            op2 = op1;
            op_2_2.val = op_1_2.val << op_2_2.val;
            comm_2 = comm_1;
            removeLines (code, line, 1);
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
        else if (op1 == T9900Op::ai && op2 == T9900Op::ai && isSameReg (op_1_1, op_2_1) && !op_1_2.isLabel () && !op_2_2.isLabel ()) {
            op_2_2.val = (op_2_2.val + op_1_2.val) & 0xffff;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // li reg, imm1
        // ai reg, imm2
        // ->
        // li reg, imm1 + imm2
        else if (op1 == T9900Op::li && op2 == T9900Op::ai && isSameReg (op_1_1, op_2_1)) {
            op2 = op1;
            op_2_2.val += op_1_2.val;
            comm_2 = comm_1 + " " + comm_2;
            removeLines (code, line, 1);
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
            else if ((op3 == T9900Op::a || op3 == T9900Op::s || op3 == T9900Op::mov || op3 == T9900Op::movb) && op_1_1.usesReg () && isSameCalcStackReg (op_2_1, op_3_2) && op_3_2.t == T9900Operand::TAddressingMode::RegInd) {
                op_3_2 = T9900Operand (T9900Reg::r9, op_2_2.val);
                comm_3 = comm_2;
                removeLines (code, line, 2);
            } else
                ++line;
        }
        
        // mov reg, op1
        // op  op1, op2
        // ->
        // mov reg, op1
        // op  reg, op2
        else if (op1 == T9900Op::mov && op_1_1.isReg () && !op_1_2.isReg () && op_1_2 == op_2_1 && op_2_2.isValid ()) {
            op_2_1 = op_1_1;
            ++line;
        }
        
        // mov op, op
        // not cond jump
        // ->
        // not cond jump
        else if (op1 == T9900Op::mov && op_1_1 == op_1_2 && (op2 <= T9900Op::jmp || op2 >= T9900Op::sbo)) {
            removeLines (code, line, 1);
        }

/*        
        // li r12, bank
        // li r13, subprog
        // bl @farCall
        // ->
        // bl @farCallCode
        // data bank
        // data subprog
        else if (op1 == T9900Op::li && op2 == T9900Op::li && op3 == T9900Op::bl && op_3_1.label == farCall) {
            op1 = op3;
            op2 = op3 = T9900Op::data;
            op_1_1 = makeLabelMemory (farCallCode);
            op_2_1 = op_1_2;
            op_3_1 = op_2_2;
            op_1_2 = op_2_2 = T9900Operand ();
        }
*/        
        
         else
            ++line;
    }
    
    while (!code.empty () && code.back ().operation == T9900Op::end)
        code.pop_back ();
}

void T9900Generator::calcLength (TCodeBlock &proc) {
    proc.size = 0;
    for (T9900Operation &op: proc.codeSequence)
        proc.size += op.getSize (proc.size);
}

void T9900Generator::assignBank (TCodeBlock &proc, std::size_t &bank, std::size_t &org) {
    if (org + proc.size >= 0x7ff0) {
//    if (org + proc.size >= 0x6d00) {	// force earlier switch for tests
        ++bank;
        org = 0x6000 + sharedCode.size;
    }
    // TODO: check overflow of bank !!!!
    proc.bank = bank;
    proc.address = org;
    proc.codeSequence.push_front (T9900Operation (T9900Op::bank, org == 0x6000 ? -1 : bank, org));
    if (proc.symbol)
        bankMapping [getBankName (proc.symbol)] = bank;
    org += (proc.size + 1) & ~1;
}
    
void T9900Generator::resolveBankLabels (TCodeBlock &proc) {
    for (T9900Operation &op: proc.codeSequence) {
        if (op.operand1.label.substr (0, 6) == "$bank_")
            op.operand1 = T9900Operand (0x6000 + 2 * bankMapping [op.operand1.label]);
        if (op.operand2.label.substr (0, 6) == "$bank_")
            op.operand2 = T9900Operand (0x6000 + 2 * bankMapping [op.operand2.label]);
    }
}

std::string T9900Generator::getBankName (const std::string &s) {
    return "$bank_" + s;
}

std::string T9900Generator::getBankName (const TSymbol *s) {
    return getBankName (s->getName ());
}

void T9900Generator::getAssemblerCode (std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &listing) {
    opcodes.clear ();
    listing.clear ();

    calcLength (sharedCode);    
    calcLength (mainProgram);
    for (TCodeBlock &proc: subPrograms)
        calcLength (proc);
    
    std::size_t org = 0x6000;
    std::size_t bank = 0;
    assignBank (sharedCode, bank, org);
    
    assignBank (mainProgram, bank, org);
    for (TCodeBlock &proc: subPrograms)
        assignBank (proc, bank, org);
        
    // last word of each bank is filled with bank switching address (to be pushed in far calls)
    sharedCode.codeSequence.push_back (T9900Operation (T9900Op::comment, T9900Operand (), T9900Operand (),  std::string ("")));
    sharedCode.codeSequence.push_back (T9900Operation (T9900Op::comment, T9900Operand (), T9900Operand (),  std::string ("Bank ids at end of each page")));
    for (std::size_t i = 0; i <= bank; ++i) {
        sharedCode.codeSequence.push_back (T9900Operation (T9900Op::bank, i, 0x7ffe));
        sharedCode.codeSequence.push_back (T9900Operation (T9900Op::data, 0x6000 + 2 * i));
    }
    sharedCode.codeSequence.push_back (T9900Operation (T9900Op::comment, T9900Operand (), T9900Operand (),  std::string ("")));

    resolveBankLabels (mainProgram);
    for (TCodeBlock &proc: subPrograms)
        resolveBankLabels (proc);
        
    for (T9900Operation &op: sharedCode.codeSequence) 
        listing.push_back (op.makeString ());
    
    for (T9900Operation &op: mainProgram.codeSequence)
        listing.push_back (op.makeString ());
    for (TCodeBlock &codeBlock: subPrograms) {
        listing.push_back (std::string ());
        for (T9900Operation &op: codeBlock.codeSequence)
            listing.push_back (op.makeString ());
    }
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

void T9900Generator::outputComment (const std::string &s) {
    outputCode (T9900Op::comment, T9900Operand (), T9900Operand (), s);
}

void T9900Generator::outputLocalDefinitions () {
    if (!jumpTableDefinitions.empty ()) {
        outputComment ("jump tables for case statements");
        for (const TJumpTable &it: jumpTableDefinitions) {
            outputLabel (it.tableLabel);
            for (const std::string &s: it.jumpLabels)
                outputCode (T9900Op::data, T9900Operand (s.empty () ? it.defaultLabel : s));
        }
        jumpTableDefinitions.clear ();
    }
    if (!stringDefinitions.empty ()) {
        outputComment (std::string ());
        for (const TStringDefinition &s: stringDefinitions)
            outputCode (T9900Op::stri, T9900Operand (s.label), T9900Operand (s.val));
        stringDefinitions.clear ();
        outputCode (T9900Op::even);
    }
    if (!setDefinitions.empty ()) {
        outputComment (std::string ());
        outputComment ("; Set Constants");
        for (const TSetDefinition &s: setDefinitions) {
            outputLabel (s.label);
            std::string t;
            for (std::uint64_t v: s.val)
                for (int i = 0; i < 8; i += 2) {	// words to big endian
                    t.push_back (reinterpret_cast<const char *> (&v) [i + 1]);
                    t.push_back (reinterpret_cast<const char *> (&v) [i]);
                }
            outputCode (T9900Op::byte, t);
        }    
        setDefinitions.clear ();
    }
    if (!farCallVectors.empty ()) {
        outputComment (std::string ());
        outputComment ("; Far call vectors");
        for (const TFarCallVec &v: farCallVectors) {
            outputLabel (v.label);
            outputCode (T9900Op::data, v.bank);
            outputCode (T9900Op::data, v.proc);
        }
        farCallVectors.clear ();
    }
    
/*    
    if (!stringDefinitions.empty ()) {
        outputComment (std::string ());
        for (const TStringDefinition &s: stringDefinitions)
            outputCode (T9900Op::stri, T9900Operand (s.label), T9900Operand (s.val));
        stringDefinitions.clear ();
        outputCode (T9900Op::even);
    }
*/    
}

void T9900Generator::outputStaticConstants () {
    if (!staticDataDefinition.label.empty ()) {
        outputComment (std::string ());
        outputComment ("Static variable init data");
        outputLabel (staticDataDefinition.label);
        
        std::string val;
        std::size_t i = 0;
        std::vector<std::pair<std::uint16_t, std::string>>::iterator it = staticDataDefinition.staticRoutinePtrs.begin ();
        while (i < staticDataDefinition.values.size ()) {
            if (it != staticDataDefinition.staticRoutinePtrs.end () && i == it->first) {
                if (!val.empty ()) {
                    outputCode (T9900Op::byte, val);
                    val.clear ();
                }
                outputCode (T9900Op::data, getBankName (it->second));
                outputCode (T9900Op::data, it->second);
                ++it;
                i += 4;
            } else
                val.push_back (staticDataDefinition.values [i++]);
            if (val.size () == 16) {
                outputCode (T9900Op::byte, val);
                val.clear ();
            }
        }
        if (val.size ())
            outputCode (T9900Op::byte, val);
        outputCode (T9900Op::even);
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

//void T9900Generator::outputLabelDefinition (const std::string &label, const std::size_t value) {
//    labelDefinitions [label] = value;
//}

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

void T9900Generator::outputBooleanCheck (TExpressionBase *expr, std::string labelTrue, std::string labelFalse) {
    static const std::map<TToken, std::vector<T9900Op>> intTrueJmp = {
        {TToken::Equal, {T9900Op::jne}}, 
        {TToken::GreaterThan, {T9900Op::jlt, T9900Op::jeq}},
        {TToken::LessThan, {T9900Op::jgt, T9900Op::jeq}}, 
        {TToken::GreaterEqual, {T9900Op::jlt}}, 
        {TToken::LessEqual, {T9900Op::jgt}},
        {TToken::NotEqual, {T9900Op::jeq}}
    };
    
    if (TPrefixedExpression *prefExpr = dynamic_cast<TPrefixedExpression *> (expr))
        if (prefExpr->getOperation () == TToken::Not) {
            std::swap (labelTrue, labelFalse);
            expr = prefExpr->getExpression ();
        }
    
    if (expr->isConstant ()) {
        outputCode (T9900Op::b, makeLabelMemory (static_cast<TConstantValue *> (expr)->getConstant ()->getInteger () ? labelTrue : labelFalse));
        return;
    }
    
    if (TSimpleExpression *simple = dynamic_cast<TSimpleExpression *> (expr)) {
        if (simple->getLeftExpression ()->getType () == &stdType.Boolean &&
            simple->getRightExpression ()->getType () == &stdType.Boolean &&
            simple->getOperation () == TToken::Or) {
                std::string ll = getNextLocalLabel ();
                outputBooleanCheck (simple->getLeftExpression (), labelTrue, ll);
                outputLabel (ll);
                outputBooleanCheck (simple->getRightExpression (), labelTrue, labelFalse);
                return;
            }
    }
    
    if (TTerm *term = dynamic_cast<TTerm *> (expr)) {
        if (term->getLeftExpression ()->getType () == &stdType.Boolean &&
            term->getRightExpression ()->getType () == &stdType.Boolean &&
            term->getOperation () == TToken::And) {
                std::string ll = getNextLocalLabel ();
                outputBooleanCheck (term->getLeftExpression (), ll, labelFalse);
                outputLabel (ll);
                outputBooleanCheck (term->getRightExpression (), labelTrue, labelFalse);
                return;
        }
    }
    
    TExpression *condition = dynamic_cast<TExpression *> (expr);
    
    const std::string ll = getNextLocalLabel ();
    if (condition && (condition->getLeftExpression ()->getType ()->isEnumerated () || condition->getLeftExpression ()->getType ()->isPointer ())) {
        visit (condition->getLeftExpression ());
        TExpressionBase *right = condition->getRightExpression ();
        if (right->isConstant ())
            outputCode (T9900Op::ci, fetchReg (intScratchReg1), static_cast<TConstantValue *> (right)->getConstant ()->getInteger ());
        else {
            visit (condition->getRightExpression ());
            const T9900Reg r2 = fetchReg (intScratchReg1);
            const T9900Reg r1 = fetchReg (intScratchReg2);
            outputCode (T9900Operation (T9900Op::c, r1, r2));
        }
        for (T9900Op op: intTrueJmp.at (condition->getOperation ()))
            outputCode (op, ll);
    } else if (condition && condition->getLeftExpression ()->getType ()->isReal ()) {
        //
    } else {
        visit (expr);
        const T9900Reg reg = fetchReg (intScratchReg1);
        outputCode (T9900Op::ci, reg, 0);
        outputCode (T9900Op::jeq, ll);
    }
    outputCode (T9900Op::b, makeLabelMemory (labelTrue));
    outputLabel (ll);
    outputCode (T9900Op::b, makeLabelMemory (labelFalse));
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
    T9900Reg r1, r2, r3;
    
    if (right->isConstant () && !static_cast<TConstantValue *> (right)->getConstant ()->getInteger () && (operation == TToken::LessThan || operation == TToken::GreaterThan)) {
        visit (left);
        T9900Reg reg = fetchReg (intScratchReg2);
        if (operation == TToken::GreaterThan)
            outputCode (T9900Op::neg, reg);
        outputCode (T9900Op::srl, reg, 15);
        saveReg (reg);
        return;
    }
    
    
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
    const std::string ll = getNextLocalLabel ();    
    for (T9900Op op: cmpOperation.at (operation))
        outputCode (op, ll);
    outputCode (T9900Op::inc, r3);
    outputLabel (ll);
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
            const std::string ll = getNextLocalLabel ();
            outputCode (T9900Op::jeq, ll);
            T9900Reg r = fetchReg (intScratchReg2);
            outputCode (operation == TToken::Shl ? T9900Op::sla : T9900Op::sra, r, T9900Reg::r0);
            outputLabel (ll);
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

    TExpressionBase *function = functionCall.getFunction ();
    const std::vector<TExpressionBase *> &args = functionCall.getArguments ();
    const std::string s = static_cast<TRoutineValue *> (function)->getSymbol ()->getExtSymbolName ();
    
    if (s == "__abs") {
        visit (args [0]);
        const T9900Reg reg = fetchReg (intScratchReg1);
        outputCode (T9900Op::abs, reg);
        saveReg (reg);
    }
    if (s == "__min" || s == "__max") {
        visit (args [0]);
        visit (args [1]);
        const T9900Reg right = fetchReg (intScratchReg2),
                       left = fetchReg (intScratchReg3);
        outputComment ("no.opt");
        outputCode (T9900Op::c, left, right);
        const std::string ll = getNextLocalLabel ();
        outputCode (s == "__min" ? T9900Op::jlt : T9900Op::jgt, ll);
        outputCode (T9900Op::mov, right, left);
        saveReg (left);
        outputLabel (ll);
    }
    if (s == "__sqr") {
        visit (args [0]);
        const T9900Reg reg = fetchReg (intScratchReg2);
        outputCode (T9900Op::mpy, reg, reg);
        outputCode (T9900Op::mov, static_cast<T9900Reg> (static_cast<unsigned> (reg) + 1), reg);
        saveReg (reg);
    }
    if (s == "__in_set") {
        visit (args [0]);
        T9900Reg val = fetchReg (intScratchReg3);
        outputCode (T9900Op::li, intScratchReg4, 1);
        outputCode (T9900Op::mov, val, intScratchReg1);
        outputCode (T9900Op::andi, intScratchReg1, 15);
        std::string ll = getNextLocalLabel ();
        outputCode (T9900Op::jeq, ll);
        outputCode (T9900Op::sla, intScratchReg4, 0);
        outputLabel (ll);
        outputCode (T9900Op::sra, val, 3);
        saveReg (val);
        
        visit (args [1]);
        const T9900Reg set = fetchReg (intScratchReg2);
        val = fetchReg (intScratchReg3);

        outputCode (T9900Op::a, val, set);
        outputCode (T9900Op::mov, T9900Operand (set, T9900Operand::TAddressingMode::RegInd), set);
        outputCode (T9900Op::clr, val);
        outputCode (T9900Op::coc, intScratchReg4, set);
        ll = getNextLocalLabel ();
        outputCode (T9900Op::jne, ll);
        outputCode (T9900Op::inc, val);
        saveReg (val);
        outputLabel (ll);
        
    }  
}

bool T9900Generator::isFunctionCallInlined (TFunctionCall &functionCall) {
    TExpressionBase *function = functionCall.getFunction ();
    if (function->isRoutine ()) {
        TSymbol *s = static_cast<TRoutineValue *> (function)->getSymbol ();
        return s->checkSymbolFlag (TSymbol::Intrinsic);
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


//    TRoutineType *type = static_cast<TRoutineType *> (functionCall.getFunction ()->getType ());
//    std::cout << "CALL: " <<  type->getName () << (type->isFarCall () ? ": FAR" : ": NEAR") << std::endl;


    TExpressionBase *function = functionCall.getFunction ();
    const bool isFarCall = static_cast<TRoutineType *> (function->getType ())->isFarCall ();
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
    if (isFarCall)
        offCount += 2;
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
    if (isFarCall) 
        outputCode (T9900Op::mov, T9900Operand (0x7ffe, T9900Operand::TAddressingMode::Memory), T9900Operand (T9900Reg::r10, offCount - 2));

    for (ssize_t i = parameterDescriptions.size () - 1; i >= 0; --i) {
        visit (parameterDescriptions [i].actualParameter);
        if (parameterDescriptions [i].isInteger) {
            const T9900Reg reg = fetchReg (intScratchReg1);
            bool isByte = parameterDescriptions [i].type->getSize () == 1;
            if (isByte)
                outputCode (T9900Op::swpb, reg);            
            outputCode (isByte ? T9900Op::movb : T9900Op::mov, reg, T9900Operand (T9900Reg::r10, parameterDescriptions [i].offset));
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
    }
    
    if (TRoutineValue *routine = dynamic_cast<TRoutineValue *> (functionCall.getFunction ())) {
        const std::string name = routine->getSymbol ()->getName ();
        if (isFarCall) {
            outputCode (T9900Op::bl, makeLabelMemory (farCallCode));
            outputCode (T9900Op::data, getBankName (name));
            outputCode (T9900Op::data, name);
        } else
            outputCode (T9900Op::bl, makeLabelMemory (name));
    } else {
        visit (function);
        const T9900Reg reg = fetchReg (intScratchReg1);
        if (isFarCall) {
            outputCode (T9900Op::mov, T9900Operand (reg, T9900Operand::TAddressingMode::RegIndInc), intScratchReg2);
            outputCode (T9900Op::mov, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd), intScratchReg3);
            outputCode (T9900Op::bl, makeLabelMemory (farCall));
        } else 
            outputCode (T9900Op::bl, T9900Operand (reg, T9900Operand::TAddressingMode::RegInd));
    }

    if (functionCall.getType () != &stdType.Void && !functionCall.isIgnoreReturn ())
        visit (functionCall.getReturnStorageDeref ());
}

void T9900Generator::generateCode (TConstantValue &constant) {
    if (constant.getType () == &stdType.Real) {
        //
    } else if (constant.getConstant ()->getType () == &stdType.String || constant.getType ()->isSet ()) {
        T9900Reg reg = getSaveReg (intScratchReg1);
        const std::string label = constant.getType ()->isSet () ?
            registerConstant (constant.getConstant ()->getSetValues ()) :
            registerConstant (constant.getConstant ()->getString ());
        outputCode (T9900Op::li, reg, label);
        saveReg (reg);
    } else {
        T9900Reg reg = getSaveReg (intScratchReg1);
        const std::int16_t n = constant.getConstant ()->getInteger ();
        outputCode (T9900Op::li, reg, n);
        saveReg (reg);
    }
}

void T9900Generator::generateCode (TRoutineValue &routineValue) {
    bool isFarCall = static_cast<TRoutineType *> (routineValue.getType ())->isFarCall ();
    TSymbol *s = routineValue.getSymbol ();
    const std::string name = s->getName ();
    if (isFarCall) {
        std::string label;
        for (TFarCallVec &vec: farCallVectors)
            if (vec.proc == name)
                label = vec.label;
        if (label.empty ()) {
            label = getNextLocalLabel ();
            farCallVectors.emplace_back (TFarCallVec {label, getBankName (name), name});
        }
        T9900Reg reg = getSaveReg (intScratchReg2);
        outputCode (T9900Op::li, reg, label);
        saveReg (reg);
    } else {
        T9900Reg reg = getSaveReg (intScratchReg3);
        outputCode (T9900Op::li, reg, name);
        saveReg (reg);
    }
}

void T9900Generator::codeSymbol (const TSymbol *s, const T9900Reg reg) {
    if (s->getLevel () == 1)	// global
        outputCode (T9900Op::li, reg, s->getOffset (), s->getName ());
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
        outputCode (T9900Op::movb, T9900Operand (T9900Reg::r0, workspace + 2 * static_cast<int> (srcReg) + 1), destMem, "low R" + std::to_string (static_cast<std::size_t> (srcReg)));
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
        outputCode (T9900Op::li, intScratchReg2, n);
        outputCode (T9900Op::mpy, reg, intScratchReg2);
        outputCode (T9900Op::mov, intScratchReg3, reg);
    }
}

void T9900Generator::inlineMove (T9900Reg src, T9900Reg dst, T9900Reg count) {            
    const std::string ll = getNextLocalLabel ();
    outputLabel (ll);
    outputCode (T9900Op::movb, T9900Operand (src, T9900Operand::TAddressingMode::RegIndInc), T9900Operand (dst, T9900Operand::TAddressingMode::RegIndInc));
    outputCode (T9900Op::dec, count);
    outputCode (T9900Op::jne, ll);
}

void T9900Generator::codeMove (const TType *type) {
    const std::size_t n = type->getSize ();
    if (type->isShortString ()) {
        T9900Reg count = getSaveReg (intScratchReg2);
        T9900Reg dst = fetchReg (intScratchReg3);
        T9900Reg src = fetchReg (intScratchReg4);
        outputCode (T9900Op::li, intScratchReg1, 256 * (n - 1));
        outputCode (T9900Op::movb, T9900Operand (src, T9900Operand::TAddressingMode::RegIndInc), count);
        outputCode (T9900Op::cb, count, intScratchReg1);
        const std::string ll1 = getNextLocalLabel ();
        outputCode (T9900Op::jl, ll1);
        outputCode (T9900Op::mov, intScratchReg1, count);
        outputLabel (ll1);
        outputCode (T9900Op::movb, count, T9900Operand (dst, T9900Operand::TAddressingMode::RegIndInc));
        const std::string ll2 = getNextLocalLabel ();
        outputCode (T9900Op::jeq, ll2);
        outputCode (T9900Op::srl, count, 8);
        inlineMove (src, dst, count);
        outputLabel (ll2);
    } else {
        T9900Reg count = getSaveReg (intScratchReg2);
        outputCode (T9900Op::li, count, n);
        T9900Reg dst = fetchReg (intScratchReg3);
        T9900Reg src = fetchReg (intScratchReg4);
        inlineMove (src, dst, count);
    }
}

void T9900Generator::codePush (const T9900Operand op) {
    outputCode (T9900Op::ai, T9900Reg::r10, -2);
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
                outputCode (T9900Op::b, makeLabelMemory (endOfRoutineLabel));
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
        T9900Operand operand = T9900Operand (fetchReg (intScratchReg2), T9900Operand::TAddressingMode::RegInd);
        T9900Reg reg = fetchReg (intScratchReg1);
        if (st == &stdType.Real || st == &stdType.Single) {
            //
        } else
            codeStoreMemory (st, operand, reg);
    } else
        codeMove (type);
}

void T9900Generator::generateCode (TRoutineCall &routineCall) {
    visit (routineCall.getRoutineCall ());
}

void T9900Generator::generateCode (TIfStatement &ifStatement) {
/*
    const std::string l1 = getNextLocalLabel ();
    outputBooleanCheck (ifStatement.getCondition (), l1);
    visit (ifStatement.getStatement1 ());
    if (TStatement *statement2 = ifStatement.getStatement2 ()) {
        const std::string l2 = getNextLocalLabel ();
        outputCode (T9900Op::b, makeLabelMemory (l2));
        outputLabel (l1);
        visit (statement2);
        outputLabel (l2);
    } else
        outputLabel (l1);
*/        
    const std::string 
        l1 = getNextLocalLabel (),
        l2 = getNextLocalLabel (),
        l3 = getNextLocalLabel ();
    outputBooleanCheck (ifStatement.getCondition (), l1, l2);
    outputLabel (l1);
    TStatement *stmt1 = ifStatement.getStatement1 ();
    visit (stmt1);
    if (!dynamic_cast<TGotoStatement *> (stmt1))
        outputCode (T9900Op::b, makeLabelMemory (l3));
    outputLabel (l2);
    if (TStatement *statement2 = ifStatement.getStatement2 ())
        visit (statement2);
    outputLabel (l3);
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
            const std::string ll = getNextLocalLabel ();
            if (e.label.a == e.label.b) {
                outputCode (T9900Op::jne, ll);
                outputCode (T9900Op::b, makeLabelMemory (e.jumpLabel->getName ()));
            } else {
                outputCode (T9900Op::ci, reg, e.label.b - last);
                outputCode (T9900Op::jh, ll);
                outputCode (T9900Op::b, makeLabelMemory (e.jumpLabel->getName ()));
            }
            outputLabel (ll);
        }
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            visit (defaultStatement);
        }
        outputCode (T9900Op::b, makeLabelMemory (endLabel));

    } else {
        const std::string evalTableLabel = getNextCaseLabel ();
        std::string defaultLabel = endLabel;
        if (minLabel) 
            outputCode (T9900Op::ai, reg, -minLabel);
        outputComment ("no-opt");
        outputCode (T9900Op::ci, reg, maxLabel - minLabel);
        
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            defaultLabel = getNextCaseLabel ();
            outputCode (T9900Op::jh, defaultLabel);
            outputCode (T9900Op::b, makeLabelMemory (evalTableLabel));
            outputLabel (defaultLabel);
            visit (defaultStatement);
            outputCode (T9900Op::b, makeLabelMemory (endLabel));
        }  else {
            const std::string ll = getNextLocalLabel ();
            outputCode (T9900Op::jle, ll);
            outputCode (T9900Op::b, makeLabelMemory (endLabel));
            outputLabel (ll);
        }
        
        std::vector<std::string> jumpTable (maxLabel - minLabel + 1);
        for (const TCaseStatement::TCase &c: caseList) {
            for (const TCaseStatement::TLabel &label: c.labels)
                for (std::int64_t n = label.a; n <= label.b; ++n)
                    jumpTable [n - minLabel] = c.jumpLabel->getName ();
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
        outputLabel (c.jumpLabel->getName ());
        visit (c.statement);
        outputCode (T9900Op::b, makeLabelMemory (endLabel));
    }    
    outputLabel (endLabel);
}

void T9900Generator::generateCode (TStatementSequence &statementSequence) {
    for (TStatement *statement: statementSequence.getStatements ())
        visit (statement);
}

void T9900Generator::generateCode (TLabeledStatement &labeledStatement) {
    outputLabel (labeledStatement.getLabel ()->getName ());
    visit (labeledStatement.getStatement ());
}

void T9900Generator::generateCode (TGotoStatement &gotoStatement) {
    if (TExpressionBase *condition = gotoStatement.getCondition ()) {
        const std::string ll = getNextLocalLabel ();
        outputBooleanCheck (condition, gotoStatement.getLabel ()->getName (), ll);
        outputLabel (ll);
    } else
        outputCode (T9900Op::b, makeLabelMemory (gotoStatement.getLabel ()->getName ()));
}
    
void T9900Generator::generateCode (TBlock &block) {
    generateBlock (block);
}

void T9900Generator::generateCode (TUnit &unit) {
}

void T9900Generator::generateCode (TProgram &program) {
    T9900Operand e;	// empty

    setOutput (&sharedCode.codeSequence);    
    const std::string proglist = getNextLocalLabel ();
    outputCode (T9900Op::aorg, 0x6000, e, "cartride address space");
    outputComment (std::string ());
    outputCode (T9900Op::data, 0xaa01, e, "standard header");
    outputCode (T9900Op::data, 0x0100, e, "number of programs");
    outputCode (T9900Op::data, 0x0000, e, "power up list");
    outputCode (T9900Op::data, proglist, e, "program list");	
    outputCode (T9900Op::data, 0x0000, e, "DSR list");
    outputCode (T9900Op::data, 0x0000, e, "subprogram list");
    outputCode (T9900Op::data, 0x0000, e, "ISR list");
    
    outputLabel (proglist);
    outputCode (T9900Op::data, 0x0000, e, "no next program");
    const std::string progstart = getNextLocalLabel ();
    outputCode (T9900Op::data, progstart, e, "program address");
    
    std::string progname = program.getBlock ()->getSymbol ()->getName ();
    std::transform (progname.begin (), progname.end (), progname.begin (), [] (char c) {return c == '_' ? ' ' : std::toupper (c);});
    outputCode (T9900Op::byte, progname.length ());
    outputCode (T9900Op::text, progname);
    outputCode (T9900Op::even);
    
    outputComment (std::string ());
    outputLabel (farCallCode);
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegIndInc), intScratchReg2);
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegIndInc), intScratchReg3);
    
    outputComment (std::string ());
    outputLabel (farCall);
    outputCode (T9900Op::clr, T9900Operand (intScratchReg2, T9900Operand::TAddressingMode::RegInd), T9900Operand (), "switch bank");
    outputCode (T9900Op::b, T9900Operand (intScratchReg3, T9900Operand::TAddressingMode::RegInd));
    
    outputComment (std::string ());
    outputLabel (farRet);
    codePop (intScratchReg2);
    outputCode (T9900Op::clr, T9900Operand (intScratchReg2, T9900Operand::TAddressingMode::RegInd), T9900Operand (), "switch bank");
    outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
    
    outputComment (std::string ());
    outputLabel (copySet);
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r10, T9900Operand::TAddressingMode::RegInd), intScratchReg2);
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r10, 2), intScratchReg3);
    outputCode (T9900Op::li, intScratchReg4, 16);
    std::string ll = getNextLocalLabel ();
    outputLabel (ll);
    outputCode (T9900Op::mov, T9900Operand (intScratchReg3, T9900Operand::TAddressingMode::RegIndInc), T9900Operand (intScratchReg2, T9900Operand::TAddressingMode::RegIndInc));
    outputCode (T9900Op::dec, intScratchReg4);
    outputCode (T9900Op::jne, ll);
    outputCode (T9900Op::ai, T9900Reg::r10, 4);
    outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
    
    outputComment (std::string ());
    outputLabel (copyStr);
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r10, T9900Operand::TAddressingMode::RegInd), intScratchReg2);
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r10, 2), intScratchReg3);
    outputCode (T9900Op::movb, T9900Operand (intScratchReg3, T9900Operand::TAddressingMode::RegInd), intScratchReg4);
    outputCode (T9900Op::sra, intScratchReg4, 8);
    ll = getNextLocalLabel ();
    outputLabel (ll);
    outputCode (T9900Op::movb, T9900Operand (intScratchReg3, T9900Operand::TAddressingMode::RegIndInc), T9900Operand (intScratchReg2, T9900Operand::TAddressingMode::RegIndInc));
    outputCode (T9900Op::dec, intScratchReg4);
    outputCode (T9900Op::joc, ll);
    outputCode (T9900Op::ai, T9900Reg::r10, 4);
    outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
    
    outputComment (std::string ());
    outputLabel (progstart);
    outputCode (T9900Op::lwpi, workspace);
    outputCode (T9900Op::limi, 0);
    outputCode (T9900Op::clr, T9900Operand (0x6000, T9900Operand::TAddressingMode::Memory), T9900Operand (), "activate bank 0");
    outputCode (T9900Op::b, makeLabelMemory ("__main_start"));
    
    setOutput (&mainProgram.codeSequence);
    outputLabel ("__main_start");
    generateBlock (*program.getBlock ());

    setOutput (&mainProgram.codeSequence);
    outputStaticConstants ();
    
}

void T9900Generator::initStaticRoutinePtr (std::size_t addr, const TRoutineValue *routineValue) {
    const std::string name = routineValue->getSymbol ()->getName ();
    const std::uint16_t pos = addr - reinterpret_cast<std::size_t> (&staticData [0]);
    staticDataDefinition.staticRoutinePtrs.push_back (std::make_pair (pos, name));
}

void T9900Generator::resolvePascalSymbol (T9900Operand &operand, TSymbolList &symbols) {
    if (operand.isValid () && operand.isLabel ()) 
        if (TSymbol *s = symbols.searchSymbol (operand.label))
            if (s->checkSymbolFlag (TSymbol::Variable) || s->checkSymbolFlag (TSymbol::Parameter) || s->checkSymbolFlag (TSymbol::Absolute)) {
                operand.label.clear ();
                if (s->getLevel () == 1)
                    operand.val = s->getOffset ();
                else {
                    operand.val = s->getOffset () - 4;
                    operand.reg = T9900Reg::r10;
                    if (!operand.val)
                        operand.t = T9900Operand::TAddressingMode::RegInd;
                    else
                        operand.t = T9900Operand::TAddressingMode::Indexed;
                }
            }
            
}
    
void T9900Generator::externalRoutine (TSymbol *s) {
//    std::cout << "External: " << s->getName () << ", level: " << s->getLevel () << ", libname: " << s->getExtLibName () << std::endl;
    std::map<TSymbol *, TCodeSequence>::iterator it = assemblerBlocks.find (s);
    if (it != assemblerBlocks.end () || !s->getExtLibName ().empty ()) {
        TBlock &block = *s->getBlock ();
        TSymbolList &symbols = block.getSymbols ();
        assignParameterOffsets (block);
        outputComment (std::string ());
        TRoutineType *type = static_cast<TRoutineType *> (s->getType ());
        bool isFar = type->isFarCall ();
//        std::cout << s->getName () << ": " <<  type->getName () << (type->isFarCall () ? ": FAR" : ": NEAR") << std::endl;
        for (const std::string &s: createSymbolList (s->getName (), symbols.getLevel (), symbols, {}, -4))
            outputComment (s);
        outputComment (std::string ());
        outputLabel (s->getName ());
        
        if (it != assemblerBlocks.end ()) {
            for (T9900Operation &op: it->second) {
                resolvePascalSymbol (op.operand1, symbols);
                resolvePascalSymbol (op.operand2, symbols);
            }
            std::move (it->second.begin (), it->second.end (), std::back_inserter (*currentOutput));
            
            if (std::size_t size = s->getBlock ()->getSymbols ().getParameterSize ())
                outputCode (T9900Op::ai, T9900Reg::r10, size);
            // TODO: check far flag for level 1 routines
            if (isFar)
                outputCode (T9900Op::b, makeLabelMemory (farRet));
            else
                outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
        } else {
            std::string binData;
            binData.reserve (std::filesystem::file_size (s->getExtLibName ()));	
            std::ifstream f (s->getExtLibName (), std::ios::binary);
            binData.assign (std::istreambuf_iterator<char> (f), std::istreambuf_iterator<char> ());
            for (std::size_t i = 0; i < binData.length (); i += 32)
                outputCode (T9900Op::byte, binData.substr (i, 32));
            outputCode (T9900Op::even);
        }
    }
    
}
    
void T9900Generator::beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &symbolList, const std::set<T9900Reg> &saveRegs, bool hasStackFrame) {
    outputComment (std::string ());
    for (const std::string &s: createSymbolList (routineName, level, symbolList, {}))
        outputComment (s);
    outputComment (std::string ());
    
    if (!routineName.empty ())
        outputLabel (routineName);

    if (level > 1) {    
        if (hasStackFrame) {
            int stackCount = 2 * level + symbolList.getLocalSize () + 2 * saveRegs.size ();
            outputCode (T9900Op::ai, T9900Reg::r10, -stackCount);
            outputCode (T9900Op::mov, T9900Reg::r10, T9900Reg::r12);
            const T9900Operand op (T9900Operand (T9900Reg::r12, T9900Operand::TAddressingMode::RegIndInc));
            for (std::set<T9900Reg>::reverse_iterator it = saveRegs.rbegin (); it != saveRegs.rend (); ++it)
                outputCode (T9900Op::mov, *it, op);
            outputCode (T9900Op::ai, T9900Reg::r12, symbolList.getLocalSize ());
            for (int i = level - 3; i >= 0; --i)
                outputCode (T9900Op::mov, i ? T9900Operand (T9900Reg::r9, -2 * i) : T9900Operand (T9900Reg::r9, T9900Operand::TAddressingMode::RegInd), op);
            outputCode (T9900Op::mov, T9900Reg::r9, T9900Operand (T9900Reg::r12, T9900Operand::TAddressingMode::RegInd));
            outputCode (T9900Op::mov, T9900Reg::r12, T9900Reg::r9);
            outputCode (T9900Op::mov, T9900Reg::r11, T9900Operand ( T9900Reg::r12, 2));
        }
    } else
        outputCode (T9900Op::li, T9900Reg::r10, 65536 - symbolList.getLocalSize (), "init stack ptr");
}

void T9900Generator::endRoutineBody (std::size_t level, TSymbolList &symbolList, const std::set<T9900Reg> &saveRegs, bool hasStackFrame, bool isFar) {
    if (level > 1) {
        for (std::set<T9900Reg>::reverse_iterator it = saveRegs.rbegin (); it != saveRegs.rend (); ++it)
            if (std::next (it) != saveRegs.rend ())
                codePop (*it);
            else
                outputCode (T9900Op::mov, T9900Operand (T9900Reg::r10, T9900Operand::TAddressingMode::RegInd), *it);

        if (hasStackFrame) {
            outputCode (T9900Op::mov, T9900Reg::r9, T9900Reg::r10);
            codePop (T9900Reg::r9);
            codePop (T9900Reg::r11);
        }
        if (symbolList.getParameterSize ())
            outputCode (T9900Op::ai, T9900Reg::r10, symbolList.getParameterSize ());
        
        if (isFar)
            outputCode (T9900Op::b, makeLabelMemory (farRet));
        else
            outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
    } else
        outputCode (T9900Op::blwp, T9900Operand (T9900Reg::r0, 0));
}

TCodeGenerator::TParameterLocation T9900Generator::classifyType (const TType *type) {
    if (type->isEnumerated () || type->isPointer () || type->isReference ()) 
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

//    TSymbol *s = block.getSymbol ();
//    TRoutineType *type = static_cast<TRoutineType *> (s->getType ());
//    std::cout << s->getName () << ": " <<  type->getName () << (type->isFarCall () ? ": FAR" : ": NEAR") << std::endl;

    TSymbolList &symbolList = block.getSymbols ();
    
    std::size_t valueParaOffset = 4;;
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

void T9900Generator::codeBlock (TBlock &block, bool hasStackFrame, bool isFar, TCodeSequence &output) {
    TCodeSequence blockPrologue, blockEpilogue, blockCode, globalInits;

    TCodeSequence blockStatements;

    TSymbolList &blockSymbols = block.getSymbols ();
    intStackCount = 0;
    currentLevel =  blockSymbols.getLevel ();
    endOfRoutineLabel = getNextLocalLabel ();

    if (blockSymbols.getLevel () == 1) {
        setOutput (&globalInits);
        assignGlobalVariables (blockSymbols);
        std::size_t firstStatic = 0;
        bool staticFound = false;
        for (TSymbol *s: blockSymbols)
            if (!staticFound && s->checkSymbolFlag (TSymbol::StaticVariable)) {
                firstStatic = s->getOffset ();
                staticFound = true;
            }
        if (staticFound) {
            std::size_t staticSize = 65536 - firstStatic;
            outputComment (std::string ());
            outputComment ("Init static globals: " + std::to_string (staticSize) + " bytes at address " + std::to_string (firstStatic));
            staticData.resize (staticSize);
            for (TSymbol *s: blockSymbols)
                if (s->checkSymbolFlag (TSymbol::StaticVariable))
                    initStaticVariable (&staticData [s->getOffset () - firstStatic], s->getType (), s->getConstant ());
            staticDataDefinition.label = getNextLocalLabel ();
            staticDataDefinition.values = std::move (staticData);
            outputLabel ("$init_static");
            outputCode (T9900Op::li, T9900Reg::r12, staticDataDefinition.label);
            outputCode (T9900Op::li, T9900Reg::r13, firstStatic);
            outputCode (T9900Op::li, T9900Reg::r14, staticSize);
            inlineMove (T9900Reg::r12, T9900Reg::r13, T9900Reg::r14);
            outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, T9900Operand::TAddressingMode::RegInd));
        }
    }
    
    setOutput (&blockCode);
    if (!globalInits.empty ()) 
        outputCode (T9900Op::bl, makeLabelMemory ("$init_static"));
    visit (block.getStatements ());
    outputLabel (endOfRoutineLabel);
    
    optimizePeepHole (blockCode);
    optimizeSingleLine (blockCode);
    optimizeJumps (blockCode);
    removeUnusedLocalLabels (blockCode);
    optimizeJumps (blockCode);
//    tryLeaveFunctionOptimization (blockCode);
    
    // check which callee saved registers are used after peep hole optimization
    std::set<T9900Reg> saveRegs;
    for (T9900Operation &op: blockCode) {
        if (isCalleeSavedReg (op.operand1))
            saveRegs.insert (op.operand1.reg);
        if (isCalleeSavedReg (op.operand2))
            saveRegs.insert (op.operand2.reg);
    }
    
    setOutput (&blockPrologue);    
    beginRoutineBody (block.getSymbol ()->getName (), blockSymbols.getLevel (), blockSymbols, saveRegs, hasStackFrame);
    
    setOutput (&blockEpilogue);
    endRoutineBody (blockSymbols.getLevel (), blockSymbols, saveRegs, hasStackFrame, isFar);
    outputLocalDefinitions ();
    
    if (!globalInits.empty ()) 
        std::move (globalInits.begin (), globalInits.end (), std::back_inserter (blockEpilogue));
    
    std::move (blockPrologue.begin (), blockPrologue.end (), std::back_inserter (blockStatements));
    std::move (blockCode.begin (), blockCode.end (), std::back_inserter (blockStatements));
    std::move (blockEpilogue.begin (), blockEpilogue.end (), std::back_inserter (blockStatements));

    
//    removeUnusedLocalLabels (blockStatements);
    optimizePeepHole (blockStatements);
//    optimizeSingleLine (blockStatements);
//    optimizeJumps (blockStatements);
//    removeUnusedLocalLabels (blockStatements);
    
    std::move (blockStatements.begin (), blockStatements.end (), std::back_inserter (output));
    setOutput (&output);
}

void T9900Generator::generateBlock (TBlock &block) {

    TSymbolList &blockSymbols = block.getSymbols ();
    makeUniqueLabelNames (blockSymbols);
    
    currentLevel = blockSymbols.getLevel ();
    
    TRoutineType *routine = static_cast<TRoutineType *> (block.getSymbol ()->getType ());
    bool isFar = routine->isFarCall ();
    
//    std::cout << "Entering: " << block.getSymbol ()->getName () << ", level: " << blockSymbols.getLevel () << std::endl;

    assignStackOffsets (block);
    clearRegsUsed ();
    
    codeBlock (block, true, isFar, currentLevel == 1 ? mainProgram.codeSequence : subPrograms.back ().codeSequence);

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
    
//    std::move (blockStatements.begin (), blockStatements.end (), std::back_inserter (program));
    
    for (TSymbol *s: blockSymbols) {
        if (s->checkSymbolFlag (TSymbol::Routine)) {
            if (blockSymbols.getLevel () == 1) {
//                std::cout << "Creating code block: " << s->getName () << std::endl;
                subPrograms.push_back (TCodeBlock {s});
                setOutput (&subPrograms.back ().codeSequence);
            }
            if (s->checkSymbolFlag (TSymbol::External))
                externalRoutine (s);
            else 
                visit (s->getBlock ());
        }
    }
}

T9900Operand T9900Generator::makeLabelMemory (const std::string label, T9900Reg index) {
    return T9900Operand (label, index);
}



T9900Reg T9900Generator::parseRegister (TCompilerImpl &compiler, TLexer &lexer) {
    std::int64_t regNr = -1;
    if (lexer.getToken () == TToken::Identifier) {
        std::string s= lexer.getIdentifier ();
        if (s [0] == 'R') 
            s [0] = 'r';
        for (int i = 0; i < 16; ++i)
            if (s == regname [i]) {
                regNr = i;
                break;
            }
    } else if (lexer.getToken () == TToken::IntegerConst)
        regNr = lexer.getInteger ();
    if (regNr < 0 || regNr > 15) {
        regNr = 0;
        compiler.errorMessage (TCompilerImpl::AssemblerError, "Invalid register");
    }
    lexer.getNextToken ();
    return static_cast<T9900Reg> (regNr);
}

T9900Operand T9900Generator::parseInteger (TCompilerImpl &compiler, TLexer &lexer, std::int64_t minval, std::int64_t maxval) {
    if (maxval == 0xffff && lexer.getToken () == TToken::Identifier) {
        T9900Operand ret = lexer.getString ();
        lexer.getNextToken ();
        return ret;
    }
    std::int64_t val = lexer.getInteger ();
    compiler.checkToken (TToken::IntegerConst, "Integer constant expected");
    if (val < minval || val > maxval) {
        compiler.errorMessage (TCompilerImpl::AssemblerError, "Integer out of range (allowed is " + std::to_string (minval) + " - " + std::to_string (maxval) + ")");;
        val = minval;
    }
    return val;
}   


T9900Operand T9900Generator::parseGeneralAddress (TCompilerImpl &compiler, TLexer &lexer) {
    TToken t = lexer.getToken ();
    std::string label;
    std::int64_t val;
    T9900Reg reg = T9900Reg::r0;
    switch (t) {
        case TToken::AddrOp: 
            lexer.getNextToken ();
            if (lexer.getToken () == TToken::Identifier)
                label = lexer.getIdentifier ();
            else if (lexer.getToken () == TToken::IntegerConst) {
                val = lexer.getInteger ();
                if (val < 0 || val > 65535)
                    compiler.errorMessage (TCompilerImpl::AssemblerError, "Memory address out of range");
            } else
                compiler.errorMessage (TCompilerImpl::AssemblerError, "Label or memory address expected");
            lexer.getNextToken ();
            if (lexer.checkToken (TToken::BracketOpen)) {
                reg = parseRegister (compiler, lexer);
                if (reg == T9900Reg::r0)
                    compiler.errorMessage (TCompilerImpl::AssemblerError, "Cannot use R0 for indexed memory addressing");
                compiler.checkToken (TToken::BracketClose, "')' expected at end of indexed memory addressing");
            }
            if (!label.empty ())
                return T9900Operand (label, reg);
            else
                return T9900Operand (reg, val);
        case TToken::Mul: {
            lexer.getNextToken ();
            T9900Reg reg = parseRegister (compiler, lexer);
            if (lexer.getToken () == TToken::Add) {
                lexer.getNextToken ();
                return T9900Operand (reg, T9900Operand::TAddressingMode::RegIndInc);
            } else
                return T9900Operand (reg, T9900Operand::TAddressingMode::RegInd); }
        case TToken::Identifier:
        case TToken::IntegerConst:
            return T9900Operand (parseRegister (compiler, lexer), T9900Operand::TAddressingMode::Reg);
        default:
            compiler.errorMessage (TCompilerImpl::AssemblerError, "Invalid general address");
            return T9900Operand ();
    }
}

void T9900Generator::parseAssemblerBlock (TSymbol *symbol, TBlock &block) {
//    std::cout << "Parsing ASM block " << symbol->getName () << std::endl;
    TCompilerImpl &compiler = block.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    lexer.setAssemblerMode (true);
    
    TCodeSequence &output = assemblerBlocks [symbol];
    
    while (lexer.getToken () != TToken::End && lexer.getToken () != TToken::Error) {
        const std::string &opcode = lexer.getIdentifier ();
        if (lexer.getToken () != TToken::DivInt)
            compiler.checkToken (TToken::Identifier, "Expected label definition or mnemonic");
        else
            lexer.getNextToken ();
//        std::cout << opcode << std::endl;
        T9900OpDescription desc;
        T9900Operand op1, op2;
        if (lookupInstruction (opcode, desc)) {
            switch (desc.format) {
                case T9900Format::F1:
                    op1 = parseGeneralAddress (compiler, lexer);
                    compiler.checkToken (TToken::Comma, "Comma expected after general address");
                    op2 = parseGeneralAddress (compiler, lexer);
                    break;
                case T9900Format::F2:
                    op1 = lexer.getIdentifier ();
                    compiler.checkToken (TToken::Identifier, "Label expected as target of " + opcode);
                    break;
                case T9900Format::F2M:
                    op1 = lexer.getInteger ();
                    compiler.checkToken (TToken::IntegerConst, "Integer expected in I/O instruction");
                    // TODO: range check
                    break;
                case T9900Format::F3:
                    op1 = parseRegister (compiler, lexer);
                    compiler.checkToken (TToken::Comma, "Comma expected after register");
                    op2 = parseGeneralAddress (compiler, lexer);
                    break;
                case T9900Format::F4:
                    op1 = parseGeneralAddress (compiler, lexer);
                    compiler.checkToken (TToken::Comma, "Comma expected after register");
                    op2 = parseInteger (compiler, lexer, 0, 15);
                    break;
                case T9900Format::F5:
                    op1 = parseRegister (compiler, lexer);
                    compiler.checkToken (TToken::Comma, "Comma expected after register");
                    op2 = parseInteger (compiler, lexer, 0, 15);
                    break;
                case T9900Format::F6:
                    op1 = parseGeneralAddress (compiler, lexer);
                    break;
                case T9900Format::F7:
                    // no operand
                    break;
                case T9900Format::F8:
                    op1 = parseRegister (compiler, lexer);
                    compiler.checkToken (TToken::Comma, "Comma expected after register");
                    op2 = parseInteger (compiler, lexer, -32768, 0xffff);
                    break;
                case T9900Format::F8_1:
                    op1 = parseInteger (compiler, lexer, 0, 0xffff);
                    break;
                case T9900Format::F8_2:
                    op1 = parseRegister (compiler, lexer);
                    break;
                case T9900Format::F9:
                    op1 = parseGeneralAddress (compiler, lexer);
                    compiler.checkToken (TToken::Comma, "Comma expected after register");
                    op2 = parseRegister (compiler, lexer);
                    break;
                case T9900Format::F_None:
                    switch (desc.opcode) {
                        case T9900Op::text:
                            if (lexer.getToken () == TToken::StringConst) 
                                op1 = T9900Operand (lexer.getString ());
                            else
                                compiler.errorMessage (TCompilerImpl::AssemblerError, "Expected string after 'text'");
                            lexer.getNextToken ();
                            break;
                        default:
                            break;
                    }
                    break;
            }
            output.push_back (T9900Operation (desc.opcode, op1, op2));
        } else {
            if (lexer.getToken () == TToken::Colon)
                lexer.getNextToken ();
            output.push_back (T9900Operation (T9900Op::def_label, opcode));
        } 
    }
    compiler.checkToken (TToken::End, "'end' expected at end of assembler block");
    lexer.setAssemblerMode (false);
}

} // namespace statpascal

