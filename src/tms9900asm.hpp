/** \file tms9900asm.hpp
*/

#pragma once

#include <cstdint>
#include <string>

namespace statpascal {

enum class T9900Op {
    li, ai, andi, ori, ci, stwp, stst, lwpi, limi, idle, rset, rtwp, ckon, ckof, lrex, 
    blwp, b, x, clr, neg, inv, inc, inct, dec, dect, bl, swpb, seto, abs, sra, srl, sla, src,
    jmp, jlt, jle, jeq, jhe, jgt, jne, jnc, joc, jno, jl, jh, jop, sbo, sbz, tb, coc, czc,
    xor_, xop, ldcr, stcr, mpy, div, szc, szcb, s, sb, c, cb, a, ab, mov, movb, soc, socb,
    // Pseude ops 
    def_label, comment, stri, data, byte, end
};

enum class T9900Reg {
    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, nrRegs
};

class T9900Operand {
public:
    enum class TAddressingMode { Invalid, Reg, RegInd, RegIndInc, Indexed, Imm, Label };
    
    T9900Operand ();
    T9900Operand (T9900Reg, TAddressingMode t = TAddressingMode::Reg);
    T9900Operand (T9900Reg, std::uint16_t offset);
    T9900Operand (std::uint16_t imm);
    T9900Operand (const std::string &label);

    std::string makeString (bool addAt = false) const;
    bool isValid () const { return t != TAddressingMode::Invalid; }
    bool isLabel () const { return t == TAddressingMode::Label; }
    bool isImm () const { return t == TAddressingMode::Imm; }
    bool isIndexed () const { return t == TAddressingMode::Indexed; }
    bool isReg () const { return t == TAddressingMode::Reg ||
                                 t == TAddressingMode::RegInd ||
                                 t == TAddressingMode::RegIndInc ||
                                 (t == TAddressingMode::Indexed && reg != T9900Reg::r0); }
    
    std::string label;
    T9900Reg reg;
    TAddressingMode t;
    std::uint16_t val;
};

class T9900Operation {
public:
    T9900Operation (T9900Op, T9900Operand = T9900Operand (), T9900Operand = T9900Operand (), const std::string &comment = std::string ());
    
    std::string makeString () const;
    
    T9900Op operation;
    T9900Operand operand1, operand2;
    std::string comment;
};

}
