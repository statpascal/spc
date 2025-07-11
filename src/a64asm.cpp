/** \file a64asm.cpp
*/

#include "a64asm.hpp"

#include <map>

namespace statpascal {

void TA64Operand::init () {
    isReg = isOffset = isIndex = isImm = isLabel = isCondition = false;
    offset = imm = 0;
    opSize = TA64OpSize::bit_default;
    reg = base = index = TA64Reg::none;
}

TA64Operand::TA64Operand () {
    init ();
}
  
TA64Operand::TA64Operand (TA64Reg r, TA64OpSize s) {
    init ();
    isReg = true;
    reg = r;
    opSize = s;
}

TA64Operand::TA64Operand (TA64Reg b, TA64Reg i, ssize_t o) {
    init ();
    isOffset = true;
    base = b;
    index = i;
    offset = o;
}

TA64Operand::TA64Operand (TA64Reg b, ssize_t o, TIndexMode m) {
    init ();
    isIndex = true;
    base = b;
    offset = o;
    indexMode = m;
}

TA64Operand::TA64Operand (ssize_t i) {
    init ();
    isImm = true;
    imm = i;
}

TA64Operand::TA64Operand (const std::string &l, bool rel) {
    init ();
    isLabel = true;
    label = l;
    ripRelative = rel;
}

TA64Operand::TA64Operand (TCondition c) {
    init ();
    isCondition = true;
    cond = c;
}

std::string TA64Operand::getRegName (TA64Reg reg, TA64OpSize opSize) const {
    static const std::map<TA64OpSize, const std::vector<std::string> &> a64RegNames = {
        {TA64OpSize::bit_default, a64RegName}, {TA64OpSize::bit32, a64Reg32Name}, {TA64OpSize::bit64, a64RegName}};
    return a64RegNames.at (opSize)[static_cast<std::size_t> (reg)];
}

std::string TA64Operand::makeString () const {
    std::string res;
    if (isReg) {
        res = getRegName (reg, opSize);
        if (offset != 0) 
            res += ", lsl #" + std::to_string (offset);
    } else if (isOffset) {
        res =  "[" + getRegName (base, TA64OpSize::bit64);
        if (index != TA64Reg::none) {
            res.append (", " + getRegName (index, TA64OpSize::bit64));
            if (offset)
                res.append (", lsl #" + std::to_string (offset));
        } else if (offset)
            res.append (", " + std::to_string (offset));
        else if (!label.empty ())
            res.append (", " + label);
        res.append ("]");
    } else if (isIndex) {
        res = "[" + getRegName (base, TA64OpSize::bit64);
        if (indexMode == PreIndex)
            res.append (", #" + std::to_string (offset) + "]!");
        else
            res.append ("], #" + std::to_string (offset));
    } else if (isImm)
        res = "#" +  std::to_string (imm);
    else if (isLabel)
        res = label;
    else if (isCondition) {
        const char *const s [nrConditions] = {"eq", "ne", "le", "ge", "lt", "gt"};
        res = s [cond];
    }
    return res;
}

bool TA64Operand::isValid () const {
    return isReg || isOffset || isIndex || isImm || isLabel || isCondition;
}


TA64Operation::TA64Operation (TA64Op op, std::vector<TA64Operand> &&operands, std::string &&comment):
  operation (op),
  operands (std::move (operands)),
  comment (std::move (comment)) {
}

void TA64Operation::appendString (std::vector<std::string> &code) const {
    static const std::map<TA64Op, std::string> opNames = {
        {TA64Op::adr, "adr"},
        {TA64Op::adrp, "adrp"},
        {TA64Op::ret, "ret"},
        {TA64Op::mov, "mov"},
        {TA64Op::movz, "movz"},
        {TA64Op::movk, "movk"},
        {TA64Op::movn, "movn"},
        {TA64Op::ldr, "ldr"},
        {TA64Op::cmp, "cmp"},
        {TA64Op::beq, "b.eq"},
        {TA64Op::bne, "b.ne"},
        {TA64Op::ble, "b.le"},
        {TA64Op::bge, "b.ge"},
        {TA64Op::blt, "b.lt"},
        {TA64Op::bgt, "b.gt"},
        {TA64Op::add, "add"},
        {TA64Op::adds, "adds"},
        {TA64Op::sub, "sub"},
        {TA64Op::lsr, "lsr"},
        {TA64Op::str, "str"},
        {TA64Op::orr, "orr"},
        {TA64Op::eor, "eor"},
        {TA64Op::mul, "mul"},
        {TA64Op::madd, "madd"},
        {TA64Op::and_, "and"},
        {TA64Op::ands, "ands"},
        {TA64Op::sdiv, "sdiv"},
        {TA64Op::lsl, "lsl"},
        {TA64Op::neg, "neg"},
        {TA64Op::mvn, "mvn"},
        {TA64Op::asr, "asr"},
        {TA64Op::b, "b"},
        {TA64Op::br, "br"},
        {TA64Op::bl, "bl"},
        {TA64Op::blr, "blr"},
        {TA64Op::stp, "stp"},
        {TA64Op::ldp, "ldp"},
        {TA64Op::cset, "cset"},
        {TA64Op::csel, "csel"},
        {TA64Op::ldrb, "ldrb"},
        {TA64Op::ldrsb, "ldrsb"},
        {TA64Op::ldrh, "ldrh"},
        {TA64Op::ldrsh, "ldrsh"},
        {TA64Op::ldrsw, "ldrsw"},
        {TA64Op::strb, "strb"},
        {TA64Op::strh, "strh"},
        {TA64Op::rev16, "rev16"},
        {TA64Op::subs, "subs"},
        {TA64Op::udiv, "udiv"},
        {TA64Op::msub, "msub"},
        {TA64Op::tbz, "tbz"},
        {TA64Op::cbz, "cbz"},
        {TA64Op::tbnz, "tbnz"},
        {TA64Op::cbnz, "cbnz"},
        {TA64Op::bls, "bls"},
        
        {TA64Op::fcmp, "fcmp"},
        {TA64Op::fadd, "fadd"},
        {TA64Op::fsub, "fsub"},
        {TA64Op::fmul, "fmul"},
        {TA64Op::fdiv, "fdiv"},
        {TA64Op::fneg, "fneg"},
        {TA64Op::fcvt, "fcvt"},
        {TA64Op::fmov, "fmov"},
        {TA64Op::scvtf, "scvtf"},
        {TA64Op::fabs, "fabs"},
        {TA64Op::fsqrt, "fsqrt"},

        // pseudo ops
        {TA64Op::li, "li"},
        {TA64Op::nop, "nop"},
        {TA64Op::data_dq, ".xword"}, 
        {TA64Op::aligncode, ". balign 64"},
        {TA64Op::ltorg, ".ltorg"},
        {TA64Op::comment, ""},
        {TA64Op::end, ""}};
        
    const std::string indent= std::string (8, ' ');
    if (operation == TA64Op::def_label)
        code.emplace_back (operands [0].makeString () + ":");
    else if (operation == TA64Op::comment)
        code.emplace_back ( "// " + comment);
    else if (operation == TA64Op::movk) {
        code.emplace_back (indent + "movk " + operands [0].makeString () + ", " + operands [1].makeString () + ", lsl " + operands [2].makeString ());
    } else if (operation == TA64Op::mcpy) {
        const std::string args = " [" + operands [0].makeString () + "]!, [" +  operands [1].makeString () + "]!, " + operands [2].makeString () + "!";
        code.emplace_back (indent + "cpyfp" + args);
        code.emplace_back (indent + "cpyfm" + args);
        code.emplace_back (indent + "cpyfe" + args);
    } else {
        std::string res = opNames.at (operation);
        for (const TA64Operand &op: operands)
            res.append (" " + op.makeString () + ",");
        if (res.back () == ',')
            res.pop_back ();
        if (!comment.empty ())
            res.append ("     // " + comment);
        code.emplace_back (indent + res);
    }
}
        
}  // namespace

