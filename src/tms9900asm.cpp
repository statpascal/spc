#include "tms9900asm.hpp"

#include <vector>
#include <sstream>
#include <iomanip>

namespace statpascal {

/*    
    opname =  {"li", "ai", "andi", "ori", "ci", "stwp", "stst", "lwpi", "limi", "idle", "rset", "rtwp", "ckon", "ckof", "lrex",
               "blwp", "b", "x", "clr", "neg", "inv", "inc", "inct", "dec", "dect", "bl", "swpb", "seto", "abs", "sra", "srl", "sla", "src",
               "jmp", "jlt", "jle", "jeq", "jhe", "jgt", "jne", "jnc", "joc", "jno", "jl", "jh", "jop", "sbo", "sbz", "tb", "coc", "czc",
               "xor", "xop", "ldcr", "stcr", "mpy", "div", "szc", "szcb", "s", "sb", "c", "cb", "a", "ab", "mov", "movb", "soc", "socb",
               "aorg", "even", "", "", "stri", "data", "byte", "end"};
*/
               
const std::vector<T9900OpDescription> opDescription = {
    {T9900Op::li,   0x0200, "li",   T9900Format::F8},
    {T9900Op::ai,   0x0220, "ai",   T9900Format::F8},
    {T9900Op::andi, 0x0240, "andi", T9900Format::F8},
    {T9900Op::ori,  0x0260, "ori",  T9900Format::F8},
    {T9900Op::ci,   0x0280, "ci",   T9900Format::F8},
    {T9900Op::stwp, 0x02a0, "stwp", T9900Format::F8_2},
    {T9900Op::stst, 0x02c0, "stst", T9900Format::F8_2},
    {T9900Op::lwpi, 0x02e0, "lwpi", T9900Format::F8_1},
    {T9900Op::limi, 0x0300, "limi", T9900Format::F8_1},
    {T9900Op::idle, 0x0340, "idle", T9900Format::F7},
    {T9900Op::rset, 0x0360, "rset", T9900Format::F7},
    {T9900Op::rtwp, 0x0380, "rtwp", T9900Format::F7},
    {T9900Op::ckon, 0x03a0, "ckon", T9900Format::F7},
    {T9900Op::ckof, 0x03c0, "ckof", T9900Format::F7},
    {T9900Op::lrex, 0x03e0, "lrex", T9900Format::F7},
    {T9900Op::blwp, 0x0400, "blwp", T9900Format::F6},
    {T9900Op::b,    0x0440, "b",    T9900Format::F6},
    {T9900Op::x,    0x0480, "x",    T9900Format::F6},
    {T9900Op::clr,  0x04c0, "clr",  T9900Format::F6},
    {T9900Op::neg,  0x0500, "neg",  T9900Format::F6},
    {T9900Op::inv,  0x0540, "inv",  T9900Format::F6},
    {T9900Op::inc,  0x0580, "inc",  T9900Format::F6},
    {T9900Op::inct, 0x05c0, "inct", T9900Format::F6},
    {T9900Op::dec,  0x0600, "dec",  T9900Format::F6},
    {T9900Op::dect, 0x0640, "dect", T9900Format::F6},
    {T9900Op::bl,   0x0680, "bl",   T9900Format::F6},
    {T9900Op::swpb, 0x06c0, "swpb", T9900Format::F6},
    {T9900Op::seto, 0x0700, "seto", T9900Format::F6},
    {T9900Op::abs,  0x0740, "abs",  T9900Format::F6},
    {T9900Op::sra,  0x0800, "sra",  T9900Format::F5},
    {T9900Op::srl,  0x0900, "srl",  T9900Format::F5},
    {T9900Op::sla,  0x0a00, "sla",  T9900Format::F5},
    {T9900Op::src,  0x0b00, "src",  T9900Format::F5},
    {T9900Op::jmp,  0x1000, "jmp",  T9900Format::F2},
    {T9900Op::jlt,  0x1100, "jlt",  T9900Format::F2},
    {T9900Op::jle,  0x1200, "jle",  T9900Format::F2},
    {T9900Op::jeq,  0x1300, "jeq",  T9900Format::F2},
    {T9900Op::jhe,  0x1400, "jhe",  T9900Format::F2},
    {T9900Op::jgt,  0x1500, "jgt",  T9900Format::F2},
    {T9900Op::jne,  0x1600, "jne",  T9900Format::F2},
    {T9900Op::jnc,  0x1700, "jnc",  T9900Format::F2},
    {T9900Op::joc,  0x1800, "joc",  T9900Format::F2},
    {T9900Op::jno,  0x1900, "jno",  T9900Format::F2},
    {T9900Op::jl,   0x1a00, "jl",   T9900Format::F2},
    {T9900Op::jh,   0x1b00, "jh",   T9900Format::F2},
    {T9900Op::jop,  0x1c00, "jop",  T9900Format::F2},
    {T9900Op::sbo,  0x1d00, "sbo",  T9900Format::F2M},
    {T9900Op::sbz,  0x1e00, "sbz",  T9900Format::F2M},
    {T9900Op::tb,   0x1f00, "tb",   T9900Format::F2M},
    {T9900Op::coc,  0x2000, "coc",  T9900Format::F3},
    {T9900Op::czc,  0x2400, "czc",  T9900Format::F3},
    {T9900Op::xor_, 0x2800, "xor",  T9900Format::F3},
    {T9900Op::xop,  0x2c00, "xop",  T9900Format::F9},
    {T9900Op::ldcr, 0x3000, "ldcr", T9900Format::F4},
    {T9900Op::stcr, 0x3400, "stcr", T9900Format::F4},
    {T9900Op::mpy,  0x3800, "mpy",  T9900Format::F9},
    {T9900Op::div,  0x3c00, "div",  T9900Format::F9},
    {T9900Op::szc,  0x4000, "szc",  T9900Format::F1},
    {T9900Op::szcb, 0x5000, "szcb", T9900Format::F1},
    {T9900Op::s,    0x6000, "s",    T9900Format::F1},
    {T9900Op::sb,   0x7000, "sb",   T9900Format::F1},
    {T9900Op::c,    0x8000, "c",    T9900Format::F1},
    {T9900Op::cb,   0x9000, "cb",   T9900Format::F1},
    {T9900Op::a,    0xa000, "a",    T9900Format::F1},
    {T9900Op::ab,   0xb000, "ab",   T9900Format::F1},
    {T9900Op::mov,  0xc000, "mov",  T9900Format::F1},
    {T9900Op::movb, 0xd000, "movb", T9900Format::F1},
    {T9900Op::soc,  0xe000, "soc",  T9900Format::F1},
    {T9900Op::socb, 0xf000, "socb", T9900Format::F1},
    // pseudo ops
    {T9900Op::aorg,      0, "aorg", T9900Format::F_None},
    {T9900Op::even,      0, "even", T9900Format::F_None},
    {T9900Op::def_label, 0, "",     T9900Format::F_None},
    {T9900Op::comment,   0, "",     T9900Format::F_None},
    {T9900Op::stri,      0, "stri", T9900Format::F_None},
    {T9900Op::data,      0, "data", T9900Format::F_None},
    {T9900Op::byte,      0, "byte", T9900Format::F_None},
    {T9900Op::text,      0, "text", T9900Format::F_None},
    {T9900Op::end,       0, "end",  T9900Format::F_None}
};

T9900Operand::T9900Operand ():
  t (TAddressingMode::Invalid) {
}

T9900Operand::T9900Operand (T9900Reg reg, TAddressingMode t):
  reg (reg),
  t (t),
  val (0) {
}

T9900Operand::T9900Operand (T9900Reg reg, std::uint16_t base):
  reg (reg),
  t (reg == T9900Reg::r0 ? TAddressingMode::Memory : TAddressingMode::Indexed),
  val (base) {
}

T9900Operand::T9900Operand (std::uint16_t imm):
  reg (T9900Reg::r0),
  t (TAddressingMode::Imm),
  val (imm) {
}

T9900Operand::T9900Operand (const std::string &label):
  label (label),
  reg (T9900Reg::r0),
  t (TAddressingMode::Imm),
  val (0) {
}

T9900Operand::T9900Operand (const std::string &label, const T9900Reg reg):
  label (label),
  reg (reg),
  t (reg == T9900Reg::r0 ? TAddressingMode::Memory : TAddressingMode::Indexed),
  val (0) {
}

std::string T9900Operand::getValue (bool hex) const {
    if (label.empty ())
        if (hex) {
            std::stringstream ss;
            ss << '>' << std::hex << std::setfill ('0') << std::setw (4) << val;
            return ss.str ();
        } else
        return std::to_string (val);
    else
        return label;
}
  
std::string T9900Operand::makeString (bool hex) const {
    switch (t) {
        case TAddressingMode::Invalid:
            return std::string ();
        case TAddressingMode::Reg:
            return regname [static_cast<std::size_t> (reg)];
        case TAddressingMode::RegInd:
            return "*" + regname [static_cast<std::size_t> (reg)];
        case TAddressingMode::RegIndInc:
            return "*" + regname [static_cast<std::size_t> (reg)] + "+";
        case TAddressingMode::Memory:
            return "@" + getValue (hex);
        case TAddressingMode::Indexed:
            return "@" + getValue (hex) + "(" + regname [static_cast<std::size_t> (reg)] + ")";
        case TAddressingMode::Imm:
            return getValue (hex);
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
            return operand1.makeString () + ": even";	// required for xas99/labels on consecutive lines
        case T9900Op::comment:
            return comment.empty () ? comment : "; " + comment;
        case T9900Op::stri: {
            std::stringstream res;
            auto append = [&res] (char c) {
                res << std::hex << std::setfill ('0') << std::setw (2) << std::nouppercase << static_cast<unsigned> (c);
            };
            append (operand2.label.length ());
            for (const char c: operand2.label)
                append (c);
            return operand1.makeString () + " text >" + res.str () + "    ; " + operand2.label; }
        case T9900Op::byte: {
            std::stringstream res;
            if (operand1.label.empty ()) {
                res << std::string (8, ' ') << "byte >" << std::setfill ('0') << std::hex << std::setw (2) << operand1.val;
                return res.str ();
            }
            auto append = [&res] (char c) {
                res << std::hex << std::setfill ('0') << std::setw (2) << std::nouppercase << static_cast<unsigned> (c);
            };
            for (const char c: operand1.label)
                append (c);
            return std::string (8, ' ') +  "text >" + res.str (); }
        case T9900Op::text:
            return std::string (8, ' ') + "text '" + operand1.label + "'";
        case T9900Op::end:
            return "";
        default: {
            const T9900OpDescription &od = opDescription [static_cast<std::size_t> (operation)];
            std::string res = std::string (8, ' ') + od.name;
            res.resize (13, ' ');
            if (operand1.isValid ())
                res += " " + operand1.makeString (true);
            if (operand2.isValid ())
                res += ", " + operand2.makeString (od.format != T9900Format::F5 && od.format != T9900Format::F2M && od.format != T9900Format::F4);
            if (!comment.empty ()) {
                res.resize (40, ' ');
                return res + "; " + comment;
            } else
                return res;}
    }
}

int T9900Operation::getSize () const {
    switch (opDescription [static_cast<std::size_t> (operation)].format) {
        case T9900Format::F2:
        case T9900Format::F2M:
        case T9900Format::F5:
            return 2;
        case T9900Format::F4:
            return 2 + operand1.getSize ();
        case T9900Format::F_None:
            return 0;
        default:
            return 2 + operand1.getSize () + operand2.getSize ();
    }
}

bool lookupInstruction (const std::string &s, T9900OpDescription &desc) {
    for (const T9900OpDescription &i: opDescription)
        if (i.name == s) {
            desc = i;
            return true;
        }
      return false;
}

}
