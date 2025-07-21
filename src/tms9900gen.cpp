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

/*

9900 calling conventions 

r0:	   scratch reg; used for shift/rotate count
r1 - r8:   used as calculator stack; saved by callee
r9:	   used as frame pointer; saved by callee
r10:       stack pointer (decrementing, points to last value in stack)
r11:	   link register
r12 - r15: saved by caller

Stack layout of activation frame upon entry:

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
    intStackReg = {TMS9900Reg::r1, TMS9900Reg::r2, TMS9900Reg::r3, TMS9900Reg::r4, TMS9900Reg::r5, TMS9900Reg::r6, TMS9900Reg::r7, TMS9900Reg::r8}; 
    
const T9900Reg 
    intScratchReg1 = TX64Reg::r0,
    intScratchReg2 = TX64Reg::r12;

const std::size_t
    intStackRegs = intStackReg.size ();
    
const std::string 
    globalRuntimeDataName = "__globalruntimedata",
    dblAbsMask = "__dbl_abs_mask";

TX64OpSize getX64OpSize (TType *const t) {
    if (t == &stdType.Int8 || t == &stdType.Uint8)
        return TX64OpSize::bit8;
    if (t == &stdType.Int16 || t == &stdType.Uint16)
        return TX64OpSize::bit16;
    if (t == &stdType.Int32 || t == &stdType.Uint32)
        return TX64OpSize::bit32;
    return TX64OpSize::bit64;
}

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
    return op.isReg && isCalleeSavedReg (op.reg);
}

bool T9900Generator::isCalcStackReg (const T9900Reg reg) {
    return isCalleeSavedReg (reg);
}

bool T9900Generator::isCalcStackReg (const T9900Operand &op) {
    return op.isReg && isCalcStackReg (op.reg);
}

bool T9900Generator::isSameReg (const T9900Operand &op1, const T9900Operand &op2) {
    return op1.isReg && op2.isReg && op1.reg == op2.reg;
}

bool T9900Generator::isSameCalcStackReg (const T9900Operand &op1, const T9900Operand &op2) {
    return isCalcStackReg (op1) && isCalcStackReg (op2) && op1.reg == op2.reg;
}

//bool TX64Generator::isRegisterIndirectAddress (const TX64Operand &op) {
//    return op.isPtr && op.index == TX64Reg::none && op.offset == 0;
//}

void T9900Generator::removeLines (TCodeSequence &code, TCodeSequence::iterator &line, std::size_t count) {
    for (std::size_t i = 0; i < count && line != code.end (); ++i)
        line = code.erase (line);
    for (std::size_t i = 0; i < count && line != code.begin (); ++i)
        --line;
}


#if 0

void TX64Generator::modifyReg (TX64Operand &operand, TX64Reg a, TX64Reg b) {
    if (operand.isReg) 
        if (operand.reg == a) operand.reg = b;
    if (operand.isPtr) {
        if (operand.base == a) operand.base = b;
        if (operand.index == a) operand.index = b;
    }
}

static void addUsedRegs (std::array<bool, static_cast<std::size_t> (TX64Reg::nrRegs)> &regsUsed, TX64Operand operand) {
    if (operand.isReg)
        regsUsed [static_cast<std::size_t> (operand.reg)] = true;
    if (operand.isPtr) {
        if (operand.base != TX64Reg::none)
            regsUsed [static_cast<std::size_t> (operand.base)] = true;
        if (operand.index != TX64Reg::none)
            regsUsed [static_cast<std::size_t> (operand.index)] = true;
    }
}

void TX64Generator::tryLeaveFunctionOptimization (TCodeSequence &code) {
    std::array<bool, static_cast<std::size_t> (TX64Reg::nrRegs)> regsUsed;
    regsUsed.fill (false);
    bool callUsed = false;
    for (TX64Operation &op: code) {
        if (op.operation == TX64Op::call)
            callUsed = true;
        if (op.operation == TX64Op::idiv)
            addUsedRegs (regsUsed, TX64Reg::rdx);
        addUsedRegs (regsUsed, op.operand1);
        addUsedRegs (regsUsed, op.operand2);
    }
    
    
    if (!callUsed) {
        static const std::vector<TX64Reg> r1 = {TX64Reg::rax, TX64Reg::rcx, TX64Reg::rdi, TX64Reg::rsi, TX64Reg::rdx, TX64Reg::r8, TX64Reg::r9, TX64Reg::r10, TX64Reg::r11},
                                          r2 = {TX64Reg::rbx, TX64Reg::r12, TX64Reg::r13, TX64Reg::r14, TX64Reg::r15};
        std::vector<TX64Reg> avail, used;
        for (TX64Reg r: r1)
            if (!regsUsed [static_cast<std::size_t> (r)])
                avail.push_back (r);
        for (TX64Reg r: r2)
            if (regsUsed [static_cast<std::size_t> (r)])
                used.push_back (r);
/*                
        std::cout << std::endl;
        for (std::size_t i = 0; i < std::min (avail.size (), used.size ()); ++i) 
            std::cout << x64RegName [static_cast<std::size_t> (used [i])] << " -> " << x64RegName [static_cast<std::size_t> (avail [i])] << std::endl;
*/
        for (std::size_t i = 0; i < std::min (avail.size (), used.size ()); ++i)
            for (TX64Operation &op: code) {
                modifyReg (op.operand1, used [i], avail [i]);
                modifyReg (op.operand2, used [i], avail [i]);
            }
    }
}

void TX64Generator::removeUnusedLocalLabels (TCodeSequence &code) {
    std::set<std::string> usedLabels;
    for (TX64Operation &op: code) 
        if (op.operation != TX64Op::def_label) {
            if (op.operand1.isLabel)
                usedLabels.insert (op.operand1.label);
            if (op.operand2.isLabel)
                usedLabels.insert (op.operand2.label);
        }
    TCodeSequence::iterator line = code.begin ();
    while (line != code.end ())
        if (line->operation == TX64Op::def_label && line->operand1.label.substr (0, 2) == ".l" && usedLabels.find (line->operand1.label) == usedLabels.end ())
            line = code.erase (line);
        else
            ++line;
}

void TX64Generator::trySingleReplacements (TCodeSequence &code) {
    for (TX64Operation &opcode: code) {
        TX64Op &op = opcode.operation;
        TX64Operand &op1 = opcode.operand1,
                    &op2 = opcode.operand2;
        
        // sub rsp, 8
        // ->
        // push rax
        if (op == TX64Op::sub && op1.isReg && op1.reg == TX64Reg::rsp && op2.isImm && op2.imm == 8) {
            op = TX64Op::push;
            op1 = TX64Reg::rax;
            op2.isImm = false;
        }  
        
        // add | sub op, 1
        // ->
        // inc | dec op
        else if ((op == TX64Op::add || op == TX64Op::sub) && op2.isImm && (op2.imm == 1 || op2.imm == -1)) {
            op = ((op == TX64Op::add) == (op2.imm == 1)) ? TX64Op::inc : TX64Op::dec;
            op2.isImm = false;
        }
        // cmp reg, 0
        // ->
        // test reg, reg
        else if (op == TX64Op::cmp && op1.isReg && op2.isImm && !op2.imm) {
            op = TX64Op::test;
            op2 = op1;
        }
            
        // mov reg, 0
        // ->
        // xor reg, reg
        else if (op == TX64Op::mov && op1.isReg && op2.isImm && !op2.imm) {
            op = TX64Op::xor_;
            op2 = op1;
        }
        
    }
}

bool TX64Generator::tryReplaceShift (TX64Operand &op, TX64Reg reg, ssize_t imm) {
    if (op.isPtr && op.index == reg && (op.shift << imm) <= 8) {
        op.shift <<= imm;
        return true;
    }
    return false;
}

/** merge address calculations after
    lea rstack, [op2]
    and usage of rstack in memmory operand in following operaton
*/    

bool TX64Generator::tryMergeOperands (TX64Operand &op1, TX64Reg r1, const TX64Operand &op2) {
    if (op1.isPtr) {
         if (op2.isPtr && op2.index == TX64Reg::none && is32BitLimit (op1.offset + op2.offset)) {
            bool changed = op1.base == r1 || op1.index == r1;
            if (op1.base == r1) {
                op1.base = op2.base;
                op1.offset += op2.offset;
            }
            if (op1.index == r1) {
                op1.index = op2.base;
                // TODO: check limits
                op1.offset += (op2.offset * op1.shift);
            }
            return changed;
        }
        if (op2.isImm && (op1.index == r1 || op1.base == r1) && is32BitLimit (op1.offset + op2.imm)) {
            if (op1.index == r1)
                op1.offset += op2.imm * op1.shift;
            else
                op1.offset += op2.imm;
            return true;
        }
        if (op2.isReg && op1.base == r1 && op1.index == TX64Reg::none) {
            op1.index = op2.reg;
            op1.shift = 1;
            return true;
        }
        if ((isRIPRelative (op2) || op2.isPtr) && isRegisterIndirectAddress (op1) && isSameCalcStackReg (r1, op1.base)) {
            TX64Operand op = op2;
            op.opSize = op1.opSize;
            op1 = op;
            return true;
        }
    }
    return false;
}

bool TX64Generator::tryReplaceOperand (TX64Operand &op1, TX64Operand &op2, TX64OpSize opSize)  {
    if (op2.isImm) {
        if (is32BitLimit (op2.imm)) {
            op1.opSize = opSize == TX64OpSize::bit_default ? TX64OpSize::bit64 : opSize;
            return true;
        } else
            return false;
    }
    if (op2.isReg) {
        op2.opSize = opSize;
        return true;
    }
    if (op1.isReg || op2.isReg) {
        const TX64OpSize opSize1 = op1.opSize == TX64OpSize::bit_default ? TX64OpSize::bit64 : op1.opSize,
                         opSize2 = op2.opSize == TX64OpSize::bit_default ? TX64OpSize::bit64 : op2.opSize;
        return opSize1 == opSize2;
    }
    return false;
}

namespace {
bool logOptimizer = false;
}

void TX64Generator::optimizePeepHole (TCodeSequence &code) {
    bool doLog = true;
    
    code.push_back (TX64Op::end);
    code.push_back (TX64Op::end);
    
    TCodeSequence::iterator line = code.begin ();
    while (line != code.end ()) {	// line + 1 !!!!
        if (logOptimizer && doLog) {
            for (const TX64Operation &op: code)
                std::cout << op.makeString () << std::endl;
            std::cout << std::endl << "----------------" << std::endl;
        }
        doLog = true;
        
        TCodeSequence::iterator line1 = line;
        ++line1;
        TCodeSequence::iterator line2 = line1;
        ++line2;
        TCodeSequence::iterator line3 = line2;
        ++line3;
        
        TX64Operand &op_1_1 = line->operand1,
                    &op_1_2 = line->operand2;
        TX64Op      &op1 = line->operation;
        std::string &comm_1 = line->comment;
        
        TX64Operand &op_2_1 = line1->operand1,
                    &op_2_2 = line1->operand2;
        TX64Op      &op2 = line1->operation;
        std::string &comm_2 = line1->comment; 
                    
        // push r1
        // pop r2
        // ->
        // mov r2, r2
        if (op1 == TX64Op::push && op2 == TX64Op::pop && op_1_1.isReg) {
            op2 = TX64Op::mov;
            op_2_2 = op_1_1;
            removeLines (code, line, 1);
        }
        
        // mov[apd] reg, reg
        // -> remove
        else if ((op1 == TX64Op::mov || op1 == TX64Op::movapd) && isSameReg (op_1_1, op_1_2))
            removeLines (code, line, 1);

        // add | sub op, 0
        // -> remove
        else if ((op1 == TX64Op::add || op1 == TX64Op::sub) && op_1_2.isImm && op_1_2.imm == 0) 
            removeLines (code, line, 1);
            
        // imul reg, imm1
        // imul reg, imm2
        // ->
        // imul reg, imm1 * imm2
        else if (op1 == TX64Op::imul && op2 == TX64Op::imul && isSameReg (op_1_1, op_2_1) && op_1_2.isImm && op_2_2.isImm && is32BitLimit (op_1_2.imm * op_2_2.imm)) {
            op_2_2.imm *= op_1_2.imm;
            removeLines (code, line, 1);
        }
        
        // add reg, imm1
        // add reg, imm2
        // ->
        // add reg, imm1 + imm2
        
        else if (op1 == TX64Op::add && op2 == TX64Op::add && isSameReg (op_1_1, op_2_1) && op_1_2.isImm && op_2_2.isImm && is32BitLimit (op_1_2.imm + op_2_2.imm)) {
            op_2_2.imm += op_1_2.imm;
            removeLines (code, line, 1);
        }
        
        // mov rstack, imm1
        // shl | imul | add rstack, imm2
        // -> 
        // mov rstack, val
        else if (op1 == TX64Op::mov && (op2 == TX64Op::shl || op2 == TX64Op::imul || op2 == TX64Op::add) && isSameReg (op_1_1, op_2_1) && op_1_2.isImm && op_2_2.isImm) {
            if (op2 == TX64Op::shl)
                op_2_2.imm = op_1_2.imm << op_2_2.imm;
            else if (op2 == TX64Op::imul)
                op_2_2.imm *= op_1_2.imm;
            else
                op_2_2.imm += op_1_2.imm;
            op2 = TX64Op::mov;
            removeLines (code, line, 1);
        }
        
        // lea r1, [rel f]
        // call r1
        // ->
        // call f
        else if (op1 == TX64Op::lea && op2 == TX64Op::call && isRIPRelative (op_1_2) && isSameReg (op_1_1, op_2_1)) {
            op_2_1 = TX64Operand (op_1_2.label);
            removeLines (code, line, 1);
        }
        
        // lea | mov[sx|zx|zxd] rstack, op
        // mov reg, rstack
        // ->
        // lea | mov[sx|zx|zxd] reg, op
        else if ((op1 == TX64Op::lea || op1 == TX64Op::mov || op1 == TX64Op::movsx || op1 == TX64Op::movzx || op1 == TX64Op::movsxd) && op2 == TX64Op::mov &&
            isSameCalcStackReg (op_1_1, op_2_2) && op_2_1.isReg) {
            op_1_1.reg = op_2_1.reg;
            ++line;
            removeLines (code, line, 1);
        }

        // mov rstack. op1
        // op op2, rstack
        // ->
        // op op2, op1
        else if (op1 == TX64Op::mov && (op2 == TX64Op::mov || op2 == TX64Op::cmp || op2 == TX64Op::add || op2 == TX64Op::sub || op2 == TX64Op::imul || op2 ==  TX64Op::and_ || op2 == TX64Op::or_ || op2 == TX64Op::xor_) && 
                 isSameCalcStackReg (op_1_1, op_2_2) && tryReplaceOperand (op_2_1, op_1_2, op_2_2.opSize)) {
            if (comm_2.empty ())
                comm_2 = comm_1;
            op_2_2 = op_1_2;
            removeLines (code, line, 1);
        }
        
        // lea rstack, [op]
        //
        // try merge with following operation
        else if (op1 == TX64Op::lea && isCalcStackReg (op_1_1) &&
                 (tryMergeOperands (op_2_1, op_1_1.reg, op_1_2) || tryMergeOperands (op_2_2, op_1_1.reg, op_1_2))) {
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // mov rstack, reg1
        // 
        // handle as "lea rstack, [r1]" and try merge with following operation
        else if (op1 == TX64Op::mov && op_1_2.isReg && isCalcStackReg (op_1_1) && 
                 (tryMergeOperands (op_2_1, op_1_1.reg, TX64Operand (op_1_2.reg, 0)) || tryMergeOperands (op_2_2, op_1_1.reg, TX64Operand (op_1_2.reg, 0)))) {
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }

        // add rstack, reg | imm
        //
        // handle as lea rstack, [rstack + reg | imm] and try merge with following operation
        else if (op1 == TX64Op::add && isCalcStackReg (op_1_1) && 
                 ((op_1_2.isReg && (tryMergeOperands (op_2_1, op_1_1.reg, TX64Operand (op_1_1.reg, op_1_2.reg, 1, 0)) || tryMergeOperands (op_2_2, op_1_1.reg, TX64Operand (op_1_1.reg, op_1_2.reg, 1, 0)))) ||
                  (op_1_2.isImm && (tryMergeOperands (op_2_1, op_1_1.reg, TX64Operand (op_1_1.reg, op_1_2.imm)) || tryMergeOperands (op_2_2, op_1_1.reg, TX64Operand (op_1_1.reg, op_1_2.imm)))))) {
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }

        // lea reg, [reg + v]
        // -> 
        // add reg, v
        else if (op1 == TX64Op::lea && op_1_2.isPtr && op_1_1.reg == op_1_2.base && (op_1_2.index == TX64Reg::none || (op_1_2.offset == 0 && op_1_2.shift == 1))) {
            op1 = TX64Op::add;
            op_1_2 = (op_1_2.index != TX64Reg::none) ? TX64Operand (op_1_2.index) : TX64Operand (op_1_2.offset);
            if (line != code.begin ()) --line;
        }
        
        // lea r1, [r2]
        // ->
        // mov r1, r2
        else if (op1 == TX64Op::lea && isRegisterIndirectAddress (op_1_2)) {
            op1 = TX64Op::mov;
            op_1_2 = TX64Operand (op_1_2.base);
            if (line != code.begin ()) --line;
        }
            
        // shl rstack, [1|2|3]
        // try rstack as index in following operation
        else if (op1 == TX64Op::shl && isCalcStackReg (op_1_1) && op_1_2.isImm && (tryReplaceShift (op_2_1, op_1_1.reg, op_1_2.imm) || tryReplaceShift (op_2_2, op_1_1.reg, op_1_2.imm)))
            removeLines (code, line, 1);
        
        // mov reg1, reg2
        // add reg1, n
        // ->
        // lea reg1, [reg2 + n]
        
        else if (op1 == TX64Op::mov && op2 == TX64Op::add && isSameReg (op_1_1, op_2_1) && op_1_2.isReg && op_2_2.isImm) {
            op_2_2 = TX64Operand (op_1_2.reg, TX64Reg::none, 1, op_2_2.imm);
            op2 = TX64Op::lea;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // add rstack, n | reg
        // mov reg, rstack
        // ->
        // lea reg, [rstack + n | reg}
        else if (op1 == TX64Op::add && op2 == TX64Op::mov && isSameCalcStackReg (op_1_1, op_2_2) && op_2_1.isReg && (op_1_2.isImm || op_1_2.isReg)) {
            if (op_1_2.isReg)
                op_2_2 = TX64Operand (op_1_1.reg, op_1_2.reg, 1, 0);
            else
                op_2_2 = TX64Operand (op_1_1.reg, TX64Reg::none, 1, op_1_2.imm);
            op2 = TX64Op::lea;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // add reg1, reg2
        // add|sub reg1, imm
        // ->
        // lea reg1, [reg1 + reg2 +|. imm]

        // TODO: 3 address lea is slow, avoid?
        else if (op1 == TX64Op::add && (op2 == TX64Op::add || op2 ==TX64Op::sub) && isSameReg (op_1_1, op_2_1) && op_1_2.isReg && op_2_2.isImm) {
            op_2_2 = TX64Operand (op_1_1.reg, op_1_2.reg, 1, op2 == TX64Op::add ? op_2_2.imm : -op_2_2.imm);
            op2 = TX64Op::lea;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // imul rstack, r2
        // mov r2, rstack
        // ->
        // imul r2, rstack
        else if (op1 == TX64Op::imul && op2 == TX64Op::mov && isSameCalcStackReg (op_1_1, op_2_2) && isSameReg (op_1_2, op_2_1)) {
            op2 = TX64Op::imul;
            removeLines (code, line, 1);
        }
        
        // movq/movapd xmmstack, {op1]
        // op xmmreg2, xmmstack
        // ->
        // op xmmreg2, [op1]
        else if ((op1 == TX64Op::movq || op1 == TX64Op::movapd) && (op2 ==  TX64Op::comisd || op2 == TX64Op::addsd || op2 == TX64Op::subsd || op2 == TX64Op::mulsd || op2 == TX64Op::divsd || op2 == TX64Op::comisd || op2 == TX64Op::sqrtsd) && isSameXmmStackReg (op_1_1, op_2_2) && (op_1_2.isPtr || isRIPRelative (op_1_2) || isXmmReg (op_1_2))) {
            op_2_2 = op_1_2;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }

        // mov rstack. op1
        // cmp rstack, op2
        // ->
        // cmp op1, op2
        else if (op1 == TX64Op::mov && op2 == TX64Op::cmp && isSameCalcStackReg (op_1_1, op_2_1) && (op_2_2.isImm ||  op_1_1.opSize == op_2_2.opSize) && op_1_2.isReg) {
            op_2_1 = op_1_2;
            removeLines (code, line, 1);
        }

        // movapd xmmstack, xmmreg2
        // comisd xmmstack, op
        // ->	2 rules above ????
        // comisd xmmreg2, op
        else if (op1 == TX64Op::movapd && op2 == TX64Op::comisd && isSameXmmStackReg (op_1_1, op_2_1) && op_1_2.isReg) {
            op_2_1 = op_1_2;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // mov[[sx|zx|zxd] rstack, op1
        // cmp rstack, op2
        // ->
        // cmp op1, op2
/*        

    Problems with sign extension: casetest.sp

        else if (checkCode (code, line, {TX64Op::mov, TX64Op::movsx,  TX64Op::movzx, TX64Op::movsxd}, {TX64Op::cmp}) && isSameCalcStackReg (op_1_1, op_2_1) && (op_1_2.isReg || op_1_2.isPtr) && (op_2_2.isReg || op_2_2.isPtr || op_2_2.isImm) && !(op_1_2.isPtr && op_2_2.isPtr)) {
            op1 = TX64Op::cmp;
            op_1_1 = op_1_2;
            op_1_2 = op_2_2;
            ++line;
            removeLines (code, line, 1);
        }
*/
        // and|or|xor|add|sub reg, op
        // cmp|test reg, 0
        // -> remove comparison (do this after any LEA replacements which will not set flags)
        else if ((op1 == TX64Op::and_ || op1 == TX64Op::or_ || op1 == TX64Op::xor_ || op1 == TX64Op::add || op1 == TX64Op::sub) && (op2 == TX64Op::cmp || op2 == TX64Op::test) &&
                isSameCalcStackReg (op_1_1, op_2_1) && ((op_2_2.isImm && op_2_2.imm == 0) || op2 == TX64Op::test)) {
            ++line;
            removeLines (code, line, 1);
        }
        
        // movq/movapd xmmstack, op
        // movapd xmmreg, xmmstack
        // ->
        // movq/movapd xmmreg, op
        //
        // movsd xmmstack, xmmreg	-- not handled here?
        // movq op, xmmstack
        // ->
        // movq op, xmmreg
        else if ((op1 == TX64Op::movq || op1 == TX64Op::movapd) && op2 == TX64Op::movapd && isSameXmmStackReg (op_1_1, op_2_2) && (isXmmReg (op_2_1) || isXmmReg (op_1_2))) {
            op2 = op1;
            op_2_2 = op_1_2;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // mov rstack, op
        // cvtsi2sd xmmreg, rstack
        // ->
        // cvtsi2sd xmmreg, op
        else if (op1 == TX64Op::mov && op2 == TX64Op::cvtsi2sd && isSameCalcStackReg (op_1_1, op_2_2) && !op_1_2.isImm) {
            op_2_2 = op_1_2;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // movq/movapd xmmreg1, op
        // movapd xmm0, xmmreg1
        // ->
        // movq/movapd xmm0, op
        else if ((op1 == TX64Op::movapd || op1 == TX64Op::movq) && op2 == TX64Op::movapd && isSameXmmReg (op_1_1, op_2_2) && op_2_1.isReg && op_2_1.reg == TX64Reg::xmm0) {
            op2 = op1;
            op_2_2 = op_1_2;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // setcc reg8
        // movzx reg32, reg8
        // cmp reg64, 1
        // je|jne label
        // ->
        // remove cmp
        else if ((op1 >= TX64Op::seta && op1 <= TX64Op::setz) && op2 == TX64Op::movzx &&
                 line3 != code.end () && line3->operation == TX64Op::cmp && line2->operand2.isImm && 
                 line2->operand2.imm == 1 && (line3->operation == TX64Op::jne || line3->operation == TX64Op::je)) {
            static const std::map<TX64Op, std::map<TX64Op, TX64Op>> replaceOp = {
                {TX64Op::jne, {{TX64Op::seta, TX64Op::jbe}, {TX64Op::setae, TX64Op::jb}, {TX64Op::setb, TX64Op::jae}, {TX64Op::setbe, TX64Op::ja}, {TX64Op::setc, TX64Op::jnc}, {TX64Op::setg, TX64Op::jle}, {TX64Op::setge, TX64Op::jl}, {TX64Op::setl, TX64Op::jge}, {TX64Op::setle, TX64Op::jg}, {TX64Op::setnz, TX64Op::je}, {TX64Op::setz, TX64Op::jne}}},
                {TX64Op::je,  {{TX64Op::seta, TX64Op::ja}, {TX64Op::setae, TX64Op::jae}, {TX64Op::setb, TX64Op::jb}, {TX64Op::setbe, TX64Op::jbe}, {TX64Op::setc, TX64Op::jc}, {TX64Op::setg, TX64Op::jg}, {TX64Op::setge, TX64Op::jge}, {TX64Op::setl, TX64Op::jl}, {TX64Op::setle, TX64Op::jle}, {TX64Op::setnz, TX64Op::jne}, {TX64Op::setz, TX64Op::je}}}
            };
            line3->operation = replaceOp.at (line3->operation).at (op1);
            line = line2;
            removeLines (code, line, 1);
        }
        
        // mov|movsx|movzx r1, val
        // mov rax, r1
        // ret		; yet an end as placeholder
        // ->
        // remove r1
        else if ((op1 == TX64Op::mov || op1 == TX64Op::movsx || op1 == TX64Op::movzx) && (op2 == TX64Op::mov) && line2 != code.end () && line2->operation == TX64Op::end &&
                 op_2_1.isReg && op_2_1.reg == TX64Reg::rax && isSameReg (op_2_2, op_1_1)) {
            op_1_1.reg = TX64Reg::rax;
            ++line;
            removeLines (code, line, 1);
        }
        
        // mov rstack, imm32
        // op op1, [rstack]
        // ->
        // op op1, [imm32]
        else if (op1 == TX64Op::mov && op_2_2.isPtr && op_2_2.index == TX64Reg::none && op_2_2.offset == 0 && isSameCalcStackReg (op_1_1, op_2_2.base) && op_1_2.isImm && op_1_2.imm <= std::numeric_limits<std::uint32_t>::max ()) {
            op_2_2.base = TX64Reg::none;
            op_2_2.offset = op_1_2.imm;
            comm_2 = comm_1;
            removeLines (code, line, 1);
        }
        
        // mov rstack, imm32
        // op [rstack], op1
        // ->
        // op [imm32], op1
        else if (op1 == TX64Op::mov && op_2_1.isPtr && op_2_1.index == TX64Reg::none && op_2_1.offset == 0 && isSameCalcStackReg (op_1_1, op_2_1.base) && op_1_2.isImm && op_1_2.imm <= std::numeric_limits<std::uint32_t>::max ()) {
            op_2_1.base = TX64Reg::none;
            op_2_1.offset = op_1_2.imm;
            removeLines (code, line, 1);
        }
        
        // mov r1, imm
        // mov r2, imm
        // ->
        // mov r1, imm
        // mov r2, r1

        else if (op1 == TX64Op::mov && op2 == TX64Op::mov && op_1_1.isReg && op_1_2.isImm && op_2_2.isImm && op_1_2.imm == op_2_2.imm && !isCalcStackReg (op_1_1.reg)) {
            op_2_2 = op_1_1;
        }
        
        else {
            doLog = false;
            ++line;
        }
    }
    
    while (!code.empty () && code.back ().operation == TX64Op::end)
        code.pop_back ();
}

#endif

/*
void TX64Generator::replaceLabel (TX64Operation &op, TX64Operand &operand, std::size_t offset) {
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
void TX64Generator::assemblePass (int pass, std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &listing) {
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

/*
void TX64Generator::getAssemblerCode (std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &listing) {
    opcodes.clear ();
    listing.clear ();

    for (int pass = 1; pass <= 2; ++pass)
        assemblePass (pass, opcodes, generateListing, listing);
}
*/

void T9900Generator::setOutput (TCodeSequence *output) {
    currentOutput = output;
}

void T9900Generator::outputCode (const TX64Operation &l) {
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
    outputCode (TX64Operation (T9900Op::comment, T9900Operand (), T9900Operand (), s));
}

void T9900Generator::outputLocalJumpTables () {
    if (!jumpTableDefinitions.empty ()) {
        outputComment ("jump tables for case statements");
        for (const TJumpTable &it: jumpTableDefinitions) {
            outputLabel (it.tableLabel);
            for (const std::string &s: it.jumpLabels)
                outputCode (TX64Op::data_diff_dq, (s.empty () ? it.defaultLabel : s), it.tableLabel);
        }
        jumpTableDefinitions.clear ();
    }
}

void TX9900Generator::outputGlobalConstants () {
//    outputCode (TX64Op::aligncode, TX64Operand (16));
    outputComment ("Double Constants");
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

std::string T00ßßGenerator::registerConstant (const std::array<std::uint64_t, TConfig::setwords> &v) {
    setDefinitions.push_back ({"__set_cnst_" + std::to_string (dblConstCount++), v});
    return setDefinitions.back ().label;
}

//void TX64Generator::registerLocalJumpTable (const std::string &tableLabel, const std::string &defaultLabel, std::vector<std::string> &&jumpLabels) {
//    jumpTableDefinitions.emplace_back (TJumpTable {tableLabel, defaultLabel, std::move (jumpLabels)});
//}

void T9900Generator::outputLabelDefinition (const std::string &label, const std::size_t value) {
    labelDefinitions [label] = value;
}

void T9900Generator::generateCode (TTypeCast &typeCast) {
    TExpressionBase *expression = typeCast.getExpression ();
    TType *destType = typeCast.getType (),
          *srcType = expression->getType ();
    
    visit (expression);
/*    
    if (srcType == &stdType.Int64 && destType == &stdType.Real) {
        const TX64Reg reg = getSaveXmmReg (xmmScratchReg1);
        outputCode (TX64Operation (TX64Op::cvtsi2sd, reg, fetchReg (intScratchReg1)));
        saveXmmReg (reg);
    }  else if (srcType == &stdType.String && destType->isPointer ()) {
        loadReg (TX64Reg::rdi);
        outputCode (TX64Operation (TX64Op::mov, TX64Reg::rsi, 1));
        outputCode (TX64Op::mov, TX64Reg::rax, TX64Operand ("rt_str_index"));
        outputCode (TX64Operation (TX64Op::call, TX64Reg::rax));
        saveReg (TX64Reg::rax);
    }
*/    
}

void T9900Generator::generateCode (TExpression &comparison) {
    outputBinaryOperation (comparison.getOperation (), comparison.getLeftExpression (), comparison.getRightExpression ());
}

void T9900Generator::outputBooleanCheck (TExpressionBase *expr, const std::string &label, bool branchOnFalse) {
    static const std::map<TToken, TX64Op> intFalseJmp = {
        {TToken::Equal, TX64Op::jne}, {TToken::GreaterThan, TX64Op::jle}, {TToken::LessThan, TX64Op::jge}, 
        {TToken::GreaterEqual, TX64Op::jl}, {TToken::LessEqual, TX64Op::jg}, {TToken::NotEqual, TX64Op::je}
    };
    static const std::map<TToken, TX64Op> realFalseJmp = {
        {TToken::Equal, TX64Op::jne}, {TToken::GreaterThan, TX64Op::jbe}, {TToken::LessThan, TX64Op::jae}, 
        {TToken::GreaterEqual, TX64Op::jb}, {TToken::LessEqual, TX64Op::ja}, {TToken::NotEqual, TX64Op::je}
    };
    static const std::map<TToken, TX64Op> intTrueJmp = {
        {TToken::Equal, TX64Op::je}, {TToken::GreaterThan, TX64Op::jg}, {TToken::LessThan, TX64Op::jl}, 
        {TToken::GreaterEqual, TX64Op::jge}, {TToken::LessEqual, TX64Op::jle}, {TToken::NotEqual, TX64Op::jne}
    };
    static const std::map<TToken, TX64Op> realTrueJmp = {
        {TToken::Equal, TX64Op::je}, {TToken::GreaterThan, TX64Op::ja}, {TToken::LessThan, TX64Op::jb}, 
        {TToken::GreaterEqual, TX64Op::jae}, {TToken::LessEqual, TX64Op::jbe}, {TToken::NotEqual, TX64Op::jne}
    };
    
    if (TPrefixedExpression *prefExpr = dynamic_cast<TPrefixedExpression *> (expr))
        if (prefExpr->getOperation () == TToken::Not) {
            branchOnFalse = !branchOnFalse;
            expr = prefExpr->getExpression ();
        }
    
    if (expr->isFunctionCall ()) {
        TFunctionCall *functionCall = static_cast<TFunctionCall *> (expr);
        if (static_cast<TRoutineValue *> (functionCall->getFunction ())->getSymbol ()->getExtSymbolName () == "rt_in_set") {
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
        }
    }
    
    TExpression *condition = dynamic_cast<TExpression *> (expr);
    
    if (condition && (condition->getLeftExpression ()->getType ()->isEnumerated () || condition->getLeftExpression ()->getType ()->isPointer ())) {
        visit (condition->getLeftExpression ());
        TExpressionBase *right = condition->getRightExpression ();
        if (right->isConstant ())
            // TODO: check for int limit !!!!
            outputCode (TX64Operation (TX64Op::cmp, fetchReg (intScratchReg1), static_cast<TConstantValue *> (right)->getConstant ()->getInteger ()));
        else {
            visit (condition->getRightExpression ());
            const TX64Reg r2 = fetchReg (intScratchReg1);
            const TX64Reg r1 = fetchReg (intScratchReg2);
            outputCode (TX64Operation (TX64Op::cmp, r1, r2));
        }
        outputCode (TX64Operation ((branchOnFalse ? intFalseJmp : intTrueJmp).at (condition->getOperation ()), label));
    } else if (condition && condition->getLeftExpression ()->getType ()->isReal ()) {
        visit (condition->getLeftExpression ());
        visit (condition->getRightExpression ());
        const TX64Reg r2 = fetchXmmReg (xmmScratchReg1);
        const TX64Reg r1 = fetchXmmReg (xmmScratchReg2);
        outputCode (TX64Op::comisd, r1, r2);
        outputCode (TX64Operation ((branchOnFalse ? realFalseJmp : realTrueJmp).at (condition->getOperation ()), label));
    } else {
        visit (expr);
        const TX64Reg reg = fetchReg (intScratchReg1);
        outputCode (TX64Operation (TX64Op::cmp, reg, 0));;
        outputCode (TX64Operation (branchOnFalse ? TX64Op::je : TX64Op::jne, label));
    }
}

void T9900Generator::outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    visit (left);
    TX64Reg reg = fetchReg (intScratchReg1);
    outputCode (TX64Operation (TX64Op::cmp, reg, 1));
    const std::string doneLabel = getNextLocalLabel ();
    outputCode (TX64Operation (operation == TToken::Or ? TX64Op::je : TX64Op::jne, doneLabel));;
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
        const TX64Reg r2 = fetchReg (intScratchReg1),
                      r1 = fetchReg (intScratchReg2);
        codeMultiplyConst (r2, basesize);
        outputCode (TX64Operation (TX64Op::add, r1, r2));
        saveReg (r1);
    } else {
        const TX64Reg r2 = fetchReg (intScratchReg1);
        for (std::size_t i = 1; i < 63; ++i)
            if (basesize == 1ull << i) {
                const TX64Reg r1 = fetchReg (intScratchReg2);
                outputCode (TX64Operation (TX64Op::sub, r1, r2));
                outputCode (TX64Operation (TX64Op::shr, r1, i));
                saveReg (r1);
                return;
            }
        setRegUsed (TX64Reg::rdx);
        setRegUsed (TX64Reg::rax);
        loadReg (TX64Reg::rax);
        outputCode (TX64Op::sub, TX64Reg::rax, r2);
        outputCode (TX64Op::cqo);
        outputCode (TX64Op::mov, r2, basesize);
        outputCode (TX64Op::idiv, r2);
        saveReg (TX64Reg::rax);
    }
}

void T9900Generator::outputIntegerCmpOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    static const std::map<TToken, TX64Op> cmpOperation = {
        {TToken::Equal, TX64Op::setz}, {TToken::GreaterThan, TX64Op::setg}, {TToken::LessThan, TX64Op::setl}, 
        {TToken::GreaterEqual, TX64Op::setge}, {TToken::LessEqual, TX64Op::setle}, {TToken::NotEqual, TX64Op::setnz}};
        
    bool secondFirst = left->isSymbol () || left->isConstant () || left->isLValueDereference ();
    TX64Reg r1, r2, r3;
    
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
    outputCode (TX64Op::cmp, r1, r2);
    outputCode (cmpOperation.at (operation), TX64Operand (r3, TX64OpSize::bit8));
    outputCode (TX64Op::movzx, TX64Operand (r3, TX64OpSize::bit32), TX64Operand (r3, TX64OpSize::bit8));
    saveReg (r3);
}

void T9900Generator::outputIntegerDivision (TToken operation, bool secondFirst, bool rightIsConstant, ssize_t n) {
    for (std::size_t i = 1; i < 63; ++i)
        if (n == 1LL << i) {
            TX64Reg reg = fetchReg (intScratchReg1);
            if (operation == TToken::DivInt) {
                if (n == 2) {
                    outputCode (TX64Op::bt, reg, 63);
                    outputCode (TX64Op::adc, reg, 0);
                } else {
                    outputCode (TX64Op::test, reg, reg);
                    const std::string l = getNextLocalLabel ();
                    outputCode (TX64Op::jge, l);
                    outputCode (TX64Op::add, reg, n - 1);
                    outputLabel (l);
                }
                outputCode (TX64Op::sar, reg, i);
            } else {
                outputCode (TX64Op::test, reg, reg);
                const std::string l1 = getNextLocalLabel (), l2 = getNextLocalLabel ();
                outputCode (TX64Op::jge, l1);
                outputCode (TX64Op::and_, reg, n - 1);
                outputCode (TX64Op::je, l2);
                outputCode (TX64Op::sub, reg, n);
                outputCode (TX64Op::jmp, l2);
                outputLabel (l1);
                outputCode (TX64Op::and_, reg, n - 1);
                outputLabel (l2);
            }
            saveReg (reg);
            return;
        }

    TX64Reg reg = intScratchReg1;
    if (rightIsConstant) {
        loadReg (TX64Reg::rax);
        outputCode (TX64Operation (TX64Op::mov, reg, n));
    } else if (secondFirst) {
        loadReg (TX64Reg::rax);
        reg = fetchReg (intScratchReg1);
    } else {
        reg = fetchReg (intScratchReg1);
        loadReg (TX64Reg::rax);
    }
    if (reg == intScratchReg1)
        setRegUsed (intScratchReg1);
    setRegUsed (TX64Reg::rdx);
    setRegUsed (TX64Reg::rax);
    outputCode (TX64Operation (TX64Op::cqo));
    outputCode (TX64Operation (TX64Op::idiv, reg));
    saveReg (operation == TToken::DivInt ? TX64Reg::rax : TX64Reg::rdx);
}

void TX9900Generator::outputIntegerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    if (left->isConstant () && (operation == TToken::Add || operation == TToken::Or || operation == TToken::Xor || operation == TToken::Mul || operation == TToken::And))
        std::swap (left, right);

    bool secondFirst = left->isSymbol () || left->isLValueDereference ();
    if (operation == TToken::Sub) secondFirst = false;

    TX64Reg r1;
    bool rightIsConst = right->isConstant ();
    const ssize_t n = rightIsConst ? static_cast<TConstantValue *> (right)->getConstant ()->getInteger () : 0;
    
    if (!is32BitLimit (n))
        rightIsConst = secondFirst = false;

    if (rightIsConst)
        visit (left);
    else if (secondFirst) {
        visit (right);
        visit (left);
    } else {
        visit (left);
        visit (right);   	
    }
    switch (operation) {
        case TToken::DivInt:
        case TToken::Mod: {
            outputIntegerDivision (operation, secondFirst, rightIsConst, n); 
            break; }
        case TToken::Shl:
        case TToken::Shr: {
            TX64Op op = operation == TToken::Shl ? TX64Op::shl : TX64Op::shr;
            if (rightIsConst) {
                r1 = fetchReg (intScratchReg1);
                outputCode (TX64Operation (op, r1, n));
            } else {
                setRegUsed (TX64Reg::rcx);
                if (secondFirst) {
                    r1 = fetchReg (intScratchReg1);
                    loadReg (TX64Reg::rcx);
                } else {
                    loadReg (TX64Reg::rcx);
                    r1 = fetchReg (intScratchReg1);
                }
                outputCode (op, r1, TX64Operand (TX64Reg::rcx, TX64OpSize::bit8));
            }
            saveReg (r1);
            break; }
        default: {
            TX64Operand right = rightIsConst ? TX64Operand (n) : TX64Operand (fetchReg (intScratchReg1));
            r1 = fetchReg (intScratchReg1);
            if (operation == TToken::Mul && rightIsConst && n >= 0)
                codeMultiplyConst (r1, n);
            else {
                static const std::map<TToken, TX64Op> opMap = {
                    {{TToken::Add, TX64Op::add}, {TToken::Sub, TX64Op::sub}, {TToken::Or, TX64Op::or_}, 
                     {TToken::Xor, TX64Op::xor_}, {TToken::Mul, TX64Op::imul}, {TToken::And, TX64Op::and_}}
                };
                outputCode (opMap.at (operation), r1, right);
            }
            saveReg (r1); }
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
        TX64Reg resreg = getSaveXmmReg (xmmScratchReg1);
        outputCode (TX64Operation (TX64Op::pxor, resreg, resreg));
        saveXmmReg (resreg);
        visit (prefixed.getExpression ());
        const TX64Reg r2 = fetchXmmReg (xmmScratchReg1),
                      r1 = fetchXmmReg (xmmScratchReg2);
        outputCode (TX64Operation (TX64Op::subsd, r1, r2));
        saveXmmReg (r1);
    } else {
        visit (prefixed.getExpression ());
        const TX64Reg reg = fetchReg (intScratchReg1);
        if (prefixed.getOperation () == TToken::Sub)
            outputCode (TX64Operation (TX64Op::neg, reg));
        else if (prefixed.getOperation () == TToken::Not) 
            if (type == &stdType.Boolean)
                outputCode (TX64Op::xor_, reg, 1);
            else
                outputCode (TX64Op::not_, reg);
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
    };
    std::vector<TParameterDescription> parameterDescriptions;
/*    
    for (const TSymbol *s: static_cast<TRoutineType *> (function->getType ())->getParameter ()) {
        std::cout << s->getName () << ": " << s->getType ()->getName () << std::endl;
    }
*/    
    bool usesReturnValuePointer = true
    const TSymbol *functionReturnTempStorage = functionCall.getFunctionReturnTempStorage ();
    TExpressionBase *returnStorage = functionCall.getReturnStorage ();
    
    std::vector<TExpressionBase *>::const_iterator it = args.begin ();
    for (const TSymbol *s: static_cast<TRoutineType *> (function->getType ())->getParameter ()) {
        TParameterDescription pd {s->getType (), *it, classifyType (pd.type) == ParameterLocation::IntReg);
        parameterDescriptions.push_back (pd);
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

    for (std::size_t i = 0; i < parameterDescriptions.size (); ++i) {
        visit (parameterDescriptions [i].actualParameter);
        const TX64Reg reg = fetchReg (intScratchReg1);
        codePush (reg);
    }
    
    if (usesReturnValuePointer) {
        visit (returnStorage);
        const TX64Reg reg = fetchReg (intScratchReg1);
        codePush (reg);
    }
    
    visit (function);
    const TX64Reg reg = fetchReg (intScratchReg1);
    outputCode (T9900Op::bl, T99Operand (reg, TAdressingMode::RegInd));

    intStackCount = savedIntStack;
    xmmStackCount = savedXmmStack;
    for (std::size_t i = 0; i < usedXmmStack; ++i)
        outputCode (TX64Op::movq, xmmStackReg [i], TX64Operand (TX64Reg::rsp, 8 * i));
    codeModifySP (8 * usedXmmStack);
    
    if (functionCall.getType () != &stdType.Void && !functionCall.isIgnoreReturn () && !returnStorage) {
        // loadReturnTemp
        saveReg (TX64Reg::rax);
    }
}

void TX64Generator::generateCode (TConstantValue &constant) {
    if (constant.getType ()->isSet ()) {
        //    
    } else if (constant.getType () == &stdType.Real) {
        //
    } else  (constant.getConstant ()->getType () == &stdType.String) {
        //
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
        outputCode (TX00ßßOperation (T9900Op::li, reg, s->getOverloadName ()));
    saveReg (reg);
}

void T9900Generator::codeSymbol (const TSymbol *s, const TX64Reg reg) {
    if (s->getLevel () == 1)	// global
        outputCode (T9900Op::li, reg, s->getOffset (), s->getOverloadName ());
    else if (s->getLevel () == currentLevel) {
        outputCode (T9900Op::mov, T9900Reg::r9, reg), 
        outputCode (T9900Op::ai, reg, s->getOffset (), s->getName ());
    } else {
        outputCode (T9900Op::mov, T9900Operand (T9900Reg, -(s->getLevel () - 1) * 2));
        outputCode (T9900Op::ai, reg, s->getOffset (), s->getName ());
    }
}

void T9900Generator::codeStoreMemory (TType *const t, T9900Operand destMem, T9900Reg srcReg) {
    if (t == &stdType.Uint8 || t == &stdType.Int8) {
        // WP should always be at 8300 so code direct mov?
        outputCode (T9900Op::swpb);
        outputCode (T9900Op::movb, srcReg, destMem);
        outputCode (T9900Op::swpb);
    } else if (t == &stdType.Uint16 || t == &stdType.Int16) {
        outputCode (T9900Op::mov, srcReg, destMem);
    } else if (t == &stdType.Uint32 || t == &stdType.Int32) {
        //
    } else if (t == &stdType.Int64) {
        //
    } else if (t == &stdType.Real) 
        //
    else if (t == &stdType.Single) {
        //
    }
}

void T9900Generator::codeSignExtension (TType *const t, const T9900Reg destReg, T9900Operand srcOperand) {
    if (t == &stdType.Int8) {
        outputCode (T9900Op::clr, destReg);
        outputCode (T9900Op::movb, srcOperand, destReg);
        outputCode (T9900Op::ja, "!");
        outputCode (T9900Op::ori, destReg, 0x00ff);
        outputCode (T9900Op::def_label, "!");
        outputCode (T9900Op::swpb);
    } else if (t == &stdType::Uint8) {
        outputCode (T9900Op::clr, destReg);
        outputCode (T9900Op::movb, srcOperand, destReg);
        outputCode (T9900Op::swpb);
    }
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

void T9900Generator::codeMultiplyConst (const TX64Reg reg, const std::size_t n) {
    if (n == 1)
        return;
    else if (n >= 2)
        for (std::size_t i = 1; i < 16; ++i)
            if (n == 1ull << i) {
                outputCode (T9900Op::li, T9900Reg::r0, n)
                outputCode (TX64Op::shl, T9900Reg::r0);
                return;
            }
    if (n == 0) 
        outputCode (T9900Op::clr, reg);
    else
        outputCode (T9900Op::imul, reg, n);	// TODO
}

void T9900Generator::codeMove (const std::size_t n) {
    if (n) {
        //
    }
}

void T9900Generator::codeRuntimeCall (const std::string &funcname, const TX64Reg globalDataReg, const std::vector<std::pair<TX64Reg, std::size_t>> &additionalArgs) {
/*
    for (const std::pair<TX64Reg, std::size_t> &arg: additionalArgs)
        outputCode (TX64Op::mov, arg.first, arg.second);
    codeSymbol (globalRuntimeDataSymbol, TX64Reg::rax);
    outputCode (TX64Op::mov, globalDataReg, TX64Operand (TX64Reg::rax, 0));
    outputCode (TX64Op::mov, TX64Reg::rax, funcname);
    outputCode (TX64Op::call, TX64Reg::rax);
*/    
}

void T9900Generator::codePush (const TX64Operand op) {
    outputCode (T9900Op::dect, T9900Reg::r10);
    outputCode (T9900Op::mov, op, T9900Operand (T9900Reg::r10, TAddressingMode::RegInd);
}

void T9900Generator::codePop (const TX64Operand op) {
    outputCode (T9900Op::mov, T9900Operand (T9900Reg::r10, TAddressingMode::RegIndInc), op);
}

void T9900Generator::saveReg (const TX64Reg reg) {
    if (intStackCount < intStackRegs) {
        if (reg != intStackReg [intStackCount])
            outputCode (TX64Op::mov, reg, intStackReg [intStackCount]);
    } else
        codePush (reg);
    ++intStackCount;
}

void T9900Generator::loadReg (const TX64Reg reg) {
    --intStackCount;
    if (intStackCount >= intStackRegs) 
        codePop (reg);
    else
        outputCode (TX64Op::mov, intStackReg [intStackCount], reg);
}

T9900Reg TX64Generator::fetchReg (T9900Reg reg) {
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
    if (n <> 0)
        outputCode (T9900Op::ai, TX64Reg::rsp, n);
}

void T9900Generator::generateCode (TVariable &variable) {
    const T9900Reg reg = getSaveReg (intScratchReg1);
    codeSymbol (variable.getSymbol (), reg);
    saveReg (reg);
}

void T9900Generator::generateCode (TReferenceVariable &referenceVariable) {
    const T9900Reg reg = getSaveReg (intScratchReg1);
    const TSymbol *s = referenceVariable.getSymbol ();
    codeSymbol (referenceVariable.getSymbol (), reg);
    outputCode (TX64Op::mov, T9900Operand (reg, TAddressingMode::RegInd), reg);
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
            codeLoadMemory (type, reg, T9900Operand (reg, 0));
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
        /*
        visit (base);
        visit (index);
        loadReg (TX64Reg::rsi);
        loadReg (TX64Reg::rdi);
        outputCode (TX64Op::mov, TX64Reg::rax, std::string ("rt_str_index"));
        outputCode (TX64Op::call, TX64Reg::rax);
        saveReg (TX64Reg::rax);
        */
    } else {
        visit (base);
        if (type->isPointer ()) {
            // TODO: unify with TPointerDereference
            const T9900Reg reg = fetchReg (intScratchReg1);
            outputCode (TX64Op::mov, TX64Operand (reg, TAddressingMode::RegInd), reg);
            saveReg (reg);
        } 
            
        visit (index);
        const TX64Reg indexReg = fetchReg (intScratchReg1);
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
        outputCode (TX64Op::mov, TX64Operand (reg, TAddressingMode::RegInd), reg);
        saveReg (reg);
    }
}

void T9900Generator::codeIncDec (TPredefinedRoutine &predefinedRoutine) {
    bool isIncOp = predefinedRoutine.getRoutine () == TPredefinedRoutine::Inc;
    const std::vector<TExpressionBase *> &arguments = predefinedRoutine.getArguments ();
    
    TType *type = arguments [0]->getType ();
    const TX64OpSize opSize = getX64OpSize (getMemoryOperationType (type));
    const std::size_t n = type->isPointer () ? static_cast<const TPointerType *> (type)->getBaseType ()->getSize () : 1;
    
    
    TX64Operand value;	// TODO: init to -1/1?
    
    TX64Operand varAddr;
    visit (arguments [0]);
    
    if (arguments.size () > 1) {
        visit (arguments [1]);
        const T9900Reg reg = fetchReg (intScratchReg1);
        codeMultiplyConst (reg, n);
        value = T9900Operand (reg)
    } else { // if (n > 1)
        outputCode (T9900Op::li, intScratchReg1, n);
        value = T9900Operand (intScratchReg1);

    T9900Operand varAddr (fetchReg (intScratchReg2), TAddressingMode::RegInd);
          
    if (value.isValid ())
        outputCode (isIncOp ? T9900Op::a : T9900Op::s, value, varAddr);
//    else	// optimize: INCT/DECT
//        outputCode (TX64Op::add, varAddr, isIncOp ? 1 : - 1);
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
        TX64Reg reg;
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
        TX64Operand operand = TX64Operand (fetchReg (intScratchReg1), 0);
        if (st == &stdType.Real || st == &stdType.Single)
        /*
            codeStoreMemory (st, operand, fetchXmmReg (xmmScratchReg1));
        */            
        else
            codeStoreMemory (st, operand, fetchReg (intScratchReg1));
    } else {
    /*
        TTypeAnyManager typeAnyManager = lookupAnyManager (type);
        loadReg (TX64Reg::rdi);
        loadReg (TX64Reg::rsi);
        if (typeAnyManager.anyManager)
            codeRuntimeCall ("rt_copy_mem", TX64Reg::r9, {{TX64Reg::rdx, type->getSize ()}, {TX64Reg::rcx, typeAnyManager.runtimeIndex}, {TX64Reg::r8, 1}});
        else 
            codeMove (type->getSize ());
    */            
    }
}

void TX64Generator::generateCode (TRoutineCall &routineCall) {
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
/*
    const int maxCaseJumpList = 128;

    TX64Reg reg;
    TExpressionBase *expr = caseStatement.getExpression (), *base = expr;
    if (base->isTypeCast ())
        base = static_cast<TTypeCast *> (base)->getExpression ();
    if (base->isLValueDereference ()) 
        base = static_cast<TLValueDereference *> (base)->getLValue ();
    if (base->isSymbol () && !base->isReference () && static_cast<TVariable *> (base)->getSymbol ()->getRegister () != TSymbol::InvalidRegister) {
         reg = TX64Reg::rax;
         outputCode (TX64Op::mov, reg, static_cast<TX64Reg> (static_cast<TVariable *> (base)->getSymbol ()->getRegister ()));
    } else {
        visit (expr);
        reg = fetchReg (intScratchReg2);
    }
    
    const TCaseStatement::TCaseList &caseList = caseStatement.getCaseList ();
    const std::int64_t minLabel = caseStatement.getMinLabel (), maxLabel = caseStatement.getMaxLabel ();
    std::string endLabel = getNextLocalLabel ();
    
    if (maxLabel - minLabel >= maxCaseJumpList) {
        std::int64_t last = 0;
        outputComment ("no-opt");
        for (const TCaseStatement::TSortedEntry &e: caseStatement.getSortedLabels ()) {
            if (e.label.a != last)
                outputCode (TX64Op::sub, reg, e.label.a - last);
            else
                outputCode (TX64Op::test, reg, reg);
            last = e.label.a;
            if (e.label.a == e.label.b)
                outputCode (TX64Op::je, e.jumpLabel->getOverloadName ());
            else {
                outputCode (TX64Op::cmp, reg, e.label.b - last);
                outputCode (TX64Op::jbe, e.jumpLabel->getOverloadName ());
            }
        }
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            visit (defaultStatement);
        }
        outputCode (TX64Op::jmp, endLabel);

    } else {
        const std::string evalTableLabel = getNextCaseLabel ();
        std::string defaultLabel = endLabel;
        if (minLabel) 
            outputCode (TX64Op::sub, reg, minLabel);
        outputComment ("no-opt");
        outputCode (TX64Op::cmp, reg, maxLabel - minLabel);
        
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            outputCode (TX64Op::jbe, evalTableLabel);
            defaultLabel = getNextCaseLabel ();
            outputLabel (defaultLabel);
            visit (defaultStatement);
            outputCode (TX64Op::jmp, endLabel);
        }  else
            outputCode (TX64Op::ja, endLabel);
        
        std::vector<std::string> jumpTable (maxLabel - minLabel + 1);
        for (const TCaseStatement::TCase &c: caseList) {
            for (const TCaseStatement::TLabel &label: c.labels)
                for (std::int64_t n = label.a; n <= label.b; ++n)
                    jumpTable [n - minLabel] = c.jumpLabel->getOverloadName ();
        }
        outputLabel (evalTableLabel);
        const std::string tableLabel = getNextLocalLabel ();
        outputCode (TX64Op::lea, intScratchReg1, TX64Operand (tableLabel, true));
        outputCode (TX64Op::add, intScratchReg1, TX64Operand (intScratchReg1, reg, 8, 0));
        outputCode (TX64Op::jmp, intScratchReg1);

        registerLocalJumpTable (tableLabel, defaultLabel, std::move (jumpTable));
    }
    
    for (const TCaseStatement::TCase &c: caseList) {
        outputLabel (c.jumpLabel->getOverloadName ());
        visit (c.statement);
        outputCode (TX64Op::jmp, endLabel);
    }    
    outputLabel (endLabel);
*/    
}

void T9900Generator::generateCode (TStatementSequence &statementSequence) {
    for (TStatement *statement: statementSequence.getStatements ())
        visit (statement);
}

void T9900Generator::outputCompare (const TX64Operand &var, const std::int64_t n) {
    outputCode (T9900Op::ci, var, n);
}

void T99004Generator::generateCode (TLabeledStatement &labeledStatement) {
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
    
void T9900Generator::externalRoutine (TSymbol &s) {
/*
    void *lib;
    std::unordered_map<std::string, void *>::iterator it = openLibs.find (s.getExtLibName ());
    if (it == openLibs.end ())
        lib = openLibs [s.getExtLibName ()] = dlopen (s.getExtLibName ().c_str (), RTLD_NOW);
    else
        lib = it->second;
    
    void *f = dlsym (lib, s.getExtSymbolName ().c_str ());
    if (f)
        s.setOffset (reinterpret_cast<std::int64_t> (f));
    else
        std::cerr << "Symbol not found: lib " << s.getExtLibName () << ", sym " << s.getExtSymbolName () << std::endl;
    outputLabelDefinition (s.getExtSymbolName (), reinterpret_cast<std::uint64_t> (f));
*/    
}

void T9900Generator::beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &symbolList, const std::set<TX64Reg> &saveRegs, bool hasStackFrame) {
    if (level > 1) {    
        outputComment (std::string ());
        for (const std::string &s: createSymbolList (routineName, level, symbolList, {}))
            outputComment (s);
        outputComment (std::string ());
    }
    
    // TODO: resolve overload !
    outputLabel (routineName);
    
    if (hasStackFrame) {    
        codePush (T9900Reg::r11);
        codePush (T9900Reg::r9);
        if (level > 2)
            outputCode (T9900Op::mov, TX64Reg::rbp, intScratchReg1);
        outputCode (T99004Op::mov, TX64Reg::r10, TX64Reg::r9);
        
        std::size_t offsetRequired = level > 1 ? symbolList.getLocalSize () : 0;
        if (level > 1) {
            for (std::size_t i = 1; i < level - 1; ++i)
                codePush (T9900Operand (intScratchReg1, -2 * i));
            codePush (T9900Reg::r9);
            if (symbolList.getLocalSize ())
                outputCode (T9900Op::ai, T9900Reg::r10, -symbolList.getLocalSize ());
        }
    }
    
    for (T9900Reg reg: saveRegs)
        codePush (reg);
}

void TX64Generator::endRoutineBody (std::size_t level, TSymbolList &symbolList, const std::set<TX64Reg> &saveRegs, bool hasStackFrame) {
    for (std::set<TX64Reg>::reverse_iterator it = saveRegs.rbegin (); it != saveRegs.rend (); ++it)
        outputCode (TX64Op::pop, *it);

    if (hasStackFrame) {
        outputCode (T9900Op::mov, T9900Reg::r9, T9900Reg::r10);
        codePop (T9900Reg::r9);
        codePop (T9900Reg::r11)
    }
    outputCode (T9900Op::b, T9900Operand (T9900Reg::r11, TAddressingMode::RegInd,);
}

TCodeGenerator::TParameterLocation T9900Generator::classifyType (const TType *type) {
    if (type->isEnumerated () || type->isPointer () || type->isReference () || type->isRoutine ()) 
        return TParameterLocation::IntReg;
    if (type == &stdType.Real || type == &stdType.Single)
        return TParameterLocation::FloatReg;
    else
        return TParameterLocation::Stack;
}

TCodeGenerator::TReturnLocation TX64Generator::classifyReturnType (const TType *type) {
//    if (type->isEnumerated () || type->isPointer () || type->isRoutine () || type == &stdType.Real || type == &stdType.Single)
//        return TReturnLocation::Register;
//    else
        return TReturnLocation::Reference;
}

void T9900Generator::assignParameterOffsets (ssize_t &pos, TBlock &block, std::vector<TSymbol *> &registerParameters) {
    TSymbolList &symbolList = block.getSymbols ();
    
    std::size_t valueParaOffset = 4;
    for (TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Parameter)) {
            TType *type = s->getType ();
            alignType (type);
            s->setOffset (valueParaOffset);
            valueParaOffset += ((type->getSize () + 1) & ~1);
        }
}

void T9900Generator::assignStackOffsets (TBlock &block) {
    TSymbolList &symbolList = block.getSymbols ();
    
    const std::size_t alignment = 1,
                      level = symbolList.getLevel (),
                      displaySize = level * 2;
    ssize_t pos = 0;
    
    inherited::assignStackOffsets (pos, symbolList, false);
    assignParameterOffsets (pos, block, registerParameters);
    pos = (pos + alignment) & ~alignment;
    
    for (TSymbol *s: registerParameters)
        s->setOffset (s->getOffset () - pos - displaySize + sizeof (void *));
    for (TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Alias))
            s->setAliasData ();
        else if (s->checkSymbolFlag (TSymbol::Variable) && s->getRegister () == TSymbol::InvalidRegister)
            s->setOffset (s->getOffset () - pos - displaySize + sizeof (void *));

    symbolList.setParameterSize (0);
    symbolList.setLocalSize (pos);
}

void TX64Generator::assignRegisters (TSymbolList &blockSymbols) {
    std::set<TX64Reg> xmmReg, intReg;
    for (std::size_t i = 0; i < xmmParaRegs; ++i)
        xmmReg.insert (xmmParaReg [i]);
    for (std::size_t i = 0; i < intParaRegs; ++i)
        if (!isRegUsed (intParaReg [i]))
            intReg.insert (intParaReg [i]);
        
    std::size_t xmmParaCount = 0, intParaCount = 0;
    for (TSymbol *s: blockSymbols)
        if (s->checkSymbolFlag (TSymbol::Parameter)) {
            if (s->getType ()->isReal () && xmmParaCount < xmmParaRegs) {
                if (!s->isAliased ()) {
                    s->setRegister (static_cast<std::size_t> (xmmParaReg [xmmParaCount]));
                    xmmReg.erase (xmmParaReg [xmmParaCount]);
                }
                ++xmmParaCount;
            } else if (classifyType (s->getType ()) == TParameterLocation::IntReg && intParaCount < intParaRegs) {
                if (!isRegUsed (intParaReg [intParaCount]) && (!s->isAliased () || s->getType ()->isReference ())) {
                    s->setRegister (static_cast<std::size_t> (intParaReg [intParaCount]));
                    intReg.erase (intParaReg [intParaCount]);
                }
                ++intParaCount;
            }
        }
            
    std::set<TX64Reg>::iterator itXmm = xmmReg.begin (), itInt = intReg.begin ();    
    for (TSymbol *s: blockSymbols)
        if (s->checkSymbolFlag (TSymbol::Variable)) {
            if (s->getType ()->isReal () && !s->isAliased () && itXmm != xmmReg.end ()) {
                s->setRegister (static_cast<std::size_t> (*itXmm));
                itXmm = xmmReg.erase (itXmm);
            } else if (classifyType (s->getType ()) == TParameterLocation::IntReg && !s->isAliased () && itInt != intReg.end ()) {
                s->setRegister (static_cast<std::size_t> (*itInt));
                itInt = intReg.erase (itInt);
            }
        }
}

void TX64Generator::codeBlock (TBlock &block, bool hasStackFrame, TCodeSequence &blockStatements) {
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
        assignGlobals (blockSymbols);
        initStaticGlobals (blockSymbols);
    }
    
    setOutput (&blockCode);
    
    if (!globalInits.empty ()) 
        outputCode (TX64Op::call, TX64Operand ("$init_static"));
    visit (block.getStatements ());
    
    outputLabel (endOfRoutineLabel);
    if (level > 1) {
        if (TAnyManager *anyManager = buildAnyManager (blockSymbols)) {
            std::size_t index = registerAnyManager (anyManager);
            outputCode (TX64Op::mov, TX64Reg::rdi, TX64Reg::rbp);
            codeRuntimeCall ("rt_destroy_mem", TX64Reg::rdx, {{TX64Reg::rsi, index}});
        }
        if (TExpressionBase *returnLValueDeref = block.returnLValueDeref) {
            visit (returnLValueDeref);
            if (returnLValueDeref->getType () == &stdType.Real || returnLValueDeref->getType () == &stdType.Single) {
                loadXmmReg (TX64Reg::xmm0);
                if (returnLValueDeref->getType () == &stdType.Single)
                    outputCode (TX64Op::cvtsd2ss, TX64Reg::xmm0, TX64Reg::xmm0);
            } else
                loadReg (TX64Reg::rax);
        }
    }
    // TODO: destory global variables !!!!
    
//    logOptimizer = block.getSymbol ()->getOverloadName () == "gettapeinput_$182";
    
    removeUnusedLocalLabels (blockCode);
    optimizePeepHole (blockCode);
    tryLeaveFunctionOptimization (blockCode);
    
    // check which callee saved registers are used after peep hole optimization
    std::set<TX64Reg> saveRegs;
    for (TX64Operation &op: blockCode) {
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
        outputCode (TX64Op::ret);
    }
    
    blockStatements.clear ();    
//    blockStatements.reserve (blockPrologue.size () + blockCode.size () + blockEpilogue.size ());
    std::move (blockPrologue.begin (), blockPrologue.end (), std::back_inserter (blockStatements));
    std::move (blockCode.begin (), blockCode.end (), std::back_inserter (blockStatements));
    std::move (blockEpilogue.begin (), blockEpilogue.end (), std::back_inserter (blockStatements));
    
}

void TX64Generator::generateBlock (TBlock &block) {
//    std::cout << "Entering: " << block.getSymbol ()->getName () << std::endl;

    TSymbolList &blockSymbols = block.getSymbols ();
    TCodeSequence blockStatements;
    
    assignStackOffsets (block);
    clearRegsUsed ();
    codeBlock (block, true, blockStatements);
    if (blockSymbols.getLevel () > 1) {
        bool functionCalled = std::find_if (blockStatements.begin (), blockStatements.end (), [] (const TX64Operation &op) { return op.operation == TX64Op::call; }) != blockStatements.end ();
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
    
    trySingleReplacements (blockStatements);
    std::move (blockStatements.begin (), blockStatements.end (), std::back_inserter (program));
    
    for (TSymbol *s: blockSymbols) 
        if (s->checkSymbolFlag (TSymbol::Routine)) {
            if (s->checkSymbolFlag (TSymbol::External)) 
                externalRoutine (*s);
            else 
                visit (s->getBlock ());
        }

}

bool TX64Generator::is32BitLimit (std::int64_t n) {
    return std::numeric_limits<std::int32_t>::min () <= n && n <= std::numeric_limits<std::int32_t>::max ();
}

} // namespace statpascal

