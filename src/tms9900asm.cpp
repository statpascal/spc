#include "tms9900asm.hpp"

#include <vector>

namespace statpascal {

const std::vector<std::string>
    regname = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"},
    opname =  {"li", "ai", "andi", "ori", "ci", "stwp", "stst", "lwpi", "limi", "idle", "rset", "rtwp", "ckon", "ckof", "lrex",
               "blwp", "b", "x", "clr", "neg", "inv", "inc", "inct", "dec", "dect", "bl", "swpb", "seto", "abs", "sra", "srl", "sla", "src",
               "jmp", "jlt", "jle", "jeq", "jhe", "jgt", "jne", "jnc", "joc", "jno", "jl", "jh", "jop", "sbo", "sbz", "tb", "coc", "czc",
               "xor", "xop", "ldcr", "stcr", "mpy", "div", "szc", "szcb", "s", "sb", "c", "cb", "a", "ab", "mov", "movb", "soc", "socb",
               "", "", ""};

T9900Operand::T9900Operand ():
  valid (false) {
}

T9900Operand::T9900Operand (T9900Reg reg, TAddressingMode t):
  reg (reg),
  valid (true),
  isImm (false), 
  isLabel (false),
  t (t) {
}

T9900Operand::T9900Operand (T9900Reg reg, std::uint16_t offset):
  reg (reg),
  valid (true),
  isImm (false),
  isLabel (false),
  t (TAddressingMode::Indexed),
  val (offset) {
}

T9900Operand::T9900Operand (std::uint16_t imm):
  valid (true),
  isImm (true),
  isLabel (false),
  val (imm) {
}

T9900Operand::T9900Operand (const std::string &label):
  label (label),
  valid (true),
  isImm (false),
  isLabel (true) {
}
  
std::string T9900Operand::makeString (bool addAt) const {
    if (!valid)
        return std::string ();
    if (isLabel)
        return addAt ? "@" + label: label;
    if (isImm) 
        return addAt ? "@" + std::to_string (val) : std::to_string (val);
    switch (t) {
        case TAddressingMode::Reg:
            return regname [static_cast<std::size_t> (reg)];
        case TAddressingMode::RegInd:
            return "*" + regname [static_cast<std::size_t> (reg)];
        case TAddressingMode::RegIndInc:
            return "*" + regname [static_cast<std::size_t> (reg)] + "+";
        case TAddressingMode::Indexed:
            return "@" + std::to_string (val) + (reg != T9900Reg::r0 ? "(" + regname [static_cast<std::size_t> (reg)] + ")" : std::string ());
    }
    return std::string ();
}

T9900Operation::T9900Operation (T9900Op op, T9900Operand op1, T9900Operand op2, const std::string &comment):
  operation (op),
  operand1 (op1),
  operand2 (op2),
  comment (comment) {
}

std::string T9900Operation::makeString () const {
    switch (operation) {
        case T9900Op::def_label:
            return operand1.makeString () + ":";
        case T9900Op::comment:
            return comment.empty () ? comment : "; " + comment;
        case T9900Op::end:
            return "";
        default: {
            std::string res = std::string (8, ' ') + opname [static_cast<std::size_t> (operation)];
            res.resize (13, ' ');
            if (operand1.isValid ())
                res += " " + operand1.makeString (operation == T9900Op::b || operation == T9900Op::blwp);
            if (operand2.isValid ())
                res += ", " + operand2.makeString ();
            if (!comment.empty ()) {
                res.resize (40, ' ');
                return res + "; " + comment;
            } else
                return res;}
    }
}

}
