/** \file a64asm.hpp
*/

#pragma once

#include <vector>
#include <string>

namespace statpascal {

enum class TA64OpSize {bit_default, bit32, bit64};

const std::vector<std::string> 
    a64RegName   = {"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30", "xzr", "sp",
                    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"},
    a64Reg32Name = {"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7", "w8", "w9", "w10", "w11", "w12", "w13", "w14", "w15", "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23", "w24", "w25", "w26", "w27", "w28", "w29", "w30", "wzr", "wsp",
                    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
    
enum class TA64Op {
    adr,
    adrp,
    ret,
    mov,
    movz,
    movk,
    movn,
    ldr,
    cmp,
    bcge,
    beq,
    bne,
    ble,
    bge,
    blt,
    bgt,
    add,
    adds,
    sub,
    lsr,
    str,
    orr,
    eor,
    mul,
    madd,
    and_,
    ands,
    sdiv,
    lsl,
    neg,
    mvn,
    asr,
    b,
    br,
    bl,
    blr,
    stp,
    ldp,
    cset,
    csel,
    ldrb,
    ldrsb,
    ldrh,
    ldrsh,
    ldrsw,
    strb,
    strh,
    rev16,
    subs,
    udiv,
    msub,
    tbz,
    cbz,
    tbnz,
    cbnz,
    // unsigned 
    bls,
    
    fcmp,
    fadd,
    fsub,
    fmul,
    fdiv,
    fneg,
    fcvt,
    fmov,
    scvtf,
    fabs,
    fsqrt,
    // pseudo ops
    li, nop, mcpy, ltorg, def_label, data_dq, comment, aligncode, end
};

enum class TA64Reg {
    x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30,
    xzr, sp,
    d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15, d16, d17, d18, d19, d20, d21, d22, d23, d24, d25, d26, d27, d28, d29, d30, d31,
    none, nrRegs = none
};

class TA64Operand {
public:
    TA64Operand ();
    
    // Register
    TA64Operand (TA64Reg, TA64OpSize = TA64OpSize::bit_default);
    
    // Offset modes
    TA64Operand (TA64Reg base, TA64Reg index, ssize_t offset);	
    
    // Index modes
    enum TIndexMode {PreIndex, PostIndex};
    TA64Operand (TA64Reg base, ssize_t offset, TIndexMode);
    
    // Immediates
    TA64Operand (ssize_t imm);
    
    // Label
    TA64Operand (const std::string &label, bool ripRelative = false);
    
    // Condition
    enum TCondition {EQ, NE, LE, GE, LT, GT, nrConditions};
    TA64Operand (TCondition);
    
    std::string makeString () const;
    bool isValid () const;

    
    bool isReg, isOffset, isIndex, isImm, isLabel, isCondition;
    
    TA64Reg reg, base, index;
    TIndexMode indexMode;
    ssize_t offset, imm;
    std::string label;
    bool ripRelative;    
    TA64OpSize opSize;
    TCondition cond;
    
    std::string getOpSizeName () const;
    std::string getRegName (TA64Reg, TA64OpSize) const;
    
private:    
    void init ();
};

class TA64Operation {
public:
    TA64Operation (TA64Op, std::vector<TA64Operand> &&, std::string &&comment);
    
    void appendString (std::vector<std::string> &) const;
    
    TA64Op operation;
    std::vector<TA64Operand> operands;
    std::string comment;
};

}