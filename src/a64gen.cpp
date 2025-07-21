#include "a64gen.hpp"
#include "runtime.hpp"

#include <dlfcn.h>
#include <unistd.h>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <limits>
#include <iostream>
#include <numeric>


namespace statpascal {

namespace {

//std::string toHexString (std::uint64_t n) {
//    std::stringstream ss;
//    ss << std::hex << n;
//    return ss.str ();
//}

// registers used for parameter passing

const std::vector<TA64Reg> 
    intParaReg = {TA64Reg::x0, TA64Reg::x1, TA64Reg::x2, TA64Reg::x3, TA64Reg::x4, TA64Reg::x5, TA64Reg::x6, TA64Reg::x7},
    dblParaReg = {TA64Reg::d0, TA64Reg::d1, TA64Reg::d2, TA64Reg::d3, TA64Reg::d4, TA64Reg::d5, TA64Reg::d6, TA64Reg::d7};
    
// registers used for calculator stack and scratch registers

const std::vector<TA64Reg>
    intStackReg = {TA64Reg::x19, TA64Reg::x20, TA64Reg::x21, TA64Reg::x22, TA64Reg::x23, TA64Reg::x24, TA64Reg::x25, TA64Reg::x26, TA64Reg::x27, TA64Reg::x28},
    dblStackReg = {TA64Reg::d18, TA64Reg::d19, TA64Reg::d20, TA64Reg::d21, TA64Reg::d22, TA64Reg::d23, TA64Reg::d24, TA64Reg::d25, TA64Reg::d26, TA64Reg::d27, TA64Reg::d28, TA64Reg::d29, TA64Reg::d30, TA64Reg::d31};
    
const TA64Reg 
    intScratchReg1 = TA64Reg::x16, 
    intScratchReg2 = TA64Reg::x17,
    intScratchReg3 = TA64Reg::x15,
    
    intTempReg1    = TA64Reg::x9,
    
    dblScratchReg1 = TA64Reg::d16,
    dblScratchReg2 = TA64Reg::d17;

const std::size_t
    intParaRegs = intParaReg.size (),
    dblParaRegs = dblParaReg.size (),
    intStackRegs = intStackReg.size (),
    dblStackRegs = dblStackReg.size ();
    
const std::string 
    globalRuntimeDataName = "__globalruntimedata",
    startOfGlobals = "__startofglobals$";

        
static std::array<TToken, 6> relOps = {TToken::Equal, TToken::GreaterThan, TToken::LessThan, TToken::GreaterEqual, TToken::LessEqual, TToken::NotEqual};

} // namespace

TA64Generator::TA64Generator (TRuntimeData &runtimeData, bool codeRangeCheck, bool createCompilerListing):
  inherited (runtimeData),
  runtimeData (runtimeData),
  currentLevel (0),
  intStackCount (0),
  dblStackCount (0),
  dblConstCount (0) {
    buildImmLogicTable ();
}

void TA64Generator::buildImmLogicTable () {
    // see Stack Overflow 30904718
    for (std::uint8_t size = 2; size <= 64; size <<= 1)
        for (std::uint8_t length = 1; length < size; ++length) {
            std::uint64_t result = ~(0ULL) >> (64 - length);
            for (std::uint8_t e = size; e < 64; e <<= 1)
                result |= result << e;
            for (std::uint8_t rot = 0; rot < size; ++rot) {
                immLogicTable [result] = {size, length, rot};
                result = (result >> 63) | (result << 1);
            }
        }
}

bool TA64Generator::isCalleeSavedReg (const TA64Reg reg) {
    return std::find (intStackReg.begin (), intStackReg.end (), reg) != intStackReg.end ();
}

bool TA64Generator::isCalleeSavedReg (const TA64Operand &op) {
    return op.isReg && isCalleeSavedReg (op.reg);
}

bool TA64Generator::isCalcStackReg (const TA64Reg reg) {
    return isCalleeSavedReg (reg) || reg == intScratchReg1 || reg == intScratchReg2 || reg == intScratchReg3;
}

bool TA64Generator::isDblStackReg (const TA64Reg reg) {
    return std::find (dblStackReg.begin (), dblStackReg.end (), reg) != dblStackReg.end () ||
           reg == dblScratchReg1 || reg == dblScratchReg2;
}

bool TA64Generator::isDblReg (const TA64Operand &op) {
    return op.isReg && op.reg >= TA64Reg::d0 && op.reg <= TA64Reg::d31;
}

bool TA64Generator::isCalcStackReg (const TA64Operand &op) {
    return op.isReg && isCalcStackReg (op.reg);
}

bool TA64Generator::isDblStackReg (const TA64Operand &op) {
    return op.isReg && isDblStackReg (op.reg);
}

bool TA64Generator::isSameReg (const TA64Operand &op1, const TA64Operand &op2) {
    return op1.isReg && op2.isReg && op1.reg == op2.reg;
}

bool TA64Generator::isSameCalcStackReg (const TA64Operand &op1, const TA64Operand &op2) {
   return isCalcStackReg (op1) && isCalcStackReg (op2) && op1.reg == op2.reg;
}

bool TA64Generator::isSameDblStackReg (const TA64Operand &op1, const TA64Operand &op2) {
    return isDblStackReg (op1) && isDblStackReg (op2) && op1.reg == op2.reg;
}

bool TA64Generator::isSameDblReg (const TA64Operand &op1, const TA64Operand &op2) {
    return isDblReg (op1) && isDblReg (op2) && op1.reg == op2.reg;
}

bool TA64Generator::checkCode (const TA64Op op1, const TA64Op op2, const std::vector<TA64Op> &first, const std::vector<TA64Op> &second) {
   return std::find (first.begin (), first.end (), op1) != first.end () &&
           std::find (second.begin (), second.end (), op2) != second.end ();
}

bool TA64Generator::isRegisterIndirectAddress (const TA64Operand &op) {
    return op.isOffset || op.isIndex;
}

bool TA64Generator::isRIPRelative (const TA64Operand &op) {
    return op.isLabel && op.ripRelative;
}

void TA64Generator::modifyReg (TA64Operand &operand, TA64Reg a, TA64Reg b) {
    if (operand.reg == a) operand.reg = b;
    if (operand.base == a) operand.base = b;
    if (operand.index == a) operand.index = b;
}

static bool isImm12 (TA64Operand operand) {
    return operand.isImm && operand.imm > -4096 && operand.imm < 4096;
}

static bool isImm6 (TA64Operand operand) {
    return operand.isImm && operand.imm >= 0 && operand.imm < 64;
}

bool TA64Generator::isImmLogical (TA64Operand operand) {
    return operand.isImm && immLogicTable.find (static_cast<std::uint64_t> (operand.imm)) != immLogicTable.end ();
}

//static bool checkImm9 (TA64Operand operand, ssize_t diff) {
//    return operand.isImm && operand.imm + diff >= -256 && operand.imm + diff <= 256;
//}

static bool isPow2 (const ssize_t n, int8_t &m) {
    m = -1;
    if (n == 0)
        return true;
    for (m = 0; m < 63; ++m)
        if (n == 1ll << m) 
            return true;
    return false;
}

TA64Reg operator++ (TA64Reg &r) {
    return TA64Reg (++reinterpret_cast<unsigned char &> (r));
}

void TA64Generator::tryLeaveFunctionOptimization (std::vector<TA64Operation> &code) {
    std::array<bool, static_cast<std::size_t> (TA64Reg::nrRegs) + 1> regsUsed;
    regsUsed.fill (false);
    bool callUsed = false;
    for (const TA64Operation &op: code) {
        if (op.operation == TA64Op::bl || op.operation == TA64Op::blr)
            callUsed = true;
        for (const TA64Operand &operand: op.operands) {
            regsUsed [static_cast<std::size_t> (operand.reg)] = true;
            regsUsed [static_cast<std::size_t> (operand.base)] = true;
            regsUsed [static_cast<std::size_t> (operand.index)] = true;
        }    
    }
    
    if (!callUsed) {
        std::vector<TA64Reg> r1 (16);
        std::iota (r1.begin (), r1.end (), TA64Reg::x0);
        std::vector<TA64Reg> avail, used;
        for (TA64Reg r: r1)
            if (!regsUsed [static_cast<std::size_t> (r)])
                avail.push_back (r);
        for (TA64Reg r: intStackReg)
            if (regsUsed [static_cast<std::size_t> (r)])
                used.push_back (r);
/*                
        std::cout << std::endl;
        for (std::size_t i = 0; i < std::min (avail.size (), used.size ()); ++i) 
            std::cout << a64RegName [static_cast<std::size_t> (used [i])] << " -> " << a64RegName [static_cast<std::size_t> (avail [i])] << std::endl;
*/
        for (TA64Operation &op: code)
            for (std::size_t i = 0; i < std::min (avail.size (), used.size ()); ++i) 
                for (TA64Operand &operand: op.operands)
                    modifyReg (operand, used [i], avail [i]);
    }
}

void TA64Generator::removeUnusedLocalLabels (std::vector<TA64Operation> &code) {
    std::set<std::string> usedLabels;
    for (TA64Operation &op: code) 
        if (op.operation != TA64Op::def_label)
            for (TA64Operand &operand: op.operands)
                if (operand.isLabel)
                    usedLabels.insert (operand.label);
    std::size_t line = 0;
    while (line < code.size ())
        if (code [line].operation == TA64Op::def_label && code [line].operands [0].label.substr (0, 2) == ".l" &&
            usedLabels.find (code [line].operands [0].label) == usedLabels.end ())
            code.erase (code.begin () + line);
        else
            ++line;
}

void TA64Generator::replacePseudoOpcodes (std::vector<TA64Operation> &code) {
    for (std::vector<TA64Operation>::iterator it = code.begin (); it != code.end (); ++it) {
        TA64Op &op = it->operation;
        std::vector<TA64Operand> &op1 = it->operands;
        
        // li reg, n
        // ->
        // load immediate
        if (op == TA64Op::li) {
            if (ssize_t n = op1 [1].imm) {
                TA64Reg reg = op1 [0].reg;
                bool negative = n < 0;
//                std::uint64_t val = n;
                if (n == -1)
                    *it = TA64Operation (TA64Op::mvn, {reg, TA64Reg::xzr}, std::string ());
                else if ((n < -65535 || n > 65535) && isImmLogical (op1 [1]))
                    *it = TA64Operation (TA64Op::orr, {reg, TA64Reg::xzr, n}, std::string ());
                else {
                    if (n < 0) 
                        *it = TA64Operation (TA64Op::movn, {reg, (~n) & 0xffff}, std::string ());
                    else
                        *it = TA64Operation (TA64Op::movz, {reg, n & 0xffff}, std::string ());
                    for (std::uint8_t shift = 16; shift <= 48; shift += 16) {
                        std::uint16_t part = (n >> shift) & 0xffff;
                        if ((negative && part != 0xffff) || (!negative && part))
                            it = code.insert (it + 1, TA64Operation (TA64Op::movk, {reg, part, shift}, std::string ()));
                    }
                }
            } else
                *it = TA64Operation (TA64Op::mov, {op1 [0].reg, TA64Reg::xzr}, std::string ());
        }
    }
}


namespace {
bool logOptimizer = false;
}

bool TA64Generator::optimizePass1 (TA64Op &op1, TA64Op &op2, TA64Op &op3, std::vector<TA64Operand> &op_1, std::vector<TA64Operand> &op_2, std::vector<TA64Operand> &op_3) {
    if ((op1 == TA64Op::mov || op1 == TA64Op::fmov) && isSameReg (op_1 [0], op_1 [1])) {
        op1 = TA64Op::nop;
        return true;
    }
    
    // add|sub r1, r2, #0
    // ->
    // mov r1, r2
    if ((op1 == TA64Op::add || op1 == TA64Op::sub) && op_1 [2].isImm && !op_1 [2].imm) {
        op1 = TA64Op::mov;
        op_1.pop_back ();
        return true;
    }
                
    // mov|li rstack. r1 | imm
    // mov r2, rstack
    // ->
    // mov|li r2, r1 | imm
    if (checkCode (op1, op2, {TA64Op::mov, TA64Op::li}, {TA64Op::mov}) && isSameCalcStackReg (op_1 [0], op_2 [1])) {
        op_1 [0] = op_2 [0];
        op2 = TA64Op::nop;
        return true;
    }
    
    // mov rstack, r1
    // cmp r2, rstack
    // ->
    // cmp r2, r1
    if (checkCode (op1, op2, {TA64Op::mov}, {TA64Op::cmp}) && op_1 [1].isReg && isSameCalcStackReg (op_1 [0], op_2 [1])) {
        op_2 [1] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // li rstack, imm12
    // cmp r2, rstack
    // ->
    // cmp r2, imm12
    if (checkCode (op1, op2, {TA64Op::li}, {TA64Op::cmp}) && isSameCalcStackReg (op_1 [0], op_2 [1]) && isImm12 (op_1 [1])) {
        op_2 [1] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // li rstack, imm6
    // asl|lsr|lsl r1, r2, rstack
    // ->
    // asl|lsr|lsl r1, r2, imm6
    if (checkCode (op1, op2, {TA64Op::li}, {TA64Op::asr, TA64Op::lsr, TA64Op::lsl}) &&
             isSameCalcStackReg (op_1 [0], op_2 [2]) && isImm6 (op_1 [1])) {
        op_2 [2] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // li rstack, l-imm
    // and|orr|eor r1, r2, rstack
    // ->
    // and|orr|eor r1, r2, l-imm
    if (checkCode (op1, op2, {TA64Op::li}, {TA64Op::and_, TA64Op::orr, TA64Op::eor}) &&
             isSameCalcStackReg (op_1 [0], op_2 [2]) && isImmLogical (op_1 [1])) {
        op_2 [2] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // li rstack, imm1
    // add r1, rstack, imm2
    // ->
    // li r1, imm1 + imm2
    if (checkCode (op1, op2, {TA64Op::li}, {TA64Op::add}) && isSameCalcStackReg (op_1 [0], op_2 [1]) && op_2 [2].isImm) {
        op_1 [0] = op_2 [0]; 
        op_1 [1].imm += op_2 [2].imm;
        op2 = TA64Op::nop;
        return true;
    }
    
    // li rstack1, imm1
    // li rstack2, imm2
    // mul r1, rstack1, rstack2
    // ->
    // li r1, imm1 * imm2
    if (checkCode (op1, op2, {TA64Op::li}, {TA64Op::li}) && isCalcStackReg (op_1 [0]) && isCalcStackReg (op_2 [0]) && 
            op3 == TA64Op::mul && ((op_3 [1].reg == op_1 [0].reg && op_3 [2].reg == op_2 [0].reg) || (op_3 [1].reg == op_2 [0].reg && op_3 [2].reg == op_1 [0].reg))) {
        op_1 [0] = op_3 [0];
        op_1 [1].imm *= op_2 [1].imm;
        op2 = TA64Op::nop;
        op3 = TA64Op::nop;
        return true;
    }
    
    // mov rstack, r1
    // add|sub|mul|udiv|sdiv|asr|lsr|lsl|and|orr|eor r2, rstack, val
    // add|sub|mul|udiv|sdiv|asr|lsr|lsl|and|orr|eor r2, r3, rstack
    // ->
    // remove usage of rstack
    else if (checkCode (op1, op2, {TA64Op::mov}, {TA64Op::add, TA64Op::sub, TA64Op::mul, TA64Op::udiv, TA64Op::sdiv, TA64Op::asr, TA64Op::lsr, TA64Op::lsl, TA64Op::and_, TA64Op::orr, TA64Op::eor}) &&
             isCalcStackReg (op_1 [0]) && op_1 [1].isReg && (isSameReg (op_2 [1], op_1 [0]) || isSameReg (op_2 [2], op_1 [0]))) {
        if (isSameReg (op_2 [1], op_1 [0]))
            op_2 [1] = op_1 [1];
        if (isSameReg (op_2 [2], op_1 [0]))
            op_2 [2].reg = op_1 [1].reg;	// keep shift if present
        op1 = TA64Op::nop;
        return true;
    }
    
    // add|sub|mul|udiv|sdiv|asr|lsr|lsl|and|orr|eor|madd rstack, r1, r2 [, r4]
    // mov r3, rstack
    // ->
    // remove usage of rstack
    if (checkCode (op1, op2, {TA64Op::add, TA64Op::sub, TA64Op::mul, TA64Op::udiv, TA64Op::sdiv, TA64Op::asr, TA64Op::lsr, TA64Op::lsl, TA64Op::and_, TA64Op::orr, TA64Op::eor, TA64Op::madd}, {TA64Op::mov}) &&
             isSameCalcStackReg (op_1 [0], op_2 [1])) {
        op_1 [0] = op_2 [0];
        op2 = TA64Op::nop;
        return true;
    }
    
    // mul rstack, r1, r2
    // add r3, r4, rstack | add r3, rstack, r4
    if (checkCode (op1, op2, {TA64Op::mul}, {TA64Op::add}) && ((isSameCalcStackReg (op_1 [0], op_2 [1]) && op_2 [2].isReg) || (isSameCalcStackReg (op_1 [0], op_2 [2]) && op_2 [1].isReg)) && !op_2 [2].offset) {
        op2 = TA64Op::madd;
        op1 = TA64Op::nop;
        if (isSameCalcStackReg (op_1 [0], op_2 [1]))
            op_2 = {op_2 [0], op_1 [1], op_1 [2], op_2 [2]};
        else
            op_2 = {op_2 [0], op_1 [1], op_1 [2], op_2 [1]};
        return true;
    }
    
    // lsl rstack, rstack, imm
    // add r1, r2, rstack
    // ->
    // add r1, r2, rstack, lsl imm
    if (checkCode (op1, op2, {TA64Op::lsl}, {TA64Op::add}) && op_1 [2].isImm && isSameCalcStackReg (op_1 [0], op_2 [2]) && op_1 [0].reg == op_1 [1].reg && op_2 [1].reg != op_2 [2].reg) {
        op_2 [2].offset = op_1 [2].imm;
        op1 = TA64Op::nop;
        return true;
    }
    
    // mov rstack, 2^n	mov rstack, 2^n+1
    // mul r1, r2, rstack        
    // ->
    // lsl r1, r2, #n	add r1, r2, r2, #lsl #n
    // pass 2: due to duplication of regs 
    if (checkCode (op1, op2, {TA64Op::li}, {TA64Op::mul}) && isSameCalcStackReg (op_1 [0], op_2 [2])) {
        std::int8_t n;
        if (isPow2 (op_1 [1].imm, n)) {
            if (n <= 0) {
                op2 = TA64Op::mov;
                if (n < 0)
                    op_2 [1] = TA64Reg::xzr;
                op_2.pop_back ();
            } else {
                op2 = TA64Op::lsl;
                op_2 [2] = n;
            }
            op1 = TA64Op::nop;
            return true;
        } else if (isPow2 (op_1 [1].imm - 1, n)) {
            op2 = TA64Op::add;
            op_2 [2] = op_2 [1];
            op_2 [2].offset = n;
            op1 = TA64Op::nop;
            return true;
        } 
    }
    
    // mov rstack, r1
    // cmp rstack, r2
    // ->
    // cmp r1, r2
    if (checkCode (op1, op2, {TA64Op::mov}, {TA64Op::cmp}) && op_1 [1].isReg && isSameCalcStackReg (op_1 [0], op_2 [0])) {
        op_2 [0] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // add|sub|and r1, r2, val
    // cmp r1, #0
    // ->
    // adds|subs|ands r1, r2, val
/*    

    fails if followed by "bls" - casetest3.sp

    if (checkCode (op1, op2, {TA64Op::add, TA64Op::sub, TA64Op::and_}, {TA64Op::cmp}) && isSameReg (op_1 [0], op_2 [0]) && op_2 [1].isImm && !op_2 [1].imm) {
        if (op1 == TA64Op::add) 
            op1 = TA64Op::adds;
        else if (op1 == TA64Op::sub)
            op1 = TA64Op::subs;
        else
            op1 = TA64Op::ands;
        op2 = TA64Op::nop;
        return true;
    }
*/
    
    // ands r1, r2, val
    // cset r1, cond
    // ->
    // ands xzr, r2, val
    // cset r1, cond
    if (checkCode (op1, op2, {TA64Op::ands}, {TA64Op::cset}) && isSameReg (op_1 [0], op_2 [0])) {
        op_1 [0] = TA64Reg::xzr;
        return true;
    }
        
    // mov rstack, r1
    // cbz|cbnz rstack, label
    // ->
    // cbz|cbnz r1, label
    if (checkCode (op1, op2, {TA64Op::mov}, {TA64Op::cbz, TA64Op::cbnz}) && op_1 [1].isReg && isSameCalcStackReg (op_1 [0], op_2 [0])) {
        op_2 [0] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // cmp reg, #0
    // b.ne label	b.eq label
    // ->
    // cbnz reg, label	cbz reg, label
    if (checkCode (op1, op2, {TA64Op::cmp}, {TA64Op::bne, TA64Op::beq}) && op_1 [1].isImm && !op_1 [1].imm) {
        op1 = TA64Op::nop;
        op2 = op2 == TA64Op::bne ? TA64Op::cbnz : TA64Op::cbz;
        op_2 = {op_1 [0], op_2 [0]};
        return true;
    }
    
    // mov rstack, val
    // mvn r1, rstack
    // -> 
    // mvn r1, val
    if (checkCode (op1, op2, {TA64Op::mov}, {TA64Op::mvn}) && isSameCalcStackReg (op_1 [0], op_2 [1])) {
        op_2 [1] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }

    // mov rstack, imm12
    // add|sub r1, r2, rstack   add r1, rstack, r2
    // ->
    // add|sub r1, r2, imm12
    if (checkCode (op1, op2, {TA64Op::li}, {TA64Op::add, TA64Op::sub}) && isImm12 (op_1 [1]) && ((isSameCalcStackReg (op_1 [0], op_2 [1]) && op2 == TA64Op::add) || isSameCalcStackReg (op_1 [0], op_2 [2])) && !op_2 [2].isImm && !op_2 [2].offset) {
        if (op_2 [1].reg == op_1 [0].reg)
            op_2 [1] = op_2 [2];
        op_2 [2] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }

    // fmov rstack, r1
    // fmov r2, rstack
    // ->
    // fmov r2, r1
    if (checkCode (op1, op2, {TA64Op::fmov}, {TA64Op::fmov}) && isSameDblStackReg (op_1 [0], op_2 [1])) {
        op_2 [1] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // fadd|fsub|fdiv|fmul rstack, r1, r2
    // fmov r3, rstack
    // ->
    // remove usage of rstack
    if (checkCode (op1, op2, {TA64Op::fadd, TA64Op::fsub, TA64Op::fmul, TA64Op::fdiv}, {TA64Op::fmov}) && isSameDblStackReg (op_1 [0], op_2 [1])) {
        op_1 [0] = op_2 [0];
        op_2 [1] = op_2 [0];        // will be removed by first rule
        op2 = TA64Op::nop;
        return true;
    }
    
    // fmov rstack, r1
    // fadd|fsub|fdiv|fmul r2, rstack, val
    // fadd|fsub|fdiv|fmul r2, r3, rstack
    // ->
    // remove usage of rstack
    if (checkCode (op1, op2, {TA64Op::fmov}, {TA64Op::fadd, TA64Op::fsub, TA64Op::fmul, TA64Op::fdiv}) && (isSameDblStackReg (op_1 [0], op_2 [1]) || isSameDblStackReg (op_1 [0], op_2 [2]))) {
        if (isSameDblStackReg (op_2 [1], op_1 [0]))
           op_2 [1] = op_1 [1];
       else
           op_2 [2] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // mov rstack, r1
    // scvtf r2, rstack
    // ->
    // remove usage of rstack
    if (checkCode (op1, op2, {TA64Op::mov}, {TA64Op::scvtf}) &&  op_1 [1].isReg && isSameCalcStackReg (op_1 [0], op_2 [1])) {
        op_2 [1] = op_1 [1];
        op1 = TA64Op::nop;
        return true;
    }
    
    // adr rstack, s
    // blr rstack
    // ->
    // blr s
    if (checkCode (op1, op2, {TA64Op::adr}, {TA64Op::blr}) && isSameCalcStackReg (op_1 [0], op_2 [0])) {
        op_2 [0] = op_1 [1];
        op2 = TA64Op::bl;
        op1 = TA64Op::nop;
        return true;
    }
    
    // adrp rstack, label
    // add rstack, rstack, :l012:label
    // blr rstack
    // ->
    // bl label
    if (checkCode (op1, op2, {TA64Op::adrp}, {TA64Op::add}) && op3 == TA64Op::blr && op_2 [2].isLabel && isSameCalcStackReg (op_1 [0], op_2 [0]) && isSameCalcStackReg (op_2 [0], op_3 [0])) {
        op3 = TA64Op::bl;
        op_3 [0] = op_1 [1];
        op1 = op2 = TA64Op::nop;
        return true;
    }
    
    // ldr rstack, [addr}
    // mov reg, rstack
    // ->
    // ldr reg, [addr]
    if (checkCode (op1, op2, {TA64Op::ldr, TA64Op::ldrb, TA64Op::ldrsb, TA64Op::ldrh, TA64Op::ldrsh, TA64Op::ldrsw}, {TA64Op::mov}) && isSameCalcStackReg (op_1 [0], op_2 [1])) {
        op_1 [0].reg = op_2 [0].reg;
        op2 = TA64Op::nop;
        return true;
    }
    
    // op r1, r2 [,...]
    // mov r0, r1
    // ret	; yet a comment as placeholder
    // ->
    // op r0, r2 [,...]
    // ret
    if (checkCode (op1, op2, {TA64Op::li, TA64Op::mov, TA64Op::ldr, TA64Op::ldrb, TA64Op::ldrsb, TA64Op::ldrh, TA64Op::ldrsh, TA64Op::ldrsw, TA64Op::cset, TA64Op::rev16, TA64Op::add, TA64Op::sub, TA64Op::mul, TA64Op::udiv, TA64Op::sdiv, TA64Op::asr, TA64Op::lsr, TA64Op::lsl, TA64Op::and_, TA64Op::orr, TA64Op::eor, TA64Op::madd}, {TA64Op::mov}) && op3 == TA64Op::end && isSameReg (op_1 [0], op_2 [1])) {
        op_1 [0].reg = op_2 [0].reg;
        op2 = TA64Op::nop;
        return true;
    }
    
    return false;
}

bool isValidOffset (const TA64Operand &op, const ssize_t opBytes) {
    return op.isImm && op.imm >= -256 && op.imm < (4096 * opBytes);
}

bool TA64Generator::optimizePass2 (TA64Op &op1, TA64Op &op2, TA64Op &op3, std::vector<TA64Operand> &op_1, std::vector<TA64Operand> &op_2, std::vector<TA64Operand> &op_3) {
    using TLoadStoreMap = std::map<TA64Op, std::size_t>;
    static const TLoadStoreMap loadStoreOperations = {
        {TA64Op::ldr, 8}, {TA64Op::ldrb, 1}, {TA64Op::ldrsb, 1}, {TA64Op::ldrh, 2}, {TA64Op::ldrsh, 2}, {TA64Op::ldrsw, 4}, {TA64Op::str, 8}, {TA64Op::strb, 1}, {TA64Op::strh, 2}};
        
    TLoadStoreMap::const_iterator it = loadStoreOperations.find (op2);
    if (it == loadStoreOperations.end () || !op_2 [1].isOffset)
        return false;
        
    const ssize_t opBytes = (it->second == 8 && op_2 [0].opSize == TA64OpSize::bit32) ? 4 : it->second;
        
    // add rstack, r1, r2|imm|:lo12:l
    // ldrxx|strxx r3, [rstack]
    // ->
    // ldr r3, [r1, r2|imm|:lo12:l]
    if (op1 == TA64Op::add && op_2 [1].index == TA64Reg::none && (!op_1 [2].offset || (1 << op_1 [2].offset) == opBytes) &&
            !op_2 [1].offset && op_2 [1].label.empty () && isSameCalcStackReg (op_1 [0].reg, op_2 [1].base)) {
        if (op_1 [2].isReg) {
            op_2 [1].index = op_1 [2].reg;
            op_2 [1].offset = op_1 [2].offset;
        } else if (op_1 [2].isLabel)
            op_2 [1].label = op_1 [2].label;
        else if (isValidOffset (op_1 [2], opBytes))
            op_2 [1].offset = op_1 [2].imm;
        else 
            return false;
        op_2 [1].base = op_1 [1].reg;
        op1 = TA64Op::nop;
        return true;
    }
    
    // lsl rstack, r1, imm
    // ldrxx|strxx r2, [r3, rstack]
    // ->
    // ldrxx|strxx r2, [r3, r1, lsl imm]
    if (op1 == TA64Op::lsl && isSameCalcStackReg (op_1 [0].reg, op_2 [1].index) && !op_2 [1].offset &&
            op_1 [2].isImm && (1 << op_1 [2].imm) == opBytes) {
        op_2 [1].index = op_1 [1].reg;
        op_2 [1].offset = op_1 [2].imm;
        op1 = TA64Op::nop;
        return true;
    }

    // mov rstack, r1
    // ldrxx|strxx r2, [rstack[	- with other options
    // ->
    // ldrxx|strxx r2, [r1]
    if (op1 == TA64Op::mov && op_1 [1].isReg) {
        if (isSameCalcStackReg (op_1 [0].reg, op_2 [1].base)) 
            op_2 [1].base = op_1 [1].reg;
        else if (isSameCalcStackReg (op_1 [0].reg, op_2 [1].index))
            op_2 [1].index = op_1 [1].reg;
        else
            return false;
        op1 = TA64Op::nop;
        return true;
    }
    
    // li rstack, imm8
    // ldrxx|strxx r1, [r2, rstack]
    // ->
    // ldrxx|strxx r1, [r2, imm8]
    if (op1 == TA64Op::li && isValidOffset (op_1 [1], opBytes)) {
        if (isSameCalcStackReg (op_1 [0].reg, op_2 [1].base))
            op_2 [1].base = op_2 [1].index;
        else if (!isSameCalcStackReg (op_1 [0].reg, op_2 [1].index))
            return false;
        op_2 [1].offset = op_1 [1].imm;
        op_2 [1].index = TA64Reg::none;
        op1 = TA64Op::nop;
        return true;
    }
    
    return false;    
}

bool TA64Generator::optimizePass3 (TA64Op &op1, TA64Op &op2, TA64Op &op3, std::vector<TA64Operand> &op_1, std::vector<TA64Operand> &op_2, std::vector<TA64Operand> &op_3) {
    if (op1 == TA64Op::cmp && op_1 [1].isImm && !op_1 [1].imm) {
        op_1 [1] = TA64Reg::xzr;
        return true;
    }
    
    // cset rstack, cond
    // mul r1, r2, rstack
    // ->
    // csel r1, r2, xzr, cond
    if (checkCode (op1, op2, {TA64Op::cset}, {TA64Op::mul}) && isSameCalcStackReg (op_1 [0], op_2 [2])) {
        op1 = TA64Op::csel;
        op_1 = {op_2 [0], op_2 [1], TA64Reg::xzr, op_1 [1]};
        op2 = TA64Op::nop;
        return true;
    }
    
    // cset rstack1, cond
    // li rstack2, imm
    // mul reg, rstack1, rstack2
    // ->
    // li rstack2, imm
    // csel reg, rstack2, xzr, cond
    if (checkCode (op1, op2, {TA64Op::cset}, {TA64Op::li}) && op3 == TA64Op::mul && isSameCalcStackReg (op_1 [0], op_3 [1]) && isSameCalcStackReg (op_2 [0], op_3 [2])) {
        op1 = TA64Op::nop;
        op3 = TA64Op::csel;
        op_3 = {op_3 [0], op_2 [0], TA64Reg::xzr, op_1 [1]};
        return true;
    }
        
    
    return false;
}

void TA64Generator::optimizePeepHole (std::vector<TA64Operation> &code) {
    std::vector<std::string> assembly;
    code.emplace_back (TA64Operation {TA64Op::end, {}, ""});
    code.emplace_back (TA64Operation {TA64Op::end, {}, ""});
    for (std::size_t pass = 0; pass <= 2; ++pass) {
        std::size_t line = 0;
        while (line + 2 < code.size ()) {
            if (logOptimizer) {
                assembly.clear ();
                assembly.push_back ("-----------------");
                for (std::size_t i = 0; i < 3; ++i)
                    code [line + i].appendString (assembly);
                assembly.push_back (" -> ");
            }
            static bool (TA64Generator::*const f [3]) (TA64Op &op1, TA64Op &op2, TA64Op &op3, std::vector<TA64Operand> &op_1, std::vector<TA64Operand> &op_2, std::vector<TA64Operand> &op_3) =
                {&TA64Generator::optimizePass1, &TA64Generator::optimizePass2, &TA64Generator::optimizePass3};
            if ((this->*f [pass]) (code [line].operation, code [line + 1].operation, code [line + 2].operation, 
                                   code [line].operands, code [line + 1].operands, code [line + 2].operands)) {
                for (ssize_t i = 2; i >= 0; --i)
                    if (code [line + i].operation == TA64Op::nop) 
                        code.erase (code.begin () + line + i);
                if (logOptimizer) {
                    for (std::size_t i = 0; i < 2; ++i)
                        code [line + i].appendString (assembly);
                    std::copy (assembly.begin (), assembly.end (), std::ostream_iterator<std::string> (std::cout, "\n"));
                }
                if (line > 0)
                    --line;
                if (line > 0)
                    --line;
            } else
                ++line;
        }
    }
    while (!code.empty () && code.back ().operation == TA64Op::end)
        code.pop_back ();
}

void TA64Generator::getAssemblerCode (std::vector<std::uint8_t> &opcodes, bool generateListing, std::vector<std::string> &code) {
    code.clear ();

/*
    code.push_back (std::string ());
    code.push_back (".balign 4096");        
    code.push_back (".data");
    code.push_back (std::string ());
    if (!globalDefinitions.empty ()) {
        code.push_back (std::string ());
        code.push_back ("// Global variables");
        code.push_back (std::string ());
        code.push_back (".balign 16");
        code.push_back (startOfGlobals + ":");
        for (const TGlobalDefinition &g: globalDefinitions)
            if (g.name == globalRuntimeDataSymbol->getOverloadName ())
                code.push_back (g.name + ": .xword 0x" + toHexString (reinterpret_cast<std::uint64_t> (&runtimeData)));
            else if (g.size)
                code.push_back (g.name + ": .space " + std::to_string (g.size) + ", 0");
            else
                code.push_back (g.name + ":");
    }
*/    

    code.push_back (std::string ());    
    code.push_back (".balign 4096");
        
    code.push_back (".text");
    code.push_back ("_start:");
    code.push_back (".global _start");
//    code.push_back ("        adrp x16, " + startOfGlobals);
    for (const TA64Operation &op: program)
        op.appendString (code);        

    if (!constantDefinitions.empty ()) {
        code.push_back (std::string ());
        code.push_back ("// Double constants");
        code.push_back (std::string ());
        code.push_back (".balign 16");
        std::stringstream ss;
        for (const TConstantDefinition &c: constantDefinitions) {
            union {
                double d;
                std::uint64_t n;
            };
            d = c.val;
            ss.str (std::string ());
            ss << "0x" << std::hex << n << "    // " << d;
            code.push_back (c.label + ": .xword " + ss.str ());
        }
    }
    if (!setDefinitions.empty ()) {
        code.push_back (std::string ());
        code.push_back ("// Set constants");
        code.push_back (std::string ());
        code.push_back (".balign 16");
        std::stringstream ss;
        for (const TSetDefinition &s: setDefinitions) {
            ss.str (std::string ());
            ss << s.label << ": .xword ";
            for (const std::uint64_t &v: s.val)
                ss << "0x" << std::hex << v << ", ";
            std::string l = ss.str ();
            l.pop_back (); l.pop_back ();
            code.push_back (l);
        }    
    }

}

void TA64Generator::setOutput (std::vector<TA64Operation> *output) {
    currentOutput = output;
}

void TA64Generator::outputCode (TA64Operation &&l) {
    currentOutput->emplace_back (std::move (l));
}

void TA64Generator::outputCode (const TA64Op op, std::vector<TA64Operand> &&operands, std::string comment) {
    outputCode (TA64Operation (op, std::move (operands), std::move (comment)));
}

void TA64Generator::outputLabel (const std::string &label) {
    outputCode (TA64Op::def_label, {label});
}

void TA64Generator::outputGlobal (const std::string &name, const std::size_t size) {
    globalDefinitions.push_back ({name, size});
}

void TA64Generator::outputComment (const std::string &s) {
    outputCode (TA64Op::comment, {}, s);
}

void TA64Generator::outputLocalJumpTables () {
    if (!jumpTableDefinitions.empty ()) {
        outputComment ("jump tables for case statements");
        for (const TJumpTable &it: jumpTableDefinitions) {
            outputLabel (it.tableLabel);
            for (const std::string &s: it.jumpLabels)
                outputCode (TA64Op::data_dq, {(s.empty () ? it.defaultLabel : s) + "-" + it.tableLabel});
        }
        jumpTableDefinitions.clear ();
    }
}

std::string TA64Generator::registerConstant (double v) {
    for (TConstantDefinition &c: constantDefinitions)
        if (c.val == v)
            return c.label;
    constantDefinitions.push_back ({"__dbl_cnst_" + std::to_string (dblConstCount++), v});
    return constantDefinitions.back ().label;
}

std::string TA64Generator::registerConstant (const std::array<std::uint64_t, TConfig::setwords> &v) {
    setDefinitions.push_back ({"__set_cnst_" + std::to_string (dblConstCount++), v});
    return setDefinitions.back ().label;
}

void TA64Generator::registerLocalJumpTable (const std::string &tableLabel, const std::string &defaultLabel, std::vector<std::string> &&jumpLabels) {
    jumpTableDefinitions.emplace_back (TJumpTable {tableLabel, defaultLabel, std::move (jumpLabels)});
}

void TA64Generator::outputLabelDefinition (const std::string &label, const std::size_t value) {
    if (std::find_if (labelDefinitions.begin (), labelDefinitions.end (), [&label] (const TLabelDefinition &g) { return g.label == label; }) == labelDefinitions.end ())
        labelDefinitions.push_back ({label, value});
}

void TA64Generator::generateCode (TTypeCast &typeCast) {
    TExpressionBase *expression = typeCast.getExpression ();
    TType *destType = typeCast.getType (),
          *srcType = expression->getType ();
    
    visit (expression);
    if (srcType == &stdType.Int64 && destType == &stdType.Real) {
        const TA64Reg r = getSaveDblReg (dblScratchReg1);
        outputCode (TA64Op::scvtf, {r, fetchReg (intScratchReg1)});
        saveDblReg (r);
    }  else if (srcType == &stdType.String && destType->isPointer ()) {
        loadReg (TA64Reg::x0);
        codeRuntimeCall ("rt_str_index", TA64Reg::none, {{TA64Reg::x1, 1}});
        saveReg (TA64Reg::x0);
    }
}

void TA64Generator::generateCode (TExpression &comparison) {
    outputBinaryOperation (comparison.getOperation (), comparison.getLeftExpression (), comparison.getRightExpression ());
}

void TA64Generator::outputBooleanCheck (TExpressionBase *expr, const std::string &label, bool branchOnFalse) {
    static const std::map<TToken, TA64Op> branchFalseMap = {
        {TToken::Equal, TA64Op::bne}, {TToken::GreaterThan, TA64Op::ble}, {TToken::LessThan, TA64Op::bge}, 
        {TToken::GreaterEqual, TA64Op::blt}, {TToken::LessEqual, TA64Op::bgt}, {TToken::NotEqual, TA64Op::beq}
    };
    static const std::map<TToken, TA64Op> branchTrueMap = {
       {TToken::Equal, TA64Op::beq}, {TToken::GreaterThan, TA64Op::bgt}, {TToken::LessThan, TA64Op::blt}, 
       {TToken::GreaterEqual, TA64Op::bge}, {TToken::LessEqual, TA64Op::ble}, {TToken::NotEqual, TA64Op::bne}
    };
    if (expr->isFunctionCall ()) {
        TFunctionCall *functionCall = static_cast<TFunctionCall *> (expr);
        if (static_cast<TRoutineValue *> (functionCall->getFunction ())->getSymbol ()->getExtSymbolName () == "rt_in_set") {
            visit (functionCall->getArguments () [0]);
            visit (functionCall->getArguments () [1]);
            const TA64Reg r1 = fetchReg (intScratchReg2);
            const TA64Reg r2 = fetchReg (intScratchReg1);
            std::int64_t minval = 0, maxval = std::numeric_limits<std::int64_t>::max ();
            getSetTypeLimit (functionCall->getArguments () [0], minval, maxval);
            if (minval < 0 || maxval >= static_cast<std::int64_t> (TConfig::setLimit)) {
                outputCode (TA64Op::cmp, {r2, 256});
                outputCode (TA64Op::bge, {label});
            }
            outputCode (TA64Op::lsr, {intScratchReg3, r2, 3});
            outputCode (TA64Op::ldrb, {TA64Operand (r1, TA64OpSize::bit32), TA64Operand (r1, intScratchReg3, 0)});
            outputCode (TA64Op::and_, {r2, r2, 7});
            outputCode (TA64Op::lsr, {r1, r1, r2});
            outputCode (branchOnFalse ? TA64Op::tbz : TA64Op::tbnz, {r1, 0, label});
            return;
        }
    }
    
    TExpression *condition = dynamic_cast<TExpression *> (expr);
    if (condition && (condition->getLeftExpression ()->getType ()->isEnumerated () || condition->getLeftExpression ()->getType ()->isPointer ())) {
        visit (condition->getLeftExpression ());
        visit (condition->getRightExpression ());
        const TA64Reg r2 = fetchReg (intScratchReg1);
        const TA64Reg r1 = fetchReg (intScratchReg2);
        outputCode (TA64Op::cmp, {r1, r2});
        outputCode ((branchOnFalse ? branchFalseMap : branchTrueMap).at (condition->getOperation ()), {label});
    } else if (condition && condition->getLeftExpression ()->getType ()->isReal ()) {
        visit (condition->getLeftExpression ());
        visit (condition->getRightExpression ());
        const TA64Reg r2 = fetchDblReg (dblScratchReg1);
        const TA64Reg r1 = fetchDblReg (dblScratchReg2);
        outputCode (TA64Op::fcmp, {r1, r2});
        outputCode ((branchOnFalse ? branchFalseMap : branchTrueMap).at (condition->getOperation ()), {label});
    } else {
        visit (expr);
        outputCode (branchOnFalse ? TA64Op::cbz : TA64Op::cbnz, {fetchReg (intScratchReg1), label});
    }
}

void TA64Generator::outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    visit (left);
    TA64Reg reg = fetchReg (intScratchReg1);
    outputComment ("no-opt");
    outputCode (TA64Op::cmp, {reg, 1});
    const std::string doneLabel = getNextLocalLabel ();
    outputCode (operation == TToken::Or ? TA64Op::beq : TA64Op::bne, {doneLabel});;
    visit (right);
    reg = fetchReg (intScratchReg1);
    outputLabel (doneLabel);
    saveReg (reg);
}

void TA64Generator::outputPointerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    visit (left);
    visit (right);   	
    const std::size_t basesize = std::max<std::size_t> (left->getType ()->getBaseType ()->getSize (), 1);
    if (operation == TToken::Add) {
        const TA64Reg r2 = fetchReg (intScratchReg1);
        codeMultiplyConst (r2, basesize, intScratchReg2);
        const TA64Reg r1 = fetchReg (intScratchReg2);
        outputCode (TA64Op::add, {r1, r1, r2});
        saveReg (r1);
    } else {
        const TA64Reg r2 = fetchReg (intScratchReg1);
        const TA64Reg r1 = fetchReg (intScratchReg2);
        outputCode (TA64Op::sub, {r1, r1, r2});
        for (std::uint8_t i = 1; i < 63; ++i)
            if (basesize == 1ull << i) {
                outputCode (TA64Op::lsr, {r1, r1, i});
                saveReg (r1);
                return;
            }
        loadImmediate (r2, basesize);
        outputCode (TA64Op::udiv, {r1, r1, r2});
        saveReg (r1);
    }
}

void TA64Generator::outputIntegerCmpOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    static const std::map<TToken, TA64Operand::TCondition> cmpCondition = {
        {TToken::Equal, TA64Operand::EQ}, {TToken::GreaterThan, TA64Operand::GT}, {TToken::LessThan, TA64Operand::LT}, 
        {TToken::GreaterEqual, TA64Operand::GE}, {TToken::LessEqual, TA64Operand::LE}, {TToken::NotEqual, TA64Operand::NE}};
        
    bool secondFirst = left->isSymbol () || left->isConstant () || left->isLValueDereference ();
    TA64Reg r1, r2, r3;
    
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
    outputCode (TA64Op::cmp, {r1, r2});
    outputCode (TA64Op::cset, {r3, cmpCondition.at (operation)});
    saveReg (r3);
}

void TA64Generator::outputIntegerOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    static const std::map<TToken, TA64Op> opMap = {
        {{TToken::Add, TA64Op::add}, {TToken::Sub, TA64Op::sub}, {TToken::Or, TA64Op::orr}, 
         {TToken::Xor, TA64Op::eor}, {TToken::Mul, TA64Op::mul}, {TToken::And, TA64Op::and_},
         {TToken::DivInt, TA64Op::sdiv}, {TToken::Shl, TA64Op::lsl}, {TToken::Shr, TA64Op::asr}}
    };
    
    const bool isCommutative = operation == TToken::Add || operation == TToken::Or || operation == TToken::Xor || operation == TToken::Mul || operation == TToken::And;
    if ((left->isConstant () || left->isSymbol () || left->isLValueDereference ()) && isCommutative)
        std::swap (left, right);
     
    visit (left);
    visit (right);   	
    TA64Reg r2 = fetchReg (intScratchReg2),
            r1 = fetchReg (intScratchReg1);
    if (operation == TToken::Mod) {
        outputCode (TA64Op::sdiv, {intScratchReg3, r1, r2});
        outputCode (TA64Op::msub, {r1, intScratchReg3, r2, r1});
    } else
        outputCode (opMap.at (operation), {r1, r1, r2});
    saveReg (r1); 
}

void TA64Generator::outputFloatOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    static const std::map<TToken, TA64Op> dblOperation = {
        {TToken::Add, TA64Op::fadd}, {TToken::Sub, TA64Op::fsub}, {TToken::Mul, TA64Op::fmul}, {TToken::Div, TA64Op::fdiv}};
    // TODO: wie bei Integer-Vergleichen
    static const std::map<TToken, TA64Operand::TCondition> cmpCondition = {
        {TToken::Equal, TA64Operand::EQ}, {TToken::GreaterThan, TA64Operand::GT}, {TToken::LessThan, TA64Operand::LT}, 
        {TToken::GreaterEqual, TA64Operand::GE}, {TToken::LessEqual, TA64Operand::LE}, {TToken::NotEqual, TA64Operand::NE}};
        
    TA64Reg r1, r2;
    
    bool secondFirst = (left->isSymbol () || left->isConstant () || left->isLValueDereference ()) && operation != TToken::Sub && operation != TToken::Div;
    if (secondFirst) {
        visit (right);
        visit (left);
        r1 = fetchDblReg (dblScratchReg1);
        r2 = fetchDblReg (dblScratchReg2);
    } else {
        visit (left);
        visit (right);   	
        r2 = fetchDblReg (dblScratchReg2);
        r1 = fetchDblReg (dblScratchReg1);
    }
    
    if (std::find (std::begin (relOps), std::end (relOps), operation) == std::end (relOps)) {
        if (secondFirst)
            std::swap (r1, r2);
        outputCode (dblOperation.at (operation), {r1, r1, r2});
        saveDblReg (r1);
    } else {
        TA64Reg reg = getSaveReg (intScratchReg1);
        outputCode (TA64Op::fcmp, {r1, r2});
        outputCode (TA64Op::cset, {reg, cmpCondition.at (operation)});
        saveReg (reg);
    }
}

void TA64Generator::outputBinaryOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    const TType *lType = left->getType ();
    
    if (lType == &stdType.Boolean && (operation == TToken::And || operation == TToken::Or)) 
        outputBooleanShortcut (operation, left, right);
    else if (lType->isPointer () && (operation == TToken::Add || operation == TToken::Sub)) 
        outputPointerOperation (operation, left, right);
    else if (lType->isEnumerated () || lType->isPointer ()) 
        if (std::find (std::begin (relOps), std::end (relOps), operation) != std::end (relOps)) 
            outputIntegerCmpOperation (operation, left, right);
        else
           outputIntegerOperation (operation, left,right);
    else
        outputFloatOperation (operation, left, right);
}    

void TA64Generator::generateCode (TPrefixedExpression &prefixed) {
    const TType *type = prefixed.getExpression ()->getType ();
    if (type == &stdType.Real) {
        visit (prefixed.getExpression ());
        const TA64Reg r1 = fetchDblReg (dblScratchReg2);
        outputCode (TA64Op::fneg, {r1, r1});
        saveDblReg (r1);
    } else {
        visit (prefixed.getExpression ());
        const TA64Reg reg = fetchReg (intScratchReg1);
        if (prefixed.getOperation () == TToken::Sub)
            outputCode (TA64Op::neg, {reg, reg});
        else if (prefixed.getOperation () == TToken::Not) 
            if (type == &stdType.Boolean)
                outputCode (TA64Op::eor, {reg, reg, 1});
            else
                outputCode (TA64Op::mvn, {reg, reg});
        else {
            std::cout << "Internal Error: invalid prefixed expression" << std::endl;
            exit (1);
        }
        saveReg (reg);
    }
}

void TA64Generator::generateCode (TSimpleExpression &expression) {
    outputBinaryOperation (expression.getOperation (), expression.getLeftExpression (), expression.getRightExpression ());
}

void TA64Generator::generateCode (TTerm &term) {
    outputBinaryOperation (term.getOperation (), term.getLeftExpression (), term.getRightExpression ());
}

void TA64Generator::codeInlinedFunction (TFunctionCall &functionCall) {
    TExpressionBase *function = functionCall.getFunction ();
    const std::vector<TExpressionBase *> &args = functionCall.getArguments ();
    const std::string s = static_cast<TRoutineValue *> (function)->getSymbol ()->getExtSymbolName ();

    if (s == "rt_dbl_abs") {
        visit (args [0]);
        const TA64Reg reg = fetchDblReg (dblScratchReg1);
        outputCode (TA64Op::fabs, {reg, reg});
        saveDblReg (reg);
    } else if (s == "sqrt") {
        visit (args [0]);
        const TA64Reg reg = fetchDblReg (dblScratchReg1);
        outputCode (TA64Op::fsqrt, {reg, reg});
        saveDblReg (reg);
    } else if (s == "ntohs" || s == "htons") {
        visit (args [0]);
        const TA64Reg reg = fetchReg (intScratchReg1);
        outputCode (TA64Op::rev16, {reg, reg});
        saveReg (reg);
    } 
/*
      else if (s == "rt_in_set") {
        visit (args [0]); visit (args [1]);
        const TA64Reg r1 = fetchReg (intScratchReg2);
        const TA64Reg r2 = fetchReg (intScratchReg1);
        const std::string l1 = getNextLocalLabel (), l2 = getNextLocalLabel ();
        
        std::int64_t minval = 0, maxval = std::numeric_limits<std::int64_t>::max ();
        getSetTypeLimit (args [0], minval, maxval);
        if (minval < 0 || maxval >= TConfig::setLimit) {
            outputCode (TA64Op::cmp, r2, 256);
            outputCode (TA64Op::jb, l1);
            outputCode (TA64Op::mov, r1, 0);
            outputCode (TA64Op::jmp, l2);
            outputLabel (l1);
        }
        outputCode (TA64Op::bt, TA64Operand (r1, 0), r2);
        outputCode (TA64Op::setc, TA64Operand (r1, TA64OpSize::bit8));
        outputCode (TA64Op::movsx, r1, TA64Operand (r1, TA64OpSize::bit8));
        
        outputLabel (l2);
        saveReg (r1);
    }
*/
}

bool TA64Generator::isFunctionCallInlined (TFunctionCall &functionCall) {
    TExpressionBase *function = functionCall.getFunction ();
    if (function->isRoutine ()) {
        const std::string s = static_cast<TRoutineValue *> (function)->getSymbol ()->getExtSymbolName ();
        return s == "rt_dbl_abs" || s == "sqrt" || s == "ntohs" || s == "htons"; //  || s == "rt_in_set";
    } else
        return false;        
}

void TA64Generator::generateCode (TFunctionCall &functionCall) {
    if (isFunctionCallInlined (functionCall)) {
        codeInlinedFunction (functionCall);
        return;
    }

    TExpressionBase *function = functionCall.getFunction ();
    const std::vector<TExpressionBase *> &args = functionCall.getArguments ();

    struct TParameterDescription {
        TType *type;
        TExpressionBase *actualParameter;
        int intRegNr;
        int dblRegNr;
        ssize_t stackOffset, stackCopyOffset;
        bool isReferenceStackCopy, isInteger, isObjectByValue, evaluateLate;
    };
    std::vector<TParameterDescription> parameterDescriptions;
/*
    for (const TSymbol *s: static_cast<TRoutineType *> (function->getType ())->getParameter ()) {
        std::cout << s->getName () << ": " << s->getType ()->getName () << std::endl;
    }
*/    
    std::size_t intCount = 0, dblCount = 0, stackCount = 0, stackCopyCount = 0;
    bool usesReturnValuePointer = false;
    const TSymbol *functionReturnTempStorage = functionCall.getFunctionReturnTempStorage ();
    TExpressionBase *returnStorage = functionCall.getReturnStorage ();
    if ((returnStorage || functionReturnTempStorage) && classifyReturnType (functionReturnTempStorage->getType ()) == TReturnLocation::Reference) 
        usesReturnValuePointer = true;
    
    /* The actual parameter may be a string or set constant; which is replaced with an integer by the
       code generator. We therefore use the type provided in the routine declaration. 
    */

    const std::size_t savedDblStack = dblStackCount,
                savedIntStack = intStackCount,
                usedDblStackPairs = (std::min (dblStackRegs, dblStackCount) + 1) / 2;
                
    for (std::size_t i = 0; i < usedDblStackPairs; ++i)
        codePush (dblStackReg [2 * i], dblStackReg [2 * i + 1]);
    dblStackCount = 0;
    
    std::vector<TExpressionBase *>::const_iterator it = args.begin ();
    for (const TSymbol *s: static_cast<TRoutineType *> (function->getType ())->getParameter ()) {
        TParameterDescription pd {s->getType (), *it, -1, -1, -1, false, false, false, false};
        TParameterLocation parameterLocation = classifyType (pd.type);
        
        if ((pd.isReferenceStackCopy = parameterLocation == TParameterLocation::Stack)) {
            pd.stackCopyOffset = stackCopyCount;
            stackCopyCount += (pd.type->getSize () + 7) & ~7;
            parameterLocation = TParameterLocation::IntReg;
        }
        
        if (((*it)->isLValue () || parameterLocation == TParameterLocation::IntReg || parameterLocation == TParameterLocation::ObjectByValue) && intCount < intParaRegs)
            pd.intRegNr = intCount++;
        else if (parameterLocation == TParameterLocation::FloatReg && dblCount < dblParaRegs)
            pd.dblRegNr = dblCount++;
        else {
            pd.stackOffset = stackCount;
            pd.isInteger = parameterLocation == TParameterLocation::IntReg;
            pd.isObjectByValue = parameterLocation == TParameterLocation::ObjectByValue;
            stackCount += 8;
        }
        pd.evaluateLate = pd.actualParameter->isConstant () || pd.actualParameter->isSymbol () || 
            (pd.actualParameter->isLValueDereference () && static_cast<TLValueDereference *> (pd.actualParameter)->getLValue ()->isSymbol ());
        parameterDescriptions.push_back (pd);
        ++it;
    }
    
    if (stackCount % 16)
        stackCount += 8;
    if (stackCopyCount % 16)
        stackCopyCount += 8;

/*
    std::cout << "Function call:" << std::endl << "  Stack: " << stackCount << std::endl;
    for (TParameterDescription &pd: parameterDescriptions) {
        std::cout << pd.type->getName () << " -> ";
        if (pd.intRegNr != -1) 
            std::cout << a64RegName [static_cast<int> (intParaReg [pd.intRegNr])];
        if (pd.dblRegNr != -1) 
            std::cout << a64RegName [static_cast<int> (dblParaReg [pd.dblRegNr])];
        if (pd.stackOffset != -1)
            std::cout << "SP + " << pd.stackOffset;
        std::cout << " late: " << pd.evaluateLate;
        std::cout << std::endl;
    }
    std::cout << std::endl;
*/    

    codeModifySP (-stackCount - stackCopyCount);
    
    for (TParameterDescription &pd: parameterDescriptions)
        if (pd.isReferenceStackCopy) {
            visit (pd.actualParameter);
            loadReg (TA64Reg::x1);
            loadImmediate (pd.stackCopyOffset + stackCount);
            outputCode (TA64Op::add, {TA64Reg::x0, TA64Reg::sp, fetchReg (TA64Reg::x1)});
            if (pd.stackOffset != -1) {
                const TA64Operand stackPos (TA64Reg::sp, TA64Reg::none, pd.stackOffset);
                outputCode (TA64Op::str, {TA64Reg::x1, stackPos});
            }
            codeRuntimeCall ("memcpy", TA64Reg::none, {{TA64Reg::x2, pd.type->getSize ()}});
        } else if (pd.stackOffset != -1) {
            visit (pd.actualParameter);
            const TA64Operand stackPos (TA64Reg::sp, TA64Reg::none, pd.stackOffset);
            if (pd.isInteger || pd.isObjectByValue) {
                const TA64Reg reg = fetchReg (intScratchReg1);
//                codeStoreMemory (getMemoryOperationType (pd.type), reg, stackPos);
                outputCode (TA64Op::str, {reg, stackPos});
            } else {
                const TA64Reg reg = fetchDblReg (dblScratchReg1);
                codeStoreMemory (getMemoryOperationType (pd.type), reg, stackPos);
            } 
        }
        
    bool functionIsExpr = !(function->isRoutine ());
    if (functionIsExpr)    
        visit (function);
    
    for (std::size_t i = 0; i < parameterDescriptions.size (); ++i)
        if (parameterDescriptions [i].stackOffset == -1 && !parameterDescriptions [i].evaluateLate)
            visit (parameterDescriptions [i].actualParameter);
        
    for (std::size_t i = parameterDescriptions.size (); i >= 1;) {
        --i;
        if (parameterDescriptions [i].stackOffset == -1 && parameterDescriptions [i].evaluateLate && !parameterDescriptions [i].isReferenceStackCopy) 
            visit (parameterDescriptions [i].actualParameter);
        if (parameterDescriptions [i].intRegNr != -1)
            if (parameterDescriptions [i].isReferenceStackCopy) {
                loadImmediate (parameterDescriptions [i].stackCopyOffset + stackCount);
                outputCode (TA64Op::add, {intParaReg [parameterDescriptions [i].intRegNr], TA64Reg::sp, fetchReg (intScratchReg1)});
            } else 
                loadReg (intParaReg [parameterDescriptions [i].intRegNr]);
        else if (parameterDescriptions [i].dblRegNr != -1) {
            loadDblReg (dblParaReg [parameterDescriptions [i].dblRegNr]);
            if (parameterDescriptions [i].type == &stdType.Single)
                outputCode (TA64Op::fcvt, {TA64Operand (dblParaReg [parameterDescriptions [i].dblRegNr], TA64OpSize::bit32), dblParaReg [parameterDescriptions [i].dblRegNr]});
        }
    }
    if (usesReturnValuePointer) {
        if (returnStorage) {
            visit (returnStorage);
            loadReg (TA64Reg::x8);
        } else
            codeSymbol (functionReturnTempStorage, TA64Reg::x8);
    }
        
    if (!functionIsExpr)
        visit (function);
    const TA64Reg reg = fetchReg (intScratchReg1);
    outputCode (TA64Op::blr, {reg});
    
    codeModifySP (stackCount + stackCopyCount);
    intStackCount = savedIntStack;
    dblStackCount = savedDblStack;
    
    for (std::size_t i = usedDblStackPairs; i > 0; --i)
        codePop (dblStackReg [2 * i - 2], dblStackReg [2 * i - 1]);
    
    if (functionCall.getType () != &stdType.Void && !functionCall.isIgnoreReturn () && !returnStorage) {
        if (usesReturnValuePointer) {
            TA64Reg r = getSaveReg (intScratchReg1);
            codeSymbol (functionReturnTempStorage, r);
            saveReg (r);
        } else {
            TType *st = getMemoryOperationType (functionCall.getType ());
            if (st == &stdType.Single) {
                const TA64Reg reg = getSaveDblReg (dblScratchReg1);
                outputCode (TA64Op::fcvt, {reg, TA64Operand (TA64Reg::d0, TA64OpSize::bit32)});
                saveDblReg (reg);
            } else if (st == &stdType.Real) 
                saveDblReg (TA64Reg::d0);
            else {
                // do: check if extension needed
                saveReg (TA64Reg::x0);
            }
        }
    }
}

void TA64Generator::generateCode (TConstantValue &constant) {
    if (constant.getType ()->isSet ()) {
        const std::string label = registerConstant (constant.getConstant ()->getSetValues ());
        const TA64Reg reg = getSaveReg (intScratchReg1);
        loadLabelAddress (label, reg);
        saveReg (reg);
    } else if (constant.getType () == &stdType.Real) {
        const TA64Reg reg = getSaveDblReg (dblScratchReg1);
        const std::string label = registerConstant (constant.getConstant ()->getDouble ());
        loadLabelAddress (label, intScratchReg1);
        outputCode (TA64Op::ldr, {reg, TA64Operand (intScratchReg1, TA64Reg::none, 0)});
        saveDblReg (reg);
    } else if (constant.getConstant ()->getType () == &stdType.String)
        loadImmediate (registerStringConstant (constant.getConstant ()->getString ()));
    else
        loadImmediate (constant.getConstant ()->getInteger ());
}

void TA64Generator::generateCode (TRoutineValue &routineValue) {
    TA64Reg reg = getSaveReg (intScratchReg1);
    TSymbol *s = routineValue.getSymbol ();
    if (s->checkSymbolFlag (TSymbol::External))
        loadLabelAddress (s->getExtSymbolName (), reg);
    else
        loadLabelAddress (s->getOverloadName (), reg);
    saveReg (reg);
}

void TA64Generator::loadLabelAddress (const std::string &label, const TA64Reg reg) {
    // do: geht max 1 MB ????
    //  outputCode (TA64Op::adr, {reg, label});
    outputCode (TA64Op::adrp, {reg, label});
    outputCode (TA64Op::add, {reg, reg, ":lo12:" + label});
}

void TA64Generator::codeSymbol (const TSymbol *s, const TA64Reg reg) {
    if (s->getLevel () == 1) 
//        loadLabelAddress (s->getOverloadName (), reg);
        loadImmediate (reg, s->getOffset ());
    else if (s->getLevel () == currentLevel) {
        if (s->getRegister () == TSymbol::InvalidRegister) {
            loadImmediate (intScratchReg1, s->getOffset ());
            outputCode (TA64Op::add, {reg, TA64Reg::x29, intScratchReg1}, s->getName ());
        }
    } else {
        outputCode (TA64Op::ldr, {reg, TA64Operand (TA64Reg::x29, TA64Reg::none, -(s->getLevel () - 1) * sizeof (void *))});
        loadImmediate (intScratchReg1, s->getOffset ());
        outputCode (TA64Op::add, {reg, reg, intScratchReg1}, s->getName ());
    }
}

void TA64Generator::codeStoreMemory (TType *t, TA64Reg srcReg, TA64Operand destMem) {
    if (t == &stdType.Uint8 || t == &stdType.Int8)
        outputCode (TA64Op::strb, {TA64Operand (srcReg, TA64OpSize::bit32), destMem});
    else if (t == &stdType.Uint16 || t == &stdType.Int16)
        outputCode (TA64Op::strh, {TA64Operand (srcReg, TA64OpSize::bit32), destMem});
    else if (t == &stdType.Uint32 || t == &stdType.Int32)
        outputCode (TA64Op::str, {TA64Operand (srcReg, TA64OpSize::bit32), destMem}); 
    else if (t == &stdType.Int64 || t == &stdType.Real) 
        outputCode (TA64Op::str, {srcReg, destMem});
    else if (t == &stdType.Single) {
        outputCode (TA64Op::fcvt, {TA64Operand (srcReg, TA64OpSize::bit32), srcReg});
        outputCode (TA64Op::str, {TA64Operand (srcReg, TA64OpSize::bit32), destMem});
    } else {
        std::cout << "Cannot store type: " << t->getName () << std::endl;
        exit (1);
    }
}

void TA64Generator::codeLoadMemory (TType *t, const TA64Reg regDest, const TA64Operand srcMem, bool convertSingle) {
    if (t == &stdType.Uint8)
        outputCode (TA64Op::ldrb, {TA64Operand (regDest, TA64OpSize::bit32), srcMem});
    else if (t == &stdType.Int8)
        outputCode (TA64Op::ldrsb, {regDest, srcMem});
    else if (t == &stdType.Uint16)
        outputCode (TA64Op::ldrh, {TA64Operand (regDest, TA64OpSize::bit32), srcMem});
    else if (t == &stdType.Int16)
        outputCode (TA64Op::ldrsh, {regDest, srcMem});
    else if (t == &stdType.Uint32)
        outputCode (TA64Op::ldr, {TA64Operand (regDest, TA64OpSize::bit32), srcMem}); 
    else if (t == &stdType.Int32) 
        outputCode (TA64Op::ldrsw, {regDest, srcMem});
    else if (t == &stdType.Int64 || t == &stdType.Real)
        outputCode (TA64Op::ldr, {regDest, srcMem});
    else if (t == &stdType.Single) {
        outputCode (TA64Op::ldr, {TA64Operand (regDest, TA64OpSize::bit32), srcMem}); 
        if (convertSingle)
            outputCode (TA64Op::fcvt, {regDest, TA64Operand (regDest, TA64OpSize::bit32)});
    }
}

void TA64Generator::codeMultiplyConst (const TA64Reg reg, const ssize_t n, const TA64Reg scratchReg) {
    loadImmediate (scratchReg, n);
    outputCode (TA64Op::mul, {reg, reg, scratchReg});
}

void TA64Generator::codeRuntimeCall (const std::string &funcname, const TA64Reg globalDataReg, const std::vector<std::pair<TA64Reg, ssize_t>> &additionalArgs) {
    for (const std::pair<TA64Reg, ssize_t> &arg: additionalArgs)
        loadImmediate (arg.first, arg.second);
    if (globalDataReg != TA64Reg::none) {
        codeSymbol (globalRuntimeDataSymbol, intScratchReg1);
        outputCode (TA64Op::ldr, {globalDataReg, TA64Operand (intScratchReg1, TA64Reg::none, 0)});
    }
    outputCode (TA64Op::bl, {funcname});
}

void TA64Generator::codePush (const TA64Reg r1, const TA64Reg r2) {
    outputCode (TA64Op::stp, {r1, r2, TA64Operand (TA64Reg::sp, -16, TA64Operand::PreIndex)});
}

void TA64Generator::codePop (const TA64Reg r1, const TA64Reg r2) {
    outputCode (TA64Op::ldp, {r1, r2, TA64Operand (TA64Reg::sp, 16, TA64Operand::PostIndex)});    
}

void TA64Generator::saveReg (const TA64Reg reg) {
    if (intStackCount < intStackRegs) {
        if (reg != intStackReg [intStackCount])
            outputCode (TA64Op::mov, {intStackReg [intStackCount], reg});
    } else
        codePush (reg);
    ++intStackCount;
}

void TA64Generator::saveDblReg (const TA64Reg reg) {
    if (dblStackCount < dblStackRegs) {
        if (reg != dblStackReg [dblStackCount])
            outputCode (TA64Op::fmov, {dblStackReg [dblStackCount], reg});
    } else 
        codePush (reg);
    ++dblStackCount;
}

void TA64Generator::loadReg (const TA64Reg reg) {
    --intStackCount;
    if (intStackCount >= intStackRegs) 
        codePop (reg);
    else
        outputCode (TA64Op::mov, {reg, intStackReg [intStackCount]});
}

void TA64Generator::loadDblReg (const TA64Reg reg) {
    assert (dblStackCount > 0);
    --dblStackCount;
    if (dblStackCount >= dblStackRegs)
        codePop (reg);
    else
        outputCode (TA64Op::fmov, {reg, dblStackReg [dblStackCount]});
}

TA64Reg TA64Generator::intStackTop () {
    if (intStackCount > intStackRegs || !intStackCount)
        return TA64Reg::none;
    else
        return intStackReg [intStackCount - 1];
}

TA64Reg TA64Generator::fetchReg (TA64Reg reg) {
    --intStackCount;
    if (intStackCount >= intStackRegs) {
        codePop (reg);
        return reg;
    } else
        return intStackReg [intStackCount];
}

TA64Reg TA64Generator::fetchDblReg (TA64Reg reg) {
    assert (dblStackCount > 0);
    --dblStackCount;
    if (dblStackCount >= dblStackRegs) {
        codePop (reg);
        return reg;
    } else
        return dblStackReg [dblStackCount];
}

TA64Reg TA64Generator::getSaveReg (TA64Reg reg) {
    if (intStackCount >= intStackRegs) 
        return reg;
    return intStackReg [intStackCount];
}

TA64Reg TA64Generator::getSaveDblReg (TA64Reg reg) {
    if (dblStackCount >= dblStackRegs)
        return reg;
    else
        return dblStackReg [dblStackCount];
}

void TA64Generator::clearRegsUsed () {
    std::fill (regsUsed.begin (), regsUsed.end (), false);
}

void TA64Generator::setRegUsed (TA64Reg r) {
    regsUsed [static_cast<std::size_t> (r)] = true;
}

bool TA64Generator::isRegUsed (const TA64Reg r) const {
    return regsUsed [static_cast<std::size_t> (r)];
}

void TA64Generator::codeModifySP (ssize_t n) {
    if (n) {
        loadImmediate (intScratchReg1, n);
        outputCode (TA64Op::add, {TA64Reg::sp, TA64Reg::sp, intScratchReg1});
    }
}

void TA64Generator::generateCode (TVariable &variable) {
    const TA64Reg reg = getSaveReg (intScratchReg1);
    codeSymbol (variable.getSymbol (), reg);
    saveReg (reg);
}

void TA64Generator::generateCode (TReferenceVariable &referenceVariable) {
    const TA64Reg reg = getSaveReg (intScratchReg1);
    const TSymbol *s = referenceVariable.getSymbol ();
    if (s->getRegister () != TSymbol::InvalidRegister)
        outputCode (TA64Op::mov, {reg, static_cast<TA64Reg> (s->getRegister ())});
    else {
        codeSymbol (referenceVariable.getSymbol (), reg);
        outputCode (TA64Op::ldr, {reg, TA64Operand (reg, TA64Reg::none, 0)});
    }
    saveReg (reg);
}

void TA64Generator::generateCode (TLValueDereference &lValueDereference) {
    TExpressionBase *lValue = lValueDereference.getLValue ();
    TType *type = getMemoryOperationType (lValueDereference.getType ());
    
    if (lValue->isSymbol ()) {
        const TSymbol *s = static_cast<TVariable *> (lValue)->getSymbol ();
        if (s->getRegister () != TSymbol::InvalidRegister) 
            if (!lValue->isReference ()) {
                if (type == &stdType.Real) {
                    const TA64Reg reg = getSaveDblReg (dblScratchReg1);
                    outputCode (TA64Op::fmov, {reg, static_cast<TA64Reg> (s->getRegister ())});
                    saveDblReg (reg);
                } else {
                    const TA64Reg reg = getSaveReg (intScratchReg1);
                    outputCode (TA64Op::mov, {reg, static_cast<TA64Reg> (s->getRegister ())});
                    saveReg (reg);
                }
                return;
            } else {
                const TA64Reg reg = getSaveReg (intScratchReg1);
                outputCode (TA64Op::mov, {reg, static_cast<TA64Reg> (s->getRegister ())});
                saveReg (reg);
        } else
            visit (lValue);
    } else
        visit (lValue);

    // leave pointer on stack if not scalar type
    
    if (type != &stdType.UnresOverload) {
        if (type == &stdType.Real || type == &stdType.Single) {
            const TA64Reg reg = getSaveDblReg (dblScratchReg1);
            codeLoadMemory (type, reg, TA64Operand (fetchReg (intScratchReg1), TA64Reg::none, 0));
            saveDblReg (reg);
        } else {
            const TA64Reg reg = fetchReg (intScratchReg1);
            codeLoadMemory (type, reg, TA64Operand (reg, TA64Reg::none, 0));
            saveReg (reg);
        }
    }
}

void TA64Generator::generateCode (TArrayIndex &arrayIndex) {
    TExpressionBase *base = arrayIndex.getBaseExpression (),
                    *index = arrayIndex.getIndexExpression ();
    const TType *type = base->getType ();
    const ssize_t minVal = type->isPointer () ? 0 :  static_cast<TEnumeratedType *> (index->getType ())->getMinVal ();
    const std::size_t baseSize = arrayIndex.getType ()->getSize ();
    
    if (index->isTypeCast ())
        index = static_cast<TTypeCast *> (index)->getExpression ();
        
    if (type == &stdType.String) {
        visit (base);
        visit (index);
        loadReg (TA64Reg::x1);
        loadReg (TA64Reg::x0);
        codeRuntimeCall ("rt_str_index", TA64Reg::none, {});
        saveReg (TA64Reg::x0);
    } else {
        visit (base);
        if (type->isPointer ()) {
            // TODO: unify with TPointerDereference
            const TA64Reg reg = fetchReg (intScratchReg1);
            if (base->isSymbol () && !base->isReference () && static_cast<TVariable *> (base)->getSymbol ()->getRegister () != TSymbol::InvalidRegister) 
                outputCode (TA64Op::mov, {reg, static_cast<TA64Reg> (static_cast<TVariable *> (base)->getSymbol ()->getRegister ())});
            else        
                outputCode (TA64Op::ldr, {reg, TA64Operand (reg, TA64Reg::none, 0)});
            saveReg (reg);
        } 
            
        visit (index);
        const TA64Reg indexReg = fetchReg (intScratchReg1);
        if (minVal) {
            loadImmediate (intScratchReg2, -minVal);
            outputCode (TA64Op::add, {indexReg, indexReg, intScratchReg2});
        }
        codeMultiplyConst (indexReg, baseSize, intScratchReg2);
        
        const TA64Reg baseReg = fetchReg (intScratchReg2);
        outputCode (TA64Op::add, {baseReg, baseReg, indexReg});
        saveReg (baseReg);
    }
}

void TA64Generator::generateCode (TRecordComponent &recordComponent) {
    visit (recordComponent.getExpression ());
    TRecordType *recordType = static_cast<TRecordType *> (recordComponent.getExpression ()->getType ());
    if (const TSymbol *symbol = recordType->searchComponent (recordComponent.getComponent ())) {
        if (symbol->getOffset ()) {
            const TA64Reg reg = fetchReg (intScratchReg1);
            loadImmediate (intScratchReg2, symbol->getOffset ());
            outputCode (TA64Op::add, {reg, reg, intScratchReg2}, "." + std::string (symbol->getName ()));
            saveReg (reg);
        }
    } else {
        // INternal Error: Component not found !!!!
    }
}

void TA64Generator::generateCode (TPointerDereference &pointerDereference) {
    TExpressionBase *base = pointerDereference.getExpression ();
    visit (base);
    if (base->isLValue ()) {
        const TA64Reg reg = fetchReg (intScratchReg1);
        if (base->isSymbol () && !base->isReference () && static_cast<TVariable *> (base)->getSymbol ()->getRegister () != TSymbol::InvalidRegister) 
            outputCode (TA64Op::mov, {reg, static_cast<TA64Reg> (static_cast<TVariable *> (base)->getSymbol ()->getRegister ())});
        else        
            outputCode (TA64Op::ldr, {reg, TA64Operand (reg, TA64Reg::none, 0)});
        saveReg (reg);
    }
}

//void TA64Generator::generateCode (TRuntimeRoutine &transformedRoutine) {
//    for (TSyntaxTreeNode *node: transformedRoutine.getTransformedNodes ())
//        visit (node);
//}

void TA64Generator::codeIncDec (TPredefinedRoutine &predefinedRoutine) {
    bool isIncOp = predefinedRoutine.getRoutine () == TPredefinedRoutine::Inc;
    const std::vector<TExpressionBase *> &arguments = predefinedRoutine.getArguments ();
    
    TType *type = arguments [0]->getType (),
          *st = getMemoryOperationType (type);
    
    ssize_t n = type->isPointer () ? static_cast<const TPointerType *> (type)->getBaseType ()->getSize () : 1;
    bool isConstant = true;
    
    if (arguments.size () > 1) {
        isConstant = arguments [1]->isConstant ();
        if (isConstant)
            n *= static_cast<TConstantValue *> (arguments [1])->getConstant ()->getInteger ();
        else {
            visit (arguments [1]);
            const TA64Reg reg = fetchReg (intScratchReg1);
            codeMultiplyConst (reg, n, intScratchReg2);
            saveReg (reg);
        }
    } 

    if (arguments [0]->isSymbol () && !arguments [0]->isReference ()) {
        const TSymbol *s = static_cast<TVariable *> (arguments [0])->getSymbol ();
        if (s->getRegister () != TSymbol::InvalidRegister) {
            if (isConstant)
                loadImmediate (n);
            outputCode (isIncOp ? TA64Op::add : TA64Op::sub, {static_cast<TA64Reg> (s->getRegister ()), static_cast<TA64Reg> (s->getRegister ()), fetchReg (intScratchReg1)});
            return;	// do range check
        }
    }
    
    visit (arguments [0]);
    outputCode (TA64Op::mov, {intTempReg1, fetchReg (intScratchReg2)});
        
    codeLoadMemory (st, intScratchReg3, TA64Operand (intTempReg1, TA64Reg::none, 0));
    if (isConstant)
        loadImmediate (n);
    const TA64Reg r1 = fetchReg (intScratchReg1);
    outputCode (isIncOp ? TA64Op::add : TA64Op::sub, {intScratchReg3, intScratchReg3, r1});
    codeStoreMemory (st, intScratchReg3, TA64Operand (intTempReg1, TA64Reg::none, 0));
    // TODO: range check
}

void TA64Generator::generateCode (TPredefinedRoutine &predefinedRoutine) {
    TPredefinedRoutine::TRoutine predef = predefinedRoutine.getRoutine ();
    if (predef == TPredefinedRoutine::Inc || predef == TPredefinedRoutine::Dec)
        codeIncDec (predefinedRoutine);
    else {
        const std::vector<TExpressionBase *> &arguments = predefinedRoutine.getArguments ();
        for (TExpressionBase *expression: arguments)
            visit (expression);

        bool isIncOp = false;    
        TA64Reg reg;
        switch (predefinedRoutine.getRoutine ()) {
            case TPredefinedRoutine::Odd:
                reg = fetchReg (intScratchReg1);
                outputCode (TA64Op::and_, {reg, reg, 1});
                saveReg (reg);
                break;
            case TPredefinedRoutine::Succ:
                isIncOp = true;
                // fall through
            case TPredefinedRoutine::Pred:
                reg = fetchReg (intScratchReg1);
                outputCode (TA64Op::add, {reg, reg, isIncOp ? 1 : -1});
                saveReg (reg);
                // TODO : range check
                break;
            case TPredefinedRoutine::Exit:
                outputCode (TA64Op::b, {endOfRoutineLabel});
                break;
            default:
                break;
        }
    }
}

void TA64Generator::generateCode (TAssignment &assignment) {
   TExpressionBase *lValue = assignment.getLValue (),
                    *expression = assignment.getExpression ();
    TType *type = lValue->getType (),
          *st = getMemoryOperationType (type);

    visit (expression);
    TA64Reg addrReg = TA64Reg::none;
    if (lValue->isSymbol ()) {
        const TSymbol *s = static_cast<TVariable *> (lValue)->getSymbol ();
        if (s->getRegister () != TSymbol::InvalidRegister) {
            if (lValue->isReference ())
                addrReg = static_cast<TA64Reg> (s->getRegister ());
            else {
                if (st == &stdType.Real)
                    outputCode (TA64Op::fmov, {static_cast<TA64Reg> (s->getRegister ()), fetchDblReg (dblScratchReg1)});
                else
                    outputCode (TA64Op::mov, {static_cast<TA64Reg> (s->getRegister ()), fetchReg (intScratchReg1)});
                return;
            }
        }
    }
    
    if (addrReg == TA64Reg::none)
        visit (lValue);
        
    if (st != &stdType.UnresOverload) {
        // TODO: einfacher
        TA64Operand operand = addrReg == TA64Reg::none ? TA64Operand (fetchReg (intScratchReg1), TA64Reg::none, 0) : TA64Operand (addrReg, TA64Reg::none, 0);
        if (st == &stdType.Real || st == &stdType.Single)
            codeStoreMemory (st, fetchDblReg (dblScratchReg1), operand);
        else
            codeStoreMemory (st, fetchReg (intScratchReg1), operand);
    } else {
        TTypeAnyManager typeAnyManager = lookupAnyManager (type);
        loadReg (TA64Reg::x0);
        loadReg (TA64Reg::x1);
        if (typeAnyManager.anyManager) 
            codeRuntimeCall ("rt_copy_mem", TA64Reg::x5, {{TA64Reg::x2, type->getSize ()}, {TA64Reg::x3, typeAnyManager.runtimeIndex}, {TA64Reg::x4, 1}});
        else 
            codeRuntimeCall ("memcpy", TA64Reg::none, {{TA64Reg::x2, type->getSize ()}});
    }
  
}

void TA64Generator::generateCode (TRoutineCall &routineCall) {
    visit (routineCall.getRoutineCall ());
}

void TA64Generator::generateCode (TIfStatement &ifStatement) {
    const std::string l1 = getNextLocalLabel ();
    outputBooleanCheck (ifStatement.getCondition (), l1);
    visit (ifStatement.getStatement1 ());
    if (TStatement *statement2 = ifStatement.getStatement2 ()) {
        const std::string l2 = getNextLocalLabel ();
        outputCode (TA64Op::b, {l2});
        outputLabel (l1);
        visit (statement2);
        outputLabel (l2);
    } else
        outputLabel (l1);
}

void TA64Generator::generateCode (TCaseStatement &caseStatement) {
    const ssize_t maxCaseJumpList = 128;

    TA64Reg reg;
    TExpressionBase *expr = caseStatement.getExpression (), *base = expr;
    if (base->isTypeCast ())
        base = static_cast<TTypeCast *> (base)->getExpression ();
    if (base->isLValueDereference ()) 
        base = static_cast<TLValueDereference *> (base)->getLValue ();
    if (base->isSymbol () && !base->isReference () && static_cast<TVariable *> (base)->getSymbol ()->getRegister () != TSymbol::InvalidRegister)
         reg = static_cast<TA64Reg> (static_cast<TVariable *> (base)->getSymbol ()->getRegister ());
    else {
        visit (expr);
//        outputCode (TA64Op::mov, {intTempReg1, fetchReg (intScratchReg1)});
        reg = intTempReg1;
        loadReg (reg);
    }
    
    const TCaseStatement::TCaseList &caseList = caseStatement.getCaseList ();
    const std::int64_t minLabel = caseStatement.getMinLabel (), maxLabel = caseStatement.getMaxLabel ();
    std::string endLabel = getNextLocalLabel ();
    std::vector<std::string> caseLabel;
    
    if (maxLabel - minLabel > maxCaseJumpList) {
        for (const TCaseStatement::TCase &c: caseList) {
            caseLabel.push_back (getNextCaseLabel ());
            for (const TCaseStatement::TLabel &label: c.labels) {
                if (label.a == label.b) {
                    loadImmediate (intScratchReg1, label.a);
                    outputCode (TA64Op::cmp, {reg, intScratchReg1});
                    outputCode (TA64Op::beq, {caseLabel.back ()});
                } else {
                    loadImmediate (intScratchReg1, -label.a);
                    outputCode (TA64Op::add, {intScratchReg1, intScratchReg1, reg});
                    loadImmediate (intScratchReg2, label.b - label.a);
                    outputCode (TA64Op::cmp, {intScratchReg1, intScratchReg2});
                    outputCode (TA64Op::bls, {caseLabel.back ()});
                }
            }
        }
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            visit (defaultStatement);
        }
        outputCode (TA64Op::b, {endLabel});
    } else {
        const std::string evalTableLabel = getNextCaseLabel ();
        if (minLabel) {
            loadImmediate (intScratchReg1, minLabel);
            outputCode (TA64Op::sub, {intTempReg1, reg, intScratchReg1});
            reg = intTempReg1;
        }
        outputCode (TA64Op::cmp, {reg, maxLabel - minLabel});
        outputCode (TA64Op::bls, {evalTableLabel});
        
        std::string defaultLabel = endLabel;
        if (TStatement *defaultStatement = caseStatement.getDefaultStatement ()) {
            defaultLabel = getNextCaseLabel ();
            outputLabel (defaultLabel);
            visit (defaultStatement);
        } 
        outputCode (TA64Op::b, {endLabel});
        
        std::vector<std::string> jumpTable (maxLabel - minLabel + 1);
        for (const TCaseStatement::TCase &c: caseList) {
            caseLabel.push_back (getNextCaseLabel ());
            for (const TCaseStatement::TLabel &label: c.labels)
                for (std::int64_t n = label.a; n <= label.b; ++n)
                    jumpTable [n - minLabel] = caseLabel.back ();
        }
        outputLabel (evalTableLabel);
        const std::string tableLabel = getNextLocalLabel ();
        outputCode (TA64Op::adr, {intScratchReg1, tableLabel});
        outputCode (TA64Op::ldr, {intScratchReg2, TA64Operand (intScratchReg1, reg, 3)});
        outputCode (TA64Op::add, {intScratchReg1, intScratchReg1, intScratchReg2});
        outputCode (TA64Op::br, {intScratchReg1});

        registerLocalJumpTable (tableLabel, defaultLabel, std::move (jumpTable));
    }
    
    std::vector<std::string>::iterator it = caseLabel.begin ();
    for (const TCaseStatement::TCase &c: caseList) {
        outputLabel (*it++);
        visit (c.statement);
        outputCode (TA64Op::b, {endLabel});
    }    
    outputLabel (endLabel);
    
}

void TA64Generator::generateCode (TStatementSequence &statementSequence) {
    for (TStatement *statement: statementSequence.getStatements ())
        visit (statement);
}

void TA64Generator::storeVariable (const TSymbol *s) {
    TType *st = getMemoryOperationType (s->getType ());
    if (s->getRegister () != TSymbol::InvalidRegister)
        loadReg (static_cast<TA64Reg> (s->getRegister ()));
    else {
        codeSymbol (s, intScratchReg1);
        codeStoreMemory (st, fetchReg (intScratchReg2), TA64Operand (intScratchReg1, TA64Reg::none, 0));
    }
}

void TA64Generator::loadImmediate (const TA64Reg reg, const ssize_t n) {
    outputCode (TA64Op::li, {reg, n});
}

void TA64Generator::loadImmediate (const ssize_t n) {
    TA64Reg reg = getSaveReg (intScratchReg1);
    loadImmediate (reg, n);
    saveReg (reg);
}

void TA64Generator::loadVariable (const TSymbol *s, bool convertSingle) {
    TType *type = s->getType (),
          *st = getMemoryOperationType (type);
    TA64Reg r = (st == &stdType.Single || st == &stdType.Real) ? getSaveDblReg (dblScratchReg1) : getSaveReg (intScratchReg1);
    if (s->getRegister () != TSymbol::InvalidRegister)
        if (st == &stdType.Real)
            outputCode (TA64Op::fmov, {r, static_cast<TA64Reg> (s->getRegister ())});
        else if (type->isReference ())
            outputCode (TA64Op::mov, {r, TA64Operand (static_cast<TA64Reg> (s->getRegister ()), TA64Reg::none, 0)});
        else
            outputCode (TA64Op::mov, {r, static_cast<TA64Reg> (s->getRegister ())});
    else {
        codeSymbol (s, intScratchReg2);
        if (type->isReference ())
            codeLoadMemory (&stdType.Int64, r, TA64Operand (intScratchReg2, TA64Reg::none, 0), false);
        else
            codeLoadMemory (st, r, TA64Operand (intScratchReg2, TA64Reg::none, 0), convertSingle);
    }
    
    if (st == &stdType.Single || st == &stdType.Real)
        saveDblReg (r);
    else
        saveReg (r);
}

void TA64Generator::generateCode (TLabeledStatement &labeledStatement) {
    outputLabel ("." + labeledStatement.getLabel ()->getName ());
    visit (labeledStatement.getStatement ());
}

void TA64Generator::generateCode (TGotoStatement &gotoStatement) {
    if (TExpressionBase *condition = gotoStatement.getCondition ())
        outputBooleanCheck (condition, "." + gotoStatement.getLabel ()->getName (), false);
    else
        outputCode (TA64Op::b, {"." + gotoStatement.getLabel ()->getName ()});
}
    
void TA64Generator::generateCode (TBlock &block) {
    generateBlock (block);
}

void TA64Generator::generateCode (TUnit &unit) {
}

void TA64Generator::generateCode (TProgram &prog) {
    TSymbolList &globalSymbols = prog.getBlock ()->getSymbols ();
    globalRuntimeDataSymbol = globalSymbols.searchSymbol ("__globalruntimedata");
    
    // TODO: error if not found !!!!
    generateBlock (*prog.getBlock (), prog.getUnits ());

    std::vector<TA64Operation> runtimeCalls;    
    setOutput (&runtimeCalls);
    outputComment ("Runtime library calls");
    for (const TLabelDefinition &l: labelDefinitions) {
        outputLabel (l.label);
        loadImmediate (TA64Reg::x15, l.value);
        outputCode (TA64Op::br, {TA64Reg::x15});
    }
    replacePseudoOpcodes (runtimeCalls);
    program.insert (program.end (), runtimeCalls.begin (), runtimeCalls.end ());
    
//    std::cout << "Data size is: " << globalSymbols.getLocalSize () << std::endl;
    
}
    
void TA64Generator::externalRoutine (TSymbol &s) {
    void *f = dlsym (dlopen (s.getExtLibName ().c_str (), RTLD_NOW), s.getExtSymbolName ().c_str ());
    if (f)
        s.setOffset (reinterpret_cast<std::int64_t> (f));
    outputLabelDefinition (s.getExtSymbolName (), reinterpret_cast<std::uint64_t> (f));
}

void TA64Generator::beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &symbolList, const std::vector<TA64Reg> &saveRegs, bool hasStackFrame) {
    if (level > 1) {    
        outputComment (std::string ());
        for (const std::string &s: createSymbolList (routineName, level, symbolList, a64RegName))
            outputComment (s);
        outputComment (std::string ());
    }
    
    // TODO: resolve overload !
//    outputCode (TA64Op::aligncode);
    outputLabel (routineName);
    
    if (hasStackFrame) {    
        codePush (TA64Reg::x29, TA64Reg::x30);
        if (level > 2)
            outputCode (TA64Op::mov, {intScratchReg3, TA64Reg::x29});
        outputCode (TA64Op::mov, {TA64Reg::x29, TA64Reg::sp});

        std::size_t stackSize = level > 1 ? 
            (((symbolList.getLocalSize () + 8 * (level - 1)) + 15) & ~15) : 0;
        codeModifySP (-stackSize);

        if (level > 1) {        
            if (symbolList.getLocalSize ()) 
                if (TAnyManager *anyManager = buildAnyManager (symbolList)) {
                    runtimeData.registerAnyManager (anyManager);
                    const ssize_t count = stackSize / 16 - (level - 1) / 2;
                    outputCode (TA64Op::mov, {intScratchReg1, TA64Reg::sp});
                    loadImmediate (intScratchReg2, count);
                    const std::string label = getNextLocalLabel ();
                    outputLabel (label);
                    outputCode (TA64Op::stp, {TA64Reg::xzr, TA64Reg::xzr, TA64Operand (intScratchReg1, 16, TA64Operand::PostIndex)});
                    outputCode (TA64Op::subs, {intScratchReg2, intScratchReg2, 1});
                    outputCode (TA64Op::bne, {label});
                } 
            // Display
            std::size_t offset = 8;
            for (std::size_t i = 1; i < level - 2; i += 2) {
                outputCode (TA64Op::ldp, {intScratchReg1, intScratchReg2, TA64Operand (intScratchReg3, TA64Reg::none, -offset - 8)});
                outputCode (TA64Op::stp, {intScratchReg1, intScratchReg2, TA64Operand (TA64Reg::x29, TA64Reg::none, -offset - 8)});
                offset += 16;
            }
            if (level & 1) {
                outputCode (TA64Op::ldr, {intScratchReg1, TA64Operand (intScratchReg3, TA64Reg::none, -offset)});
                outputCode (TA64Op::str, {intScratchReg1, TA64Operand (TA64Reg::x29, TA64Reg::none, -offset)});
                offset += 8;
            }
            outputCode (TA64Op::str, {TA64Reg::x29, TA64Operand (TA64Reg::x29, TA64Reg::none, -offset)});
        }
    }
    
    std::vector<TA64Reg>::const_iterator it = saveRegs.begin ();
    while (it != saveRegs.end ()) {
        codePush (it [0], it [1]);
        it += 2;
    }

    struct TDeepCopy {
        ssize_t stackOffset;
        std::size_t size, runtimeIndex;
    };
    std::vector<TDeepCopy> deepCopies;
    
    std::size_t intCount = 0, dblCount = 0;
    for (TSymbol *s: symbolList) {
//        std::cout << "PARA: " << s->getName () << ": " << s->getType () ->getName () << std::endl;
        if (s->checkSymbolFlag (TSymbol::Parameter)) {
            TType *type = s->getType ();
            if (type == &stdType.Real || type == &stdType.Single) {
                if (dblCount < dblParaRegs) {
                    if (s->getRegister () == TSymbol::InvalidRegister) {
                        loadImmediate (intScratchReg1, s->getOffset ());
                        outputCode (TA64Op::str, {TA64Operand (dblParaReg [dblCount], type ==  &stdType.Single ? TA64OpSize::bit32 : TA64OpSize::bit_default), TA64Operand (TA64Reg::x29, intScratchReg1, 0)});
                    }
                    ++dblCount;
                }
            } else if (classifyType (type) == TParameterLocation::IntReg) {
                if (intCount < intParaRegs) {
                    TType *st = type->getSize () == 8 ? &stdType.Int64 : getMemoryOperationType (type);
                    // TODO: mark return temp ????
                    if (s->getName () [0] == '$') {
                        loadImmediate (intScratchReg1, s->getOffset ());
                        outputCode (TA64Op::str, {TA64Reg::x8, TA64Operand (TA64Reg::x29, intScratchReg1, 0)});
                    } else if (st != &stdType.UnresOverload) {
                        if (s->getRegister () == TSymbol::InvalidRegister) {
                            loadImmediate (intScratchReg1, s->getOffset ());
                            codeStoreMemory (st, intParaReg [intCount], TA64Operand (TA64Reg::x29, intScratchReg1, 0));
                        }
                        ++intCount;
                    }  else {
                        std::cout << "Cannot save parameter " << s->getName () << std::endl;
                        exit (1);
                    }
                }
            } else if (classifyType (type) == TParameterLocation::ObjectByValue) {
                // TODO: das kopiert einen Return-Value erstmal auf sich selbst !!!!
                deepCopies.push_back ({s->getOffset (), type->getSize (), lookupAnyManager (type).runtimeIndex});
//                std::cout << s->getName () << std::endl;
                if (intCount < intParaRegs) {
                    loadImmediate (intScratchReg1, s->getOffset ());
                    outputCode (TA64Op::str, {intParaReg [intCount++], TA64Operand (TA64Reg::x29, intScratchReg1, 0)});
                } else {
                    loadImmediate (intScratchReg2, s->getParameterPosition ());
                    outputCode (TA64Op::ldr, {intScratchReg2, TA64Operand (TA64Reg::x29, intScratchReg2, 0)});
                    loadImmediate (intScratchReg1, s->getOffset ());
                    outputCode (TA64Op::str, {intScratchReg2, TA64Operand (TA64Reg::x29, intScratchReg1, 0)});
                }
            } else {
                // value parameter is on stack: nothing to do
            }
        }            
    }
    for (const TDeepCopy &deepCopy: deepCopies) {
        outputCode (TA64Op::add, {TA64Reg::x0, TA64Reg::x29, deepCopy.stackOffset});
        outputCode (TA64Op::ldr, {TA64Reg::x1, TA64Operand (TA64Reg::x0, TA64Reg::none, 0)});
        codeRuntimeCall ("rt_copy_mem", TA64Reg::x5, {{TA64Reg::x2, deepCopy.size}, {TA64Reg::x3, deepCopy.runtimeIndex}, {TA64Reg::x4, 0}});
    }
}

void TA64Generator::endRoutineBody (std::size_t level, TSymbolList &symbolList, const std::vector<TA64Reg> &saveRegs, bool hasStackFrame) {
    std::vector<TA64Reg>::const_reverse_iterator it = saveRegs.rbegin ();
    while (it != saveRegs.rend ()) {
        codePop (it [1], it [0]);
        it += 2;
    }
        
    if (hasStackFrame) {
        outputCode (TA64Op::mov, {TA64Reg::sp, TA64Reg::x29});
        codePop (TA64Reg::x29, TA64Reg::x30);
    }
    outputCode (TA64Op::ret);
//    outputCode (TA64Op::ltorg);
}

TCodeGenerator::TParameterLocation TA64Generator::classifyType (const TType *type) {
    if (type->isEnumerated () || type->isPointer () || type->isReference () || type->isRoutine ()) 
        return TParameterLocation::IntReg;
    if (type == &stdType.Real || type == &stdType.Single)
        return TParameterLocation::FloatReg;
    if (lookupAnyManager (type).anyManager)
        return TParameterLocation::ObjectByValue;
    // TODO: Structs <= 16 Bytes, small arrays
    return TParameterLocation::Stack;
}

TCodeGenerator::TReturnLocation TA64Generator::classifyReturnType (const TType *type) {
    if (type->isEnumerated () || type->isPointer () || type->isRoutine () || type == &stdType.Real || type == &stdType.Single)
        return TReturnLocation::Register;
    else
        return TReturnLocation::Reference;
}

void TA64Generator::assignParameterOffsets (ssize_t &pos, TBlock &block, std::vector<TSymbol *> &registerParameters) {
    TSymbolList &symbolList = block.getSymbols ();
    
    std::size_t valueParaOffset = 16;
    std::size_t intCount = 0, dblCount = 0;
    for (TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Parameter) && s->getRegister () == TSymbol::InvalidRegister) {
            TType *type = s->getType ();
            alignType (type);
            TParameterLocation parameterLocation = classifyType (type);
            if (parameterLocation == TParameterLocation::ObjectByValue) {
                assignAlignedOffset (*s, pos, type->getAlignment ());
                if (intCount >= intParaRegs) {
                    s->setParameterPosition (valueParaOffset);
                    valueParaOffset += 8;
                } else 
                    ++intCount;
                registerParameters.push_back (s);
            } else if (parameterLocation == TParameterLocation::Stack ||
                (parameterLocation == TParameterLocation::IntReg && intCount >= intParaRegs) ||
                (parameterLocation == TParameterLocation::FloatReg && dblCount >= dblParaRegs)) {
                s->setOffset (valueParaOffset);
                valueParaOffset += ((type->getSize () + 7) & ~7);
            } else {
                assignAlignedOffset (*s, pos, type->getAlignment ());
                if (s->getName () [0] != '$')	// passed in x
                    intCount += (parameterLocation == TParameterLocation::IntReg);
                dblCount += (parameterLocation == TParameterLocation::FloatReg);
                registerParameters.push_back (s);
            }
        }
}

void TA64Generator::assignStackOffsets (TBlock &block) {
    TSymbolList &symbolList = block.getSymbols ();
    
    const std::size_t alignment = TConfig::alignment - 1,
                      level = symbolList.getLevel (),
                      displaySize = (level) * sizeof (void *);
    ssize_t pos = 0;
    std::vector<TSymbol *> registerParameters;
    
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

void TA64Generator::assignRegisters (TSymbolList &blockSymbols) {
    std::set<TA64Reg> dblReg, intReg;
    
    // TODO: can use srcatch regs as well
    for (std::size_t i = 0; i < dblParaRegs; ++i)
        dblReg.insert (dblParaReg [i]);
    for (std::size_t i = 0; i < intParaRegs; ++i)
        if (!isRegUsed (intParaReg [i]))
            intReg.insert (intParaReg [i]);
        
    std::size_t dblParaCount = 0, intParaCount = 0;
    for (TSymbol *s: blockSymbols)
        if (s->checkSymbolFlag (TSymbol::Parameter)) {
            if (s->getType ()->isReal () && dblParaCount < dblParaRegs) {
                if (!s->isAliased ()) {
                    s->setRegister (static_cast<std::size_t> (dblParaReg [dblParaCount]));
                    dblReg.erase (dblParaReg [dblParaCount]);
                }
                ++dblParaCount;
            } else if (classifyType (s->getType ()) == TParameterLocation::IntReg && intParaCount < intParaRegs) {
                if (!isRegUsed (intParaReg [intParaCount]) && (!s->isAliased () || s->getType ()->isReference ())) {
                    s->setRegister (static_cast<std::size_t> (intParaReg [intParaCount]));
                    intReg.erase (intParaReg [intParaCount]);
                }
                ++intParaCount;
            }
        }
            
    std::set<TA64Reg>::iterator itDbl = dblReg.begin (), itInt = intReg.begin ();    
    for (TSymbol *s: blockSymbols)
        if (s->checkSymbolFlag (TSymbol::Variable)) {
            if (s->getType ()->isReal () && !s->isAliased () && itDbl != dblReg.end ()) {
                s->setRegister (static_cast<std::size_t> (*itDbl));
                itDbl = dblReg.erase (itDbl);
            } else if (classifyType (s->getType ()) == TParameterLocation::IntReg && !s->isAliased () && itInt != intReg.end ()) {
                s->setRegister (static_cast<std::size_t> (*itInt));
                itInt = intReg.erase (itInt);
            }
        }
}

void TA64Generator::initStaticRoutinePtr (std::size_t addr, const TRoutineValue *routineValue) {
    loadImmediate (intScratchReg1, addr);
    loadLabelAddress (routineValue->getSymbol ()->getOverloadName (), intScratchReg2);
    outputCode (TA64Op::str, {intScratchReg2, TA64Operand (intScratchReg1, TA64Reg::none, 0)});
}

void TA64Generator::codeBlock (TBlock &block, std::vector<TUnit *> units, bool hasStackFrame, std::vector<TA64Operation> &blockStatements) {
    TSymbolList &blockSymbols = block.getSymbols ();
    const std::size_t level = blockSymbols.getLevel ();
    
    std::vector<TA64Operation> blockPrologue, blockEpilogue, blockCode, globalInits;

    intStackCount = 0;
    dblStackCount = 0;
    currentLevel =  blockSymbols.getLevel ();
    endOfRoutineLabel = getNextLocalLabel ();
    
    setOutput (&globalInits);
    if (blockSymbols.getLevel () == 1) {
        assignGlobals (blockSymbols);
        initStaticGlobals (blockSymbols);
    }
    
    setOutput (&blockCode);
    
    if (!globalInits.empty ()) 
        outputCode (TA64Op::bl, {TA64Operand ("$init_static")});
    visit (block.getStatements ());
    
//    logOptimizer = block.getSymbol ()->getOverloadName () == "gettapeinput_$182";


    outputLabel (endOfRoutineLabel);
    if (level > 1) {
        // TODO: this is called twice!
        if (TAnyManager *anyManager = buildAnyManager (blockSymbols)) {
            std::size_t index = registerAnyManager (anyManager);
            outputCode (TA64Op::mov, {TA64Reg::x0, TA64Reg::x29});
            codeRuntimeCall ("rt_destroy_mem", TA64Reg::x2, {{TA64Reg::x1, index}});
        }
        if (TExpressionBase *returnLValueDeref = block.returnLValueDeref) {
            visit (returnLValueDeref);
            if (returnLValueDeref->getType () == &stdType.Real || returnLValueDeref->getType () == &stdType.Single) {
                loadDblReg (TA64Reg::d0);
                if (returnLValueDeref->getType () == &stdType.Single)
                    outputCode (TA64Op::fcvt, {TA64Operand (TA64Reg::d0, TA64OpSize::bit32), TA64Reg::d0});
            } else
                loadReg (TA64Reg::x0);
        }
    }    
    // TODO: destory global variables !!!!
    
    removeUnusedLocalLabels (blockCode);
    optimizePeepHole (blockCode);
    tryLeaveFunctionOptimization (blockCode);
    
    // check which callee saved registers are used after peep hole optimization
    std::set<TA64Reg> regs;;
    for (const TA64Operation &op: blockCode) 
        for (const TA64Operand &operand: op.operands)
            if (isCalleeSavedReg (operand))
                regs.insert (operand.reg);
    std::vector<TA64Reg> saveRegs;
    std::copy (regs.begin (), regs.end (), std::back_inserter (saveRegs));
    if (saveRegs.size () & 1)
        saveRegs.push_back (TA64Reg::xzr);
    
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
        outputCode (TA64Op::ret);
    }

    blockStatements.clear ();    
    blockStatements.reserve (blockPrologue.size () + blockCode.size () + blockEpilogue.size ());
    std::move (blockPrologue.begin (), blockPrologue.end (), std::back_inserter (blockStatements));
    std::move (blockCode.begin (), blockCode.end (), std::back_inserter (blockStatements));
    std::move (blockEpilogue.begin (), blockEpilogue.end (), std::back_inserter (blockStatements));
}

bool TA64Generator::isReferenceCallerCopy (const TType *type) {
    return classifyType (type) == TParameterLocation::Stack;
}

void TA64Generator::generateBlock (TBlock &block, std::vector<TUnit *> units) {
//    std::cout << "Entering: " << block.getSymbol ()->getName () << std::endl;

    TSymbolList &blockSymbols = block.getSymbols ();
    for (TSymbol *s: blockSymbols)
        if (s->checkSymbolFlag (TSymbol::Label))
            s->setName (getUniqueLabelName (s->getName ()));

    std::vector<TA64Operation> blockStatements;
    
    assignStackOffsets (block);
    if (blockSymbols.getLevel () == 1) 
        assignGlobals (blockSymbols);
    clearRegsUsed ();
    codeBlock (block, units, true, blockStatements);
    
    if (blockSymbols.getLevel () > 1) {
        bool functionCalled = std::find_if (blockStatements.begin (), blockStatements.end (), [] (const TA64Operation &op) { return op.operation == TA64Op::blr || op.operation == TA64Op::bl; }) != blockStatements.end ();
        if (!functionCalled) {
            assignRegisters (blockSymbols);
            assignStackOffsets (block);
            bool stackFrameNeeded = std::find_if (blockSymbols.begin (), blockSymbols.end (), [] (const TSymbol *s) {
                return (s->checkSymbolFlag (TSymbol::Parameter) || s->checkSymbolFlag (TSymbol::Variable)) && s->getRegister () == TSymbol::InvalidRegister; }) != blockSymbols.end ();
            codeBlock (block, units, stackFrameNeeded || block.isDisplayNeeded (), blockStatements);
        }
    }
    
    replacePseudoOpcodes (blockStatements);
    std::move (blockStatements.begin (), blockStatements.end (), std::back_inserter (program));
    
    for (TSymbol *s: blockSymbols) 
        if (s->checkSymbolFlag (TSymbol::Routine)) {
            if (s->checkSymbolFlag (TSymbol::External)) 
                externalRoutine (*s);
            else 
                visit (s->getBlock ());
        }
}

bool TA64Generator::is32BitLimit (std::int64_t n) {
    return std::numeric_limits<std::int32_t>::min () <= n && n <= std::numeric_limits<std::int32_t>::max ();
}

std::string TA64Generator::getUniqueLabelName (const std::string &s) {
    return s + "_" + std::to_string (labelCount++) + '$';
}

} // namespace statpascal
