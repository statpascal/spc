#include "x64asm.hpp"

#include <map>
#include <algorithm>
#include <limits>

namespace statpascal {

TX64Operand::TX64Operand ():
  isReg (false), isPtr (false), isImm (false), isLabel (false) {
}
  
TX64Operand::TX64Operand (TX64Reg reg, TX64OpSize opSize):
  isReg (true), isPtr (false), isImm (false), isLabel (false), reg (reg), opSize (opSize) {
}

TX64Operand::TX64Operand (TX64Reg base, TX64Reg index, std::uint8_t shift, ssize_t offset, TX64OpSize opSize):
  isReg (false), isPtr (true), isImm (false), isLabel (false), base (base), index (index), shift (shift), offset (offset), opSize (opSize) {
}

TX64Operand::TX64Operand (TX64Reg base, ssize_t offset, TX64OpSize opSize):
  TX64Operand (base, TX64Reg::none, 0, offset, opSize) {
}

TX64Operand::TX64Operand (ssize_t imm, TX64OpSize opSize):
  isReg (false), isPtr (false), isImm (true), isLabel (false), imm (imm), opSize (opSize) {
}

TX64Operand::TX64Operand (std::string label, bool ripRelative, TX64OpSize opSize):
  isReg (false), isPtr (false), isImm (false), isLabel (true), imm (0), label (label), ripRelative (ripRelative), opSize (opSize) {
}

std::string TX64Operand::getOpSizeName () const {
    static const std::map<TX64OpSize, std::string> sizeNames = {{TX64OpSize::bit_default, "qword"}, {TX64OpSize::bit8, "byte "}, {TX64OpSize::bit16, "word "}, {TX64OpSize::bit32, "dword "}, {TX64OpSize::bit64, "qword "}};
    return sizeNames.at (opSize);
}

std::string TX64Operand::getRegName (TX64Reg reg, TX64OpSize opSize) const {
    static const std::map<TX64OpSize, const std::vector<std::string> &> regNames = {
        {TX64OpSize::bit_default, x64RegName}, {TX64OpSize::bit8, x64Reg8Name}, {TX64OpSize::bit16, x64Reg16Name}, {TX64OpSize::bit32, x64Reg32Name}, {TX64OpSize::bit64, x64RegName}};
    return regNames.at (opSize)[static_cast<std::size_t> (reg)];
}

std::string TX64Operand::makeString () const {
    if (isReg)
        return getRegName (reg, opSize);
    if (isPtr) {
        std::string ret;
        if (base != TX64Reg::none)
            ret = getRegName (base, TX64OpSize::bit64);
        if (index != TX64Reg::none) {
            if (!ret.empty ())
                ret.push_back ('+');
            if (shift != 1)
                ret.append (std::to_string (shift) + '*');
            ret.append (getRegName (index, TX64OpSize::bit64));
        }
        if (offset > 0)
            ret.append ('+' + std::to_string (offset));
        else if (offset < 0)
            ret.append ('-' + std::to_string (-offset));
        return getOpSizeName () + "[" + ret + "]";
    }
    if (isImm)
        return std::to_string (imm);
    if (isLabel)
        return ripRelative ? getOpSizeName () + "[rel " + label + "]" : label;
    // empty operand
    return std::string ();
}

bool TX64Operand::isValid () const {
    return isReg || isPtr || isImm || isLabel;
}

TX64Operation::TX64Operation (TX64Op op, TX64Operand op1, TX64Operand op2, const std::string &comment):
  operation (op),
  operand1 (op1),
  operand2 (op2),
  comment (comment),
  prefix (0), rexPrefix (0), modRM (0), sib (0), bytesUsed (0),
  modRMValid (false), sibValid (false), operandSizePrefix (false), disp8Valid (false), disp32Valid (false), ripValid (false) {
}

std::string TX64Operation::makeString () const {
    static const std::map<TX64Op, std::string> opNames = {
        {TX64Op::add, "add"}, {TX64Op::adc, "adc"}, {TX64Op::addsd, "addsd"}, {TX64Op::and_, "and"}, {TX64Op::andpd, "andpd"}, {TX64Op::bt, "bt"}, {TX64Op::call, "call"}, {TX64Op::cmp, "cmp"}, {TX64Op::comisd, "comisd"}, {TX64Op::cqo, "cqo"}, {TX64Op::cvtsd2ss, "cvtsd2ss"}, {TX64Op::cvtsi2sd, "cvtsi2sd"}, {TX64Op::cvtss2sd, "cvtss2sd"}, 
        {TX64Op::dec, "dec"}, {TX64Op::divsd, "divsd"}, {TX64Op::idiv, "idiv"}, {TX64Op::imul, "imul"}, {TX64Op::inc, "inc"}, {TX64Op::jc, "jc"}, {TX64Op::je, "je"}, {TX64Op::jg, "jg"}, {TX64Op::jge, "jge"}, {TX64Op::jl, "jl"}, {TX64Op::jle, "jle"}, {TX64Op::jbe, "jbe"}, {TX64Op::jmp, "jmp"}, {TX64Op::jnc, "jnc"}, {TX64Op::jne, "jne"}, 
        {TX64Op::jae, "jae"}, {TX64Op::jb, "jb"}, {TX64Op::ja, "ja"}, {TX64Op::lea, "lea"}, 
        {TX64Op::leave, "leave"}, {TX64Op::mov, "mov"}, {TX64Op::movapd, "movapd"}, {TX64Op::movd, "movd"}, {TX64Op::movq, "movq"}, {TX64Op::movsx, "movsx"}, {TX64Op::movsxd, "movsxd"}, {TX64Op::movzx, "movzx"}, {TX64Op::mulsd, "mulsd"}, {TX64Op::neg, "neg"}, {TX64Op::not_, "not"}, {TX64Op::or_, "or"}, {TX64Op::pop, "pop"}, 
        {TX64Op::push, "push"}, {TX64Op::pxor, "pxor"}, {TX64Op::rep_movsb, "rep movsb"}, {TX64Op::ret, "ret"}, {TX64Op::ror, "ror"}, {TX64Op::seta, "seta"}, {TX64Op::setae, "setae"}, {TX64Op::setb, "setb"}, {TX64Op::setbe, "setbe"}, {TX64Op::setc, "setc"}, {TX64Op::setg, "setg"}, {TX64Op::setge, "setge"}, 
        {TX64Op::setl, "setl"}, {TX64Op::setle, "setle"}, {TX64Op::setnz, "setnz"}, {TX64Op::setz, "setz"}, {TX64Op::shl, "shl"}, {TX64Op::shr, "shr"}, {TX64Op::sar, "sar"}, {TX64Op::sqrtsd, "sqrtsd"}, {TX64Op::sub, "sub"}, {TX64Op::subsd, "subsd"}, {TX64Op::test, "test"}, {TX64Op::xor_, "xor"},
        // pseudo ops
        {TX64Op::data_dq, "dq"}, {TX64Op::data_diff_dq, "dq"}, {TX64Op::data_dd, "dd"}, {TX64Op::data_aorg, "aorg"}, {TX64Op::data_aorgs, "aorgs"}, {TX64Op::aligncode, "align"}};
        
    if (operation == TX64Op::def_label)
        return operand1.makeString () + ":";
    else if (operation == TX64Op::data_diff_dq)
        return "dq " + operand1.makeString ()+ " - " + operand2.makeString ();
    else if (operation == TX64Op::comment)
        return "; " + comment;
    else {
        std::string res = opNames.at (operation);
        if (operand1.isValid ()) 
            res.append (" " + operand1.makeString ());
        if (operand2.isValid ()) 
            res.append (", " + operand2.makeString ());
        if (!comment.empty ()) {
            res.resize (40, ' ');
            res.append ("; " + comment);
        }
        return "        " + res;
    }
}

void TX64Operation::generateREX () {
    rexPrefix |= 0x40;
}

void TX64Operation::generateREX_W () {
    rexPrefix |= 0x48;
}

void TX64Operation::generateREX_R () {
    rexPrefix |= 0x44;
}

void TX64Operation::generateREX_X () {
    rexPrefix |= 0x42;
}

void TX64Operation::generateREX_B () {
    rexPrefix |= 0x41;
}

void TX64Operation::setModRMReg (const TX64Operand &operand) {
    int regNr = static_cast<int> (operand.reg) & 0x0F;	// handle XMM
    modRM |= (regNr & 0x07) << 3;
    if (regNr >= 8)
        generateREX_R ();
    if (regNr >= 4 && operand.opSize == TX64OpSize::bit8)
        generateREX ();
}

void TX64Operation::setPrefix (std::uint8_t p) {
    prefix = p;
}

void TX64Operation::appendBytes (std::vector<std::uint8_t> &code, std::uint64_t imm, int nrBytes) {
    while (nrBytes-- > 0) {
        code.push_back (imm & 0xff);
        imm >>= 8;
    }
}

void TX64Operation::outputRegCode (std::vector<std::uint8_t> &code, std::uint8_t opcode, TX64Reg reg) {
    if (static_cast<int> (reg) >= 8)
        generateREX_B ();
    code.push_back (opcode + (static_cast<int> (reg) & 0x07));
}

void TX64Operation::output_RM_SIB (std::vector<std::uint8_t> &code, TX64Operand &op) {
    static const std::vector<TX64Reg> baseSIBRequired = {TX64Reg::rsp, TX64Reg::rbp, TX64Reg::r12, TX64Reg::r13};

    if (op.isReg) {
        int regNr = static_cast<int> (op.reg) & 0x0f;	// handle XMM
        modRM |= 0xC0 | (regNr & 0x07);
        if (regNr >= 8)
            generateREX_B ();
        else if (regNr >= 4 && op.opSize == TX64OpSize::bit8)
            generateREX ();
    }
    if (op.isLabel && op.ripRelative) {
        ripValid = true;
        disp32Valid = true;
        modRM |= static_cast<int> (TX64Reg::rbp);
        disp = op.imm - bytesUsed;
    } else if (op.isPtr) {
        if (op.index != TX64Reg::none || op.base == TX64Reg::rsp || op.base == TX64Reg::r12)
            sibValid = true;
        if (op.index == TX64Reg::none && op.base == TX64Reg::none) {
            sibValid = true;
            disp32Valid = true;
            disp = op.offset;
        } else if (op.base == TX64Reg::rbp || op.base == TX64Reg::r13 || op.offset) {
            if (op.offset >= -128 && op.offset <= 127) {
                disp8Valid = true;
                modRM |= 0x40;
            } else {
                disp32Valid = true;
                modRM |= 0x80;
            }
            disp = op.offset;
        }
        if (sibValid) {
            modRM |= static_cast<int> (TX64Reg::rsp);
            if (op.index == TX64Reg::none)
                sib |= (static_cast<int> (TX64Reg::rsp) & 0x07) << 3;
            else {
                sib |= (static_cast<int> (op.index) & 0x07) << 3;
                if (static_cast<int> (op.index) >= 8)
                    generateREX_X ();
            }
            if (op.base == TX64Reg::none)
                sib |= (static_cast<int> (TX64Reg::rbp) & 0x07);
            else {
                sib |= (static_cast<int> (op.base) & 0x07);
                if (static_cast<int> (op.base) >= 8)
                    generateREX_B ();
            }
                
            switch (op.shift) {
                case 2:
                    sib |= 0x40; 
                    break;
                case 4:
                    sib |= 0x80;
                    break;
                case 8:
                    sib |= 0xC0;
                    break;
            }
        } else
            modRM |= static_cast<int> (op.base) & 0x07;
            
        if (static_cast<int> (op.base) >= 8)
            generateREX_B ();
    }

    code.push_back (modRM);
    if (sibValid)
        code.push_back (sib);
    if (disp8Valid)
        code.push_back (disp);
    if (disp32Valid)
        appendBytes (code, disp, 4);
}

int TX64Operation::outputPrefixedOperation (std::vector<std::uint8_t> &code, std::uint8_t opCode, TX64OpSize opSize) {
    switch (opSize) {
        case TX64OpSize::bit8:
            code.push_back (opCode);
            return 1;
        case TX64OpSize::bit16:
            operandSizePrefix = true;
            code.push_back (opCode + 1);
            return 2;
        case TX64OpSize::bit32:
            code.push_back (opCode + 1);
            return 4;
        case TX64OpSize::bit_default:
        case TX64OpSize::bit64:
            generateREX_W ();
            code.push_back (opCode + 1);
            return 4;
    }
    return 0;
}

void TX64Operation::outputArithmetic (std::vector<std::uint8_t> &code, std::uint8_t opcode, std::uint8_t opcodeImm, std::uint8_t opcodeImmModRW) {
    if (operand2.isImm) {
        modRM |= opcodeImmModRW << 3;
        if (opcodeImm == 0x80 && operand2.imm >= -128 && operand2.imm <= 127 && operand1.opSize != TX64OpSize::bit8) {
            outputPrefixedOperation (code, 0x82, operand1.opSize);
            output_RM_SIB (code, operand1);
            appendBytes (code, operand2.imm, 1);
        } else {
            int immBytes = outputPrefixedOperation (code, opcodeImm, operand1.opSize);
            output_RM_SIB (code, operand1);
            appendBytes (code, operand2.imm, immBytes);
        }
    } else if (operand2.isPtr) {
        setModRMReg (operand1);
        outputPrefixedOperation (code, opcode + 2, operand2.opSize);
        output_RM_SIB (code, operand2);
    } else {
        setModRMReg (operand2);
        outputPrefixedOperation (code, opcode, operand1.opSize);
        output_RM_SIB (code, operand1);
    }
}

void TX64Operation::outputUnary (std::vector<std::uint8_t> &code, std::uint8_t opcode, std::uint8_t opcodeModRW) {
    modRM |= opcodeModRW << 3;
    outputPrefixedOperation (code, opcode, operand1.opSize);
    output_RM_SIB (code, operand1);
}

void TX64Operation::outputMOV (std::vector<std::uint8_t> &code) {
    if (operand1.isPtr && operand1.base == TX64Reg::none && static_cast<std::uint64_t> (operand1.offset) > std::numeric_limits<std::uint32_t>::max () && operand2.isReg && operand2.reg == TX64Reg::rax) {
        generateREX_W ();
        code.push_back (0xA3);
        appendBytes (code, operand1.offset, 8);
    } else if (operand1.isReg && operand2.isImm)  {
        if (0 <= operand2.imm && operand2.imm <= std::numeric_limits<std::uint32_t>::max ()) {
            outputRegCode (code, 0xB8, operand1.reg);
            appendBytes (code, operand2.imm, 4);
        } else if (operand2.imm >= std::numeric_limits<std::int32_t>::min () && operand2.imm <= std::numeric_limits<std::int32_t>::max ())
            outputArithmetic (code, 0x88, 0xC6, 0x00);
        else {
            generateREX_W ();
            outputRegCode (code, 0xB8, operand1.reg);
            appendBytes (code, operand2.imm, 8);
        }
    } else 
        outputArithmetic (code, 0x88, 0xC6, 0x00);
}

void TX64Operation::outputMOVSXZ (std::vector<std::uint8_t> &code) {
    // Only 64-bit dest supported
    generateREX_W ();
    if (operation == TX64Op::movsxd)
        code.push_back (0x63);
    else {
        code.push_back (0x0F);
        code.push_back ((operation == TX64Op::movzx ? 0xB6 : 0xBE) + (operand2.opSize == TX64OpSize::bit16)); 
    }
    setModRMReg (operand1);
    output_RM_SIB (code, operand2);
}

void TX64Operation::outputLEA (std::vector<std::uint8_t> &code) {
    setModRMReg (operand1);
    generateREX_W ();
    code.push_back (0x8D);
    output_RM_SIB (code, operand2);
}

void TX64Operation::outputPush (std::vector<std::uint8_t> &code) {
    if (operand1.isReg) {
        outputRegCode (code, 0x50, operand1.reg);
    } else if (operand1.isPtr) 
        // TODO: only 16, 64 bit allowed
        outputUnary (code, 0xFE, 0x06);
    else if (operand1.isImm) {
        // TODO: optimeize 8/16 bit
        code.push_back (0x68);
        appendBytes (code, operand1.imm, 4);
    }
}

void TX64Operation::outputPop (std::vector<std::uint8_t> &code) {
    if (operand1.isReg) {
        outputRegCode (code, 0x58, operand1.reg);
    } else if (operand1.isPtr) 
        // TODO: only 16, 64 bit allowed
        outputUnary (code, 0x8E, 0x00);
}

void TX64Operation::outputImul (std::vector<std::uint8_t> &code) {
    // Only used:
    // imul r64, r/m64
    // imul r64, r/m64, imm32
    generateREX_W ();
    setModRMReg (operand1);
    if (operand2.isImm) {
        // TODO: shorter form for imm8
        code.push_back (0x69);
        output_RM_SIB (code, operand1);
        appendBytes (code, operand2.imm, 4);
    } else {
        code.push_back (0x0F); code.push_back (0xAF);
        output_RM_SIB (code, operand2);
    }
}

void TX64Operation::outputShifts (std::vector<std::uint8_t> &code, std::uint8_t opcodeModRW) {
    if (!operand2.isValid () || (operand2.isImm && operand2.imm == 1))
        outputUnary (code, 0xD0, opcodeModRW);
    else if (operand2.isReg && operand2.reg == TX64Reg::rcx)
        outputUnary (code, 0xD2, opcodeModRW);
    else if (operand2.isImm) {
        outputUnary (code, 0xC0, opcodeModRW);
        appendBytes (code, operand2.imm, 1);
    }
}

void TX64Operation::outputSetCC (std::vector<std::uint8_t> &code) {
    const std::map<TX64Op, std::uint8_t> opcode = {
        {TX64Op::seta, 0x97}, {TX64Op::setae, 0x93}, {TX64Op::setb, 0x92}, {TX64Op::setbe, 0x96}, {TX64Op::setc, 0x92}, {TX64Op::setg, 0x9f}, {TX64Op::setge, 0x9d}, {TX64Op::setl, 0x9C}, {TX64Op::setle, 0x9E}, {TX64Op::setnz, 0x95}, {TX64Op::setz, 0x94}
    };
    code.push_back (0x0F); code.push_back (opcode.at (operation));
    output_RM_SIB (code, operand1);
}

void TX64Operation::outputBt (std::vector<std::uint8_t> &code) {
    code.push_back (0x0F);
    if (operand2.isImm) {
        outputUnary (code, 0xB9, 0x04);
        appendBytes (code, operand2.imm, 1);
    } else {
        outputPrefixedOperation (code, 0xA2, operand1.opSize);
        setModRMReg (operand2);
        output_RM_SIB (code, operand1);
    }
}

void TX64Operation::outputJmpCall (std::vector<std::uint8_t> &code) {
    if (operand1.isReg) {
        code.push_back (0xFF);
        modRM |= (operation == TX64Op::jmp ? 0x04 : 0x02)  << 3;
        output_RM_SIB (code, operand1);
    } else if (operation == TX64Op::call || operand1.imm >= 0 || operand1.imm <= -127) {
        code.push_back (operation == TX64Op::call ? 0xE8 : 0xE9);
        appendBytes (code, operand1.imm - 5, 4);
    } else {
        code.push_back (0xEB);
        appendBytes (code, operand1.imm - 2, 1);
    }
}

void TX64Operation::outputJcc (std::vector<std::uint8_t> &code) {
    const std::map<TX64Op, std::uint8_t> opcode = {
        {TX64Op::jc, 0x82}, {TX64Op::je, 0x84},
        {TX64Op::jg, 0x8F}, {TX64Op::jge, 0x8D}, {TX64Op::jl, 0x8C}, {TX64Op::jle, 0x8E},
        {TX64Op::jbe, 0x86}, {TX64Op::jae, 0x83}, {TX64Op::jb, 0x82}, {TX64Op::ja, 0x87},
        {TX64Op::jnc, 0x83}, {TX64Op::jne, 0x85}
    };
    if (operand1.imm < 0 && operand1.imm > -127) {
        code.push_back (opcode.at (operation) - 0x10);
        appendBytes (code, operand1.imm - 2, 1);
    } else {
        code.push_back (0x0F); code.push_back (opcode.at (operation));
        appendBytes (code, operand1.imm - 6, 4);
    }
}

void TX64Operation::outputSSE (std::vector<std::uint8_t> &code, std::uint8_t prefix, std::uint8_t opcode, TX64Operand &op1, TX64Operand &op2) {
    setPrefix (prefix);
    code.push_back (0x0F);
    code.push_back (opcode);
    setModRMReg (op1);
    output_RM_SIB (code, op2);
}

void TX64Operation::assembleInstruction (std::vector<std::uint8_t> &code, std::size_t offset) {
    if (operation >= TX64Op::seta && operation <= TX64Op::setz)
        outputSetCC (code);
    else if (operation >= TX64Op::jc && operation <= TX64Op::jne && operation != TX64Op::jmp)
        outputJcc (code);
    else switch (operation) {
        case TX64Op::add:
            outputArithmetic (code, 0x00, 0x80, 0x00);
            break;
        case TX64Op::or_:
            outputArithmetic (code, 0x08, 0x80, 0x01);
            break;
        case TX64Op::adc:
            outputArithmetic (code, 0x10, 0x80, 0x02);
            break;
        case TX64Op::and_:
            outputArithmetic (code, 0x20, 0x80, 0x04);
            break;
        case TX64Op::sub:
            outputArithmetic (code, 0x28, 0x80, 0x05);
            break;
        case TX64Op::xor_:
            outputArithmetic (code, 0x30, 0x80, 0x06);
            break;
        case TX64Op::cmp:
            outputArithmetic (code, 0x38, 0x80, 0x07);
            break;
        case TX64Op::test:
            // TODO: do not allow swapping of operands
            outputArithmetic (code, 0x84, 0xf6, 0x00);
            break;
        case TX64Op::mov:
            outputMOV (code);
            break;
        case TX64Op::movsx:
        case TX64Op::movsxd:
        case TX64Op::movzx:
            outputMOVSXZ (code);
            break;
        case TX64Op::lea:
            outputLEA (code);
            break;
        case TX64Op::not_:
            outputUnary (code, 0xF6, 0x02);
            break;
        case TX64Op::neg:
            outputUnary (code, 0xF6, 0x03);
            break;
        case TX64Op::idiv:
            outputUnary (code, 0xF6, 0x07);
            break;
        case TX64Op::inc:
            outputUnary (code, 0xFE, 0x00);
            break;
        case TX64Op::dec:
            outputUnary (code, 0xFE, 0x01);
            break;
        case TX64Op::imul:
            outputImul (code);
            break;
        case TX64Op::push:
            outputPush (code);
            break;
        case TX64Op::pop:
            outputPop (code);
            break;
        case TX64Op::cqo:
            generateREX_W ();
            code.push_back (0x99);
            break;
        case TX64Op::leave:
            code.push_back (0xC9);
            break;
        case TX64Op::ret:
            code.push_back (0xC3);
            break;
        case TX64Op::rep_movsb:
            code.push_back (0xF3); code.push_back (0xA4);
            break;
        case TX64Op::shl:
            outputShifts (code, 0x04);
            break;
        case TX64Op::shr:
            outputShifts (code, 0x05);
            break;
        case TX64Op::sar:
            outputShifts (code, 0x07);
            break;
        case TX64Op::ror:
            outputShifts (code, 0x01);
            break;
        case TX64Op::bt:
            outputBt (code);
            break;
        case TX64Op::addsd:
            outputSSE (code, 0xF2, 0x58, operand1, operand2);
            break;
        case TX64Op::subsd:
            outputSSE (code, 0xF2, 0x5C, operand1, operand2);
            break;
        case TX64Op::mulsd:
            outputSSE (code, 0xF2, 0x59, operand1, operand2);
            break;
        case TX64Op::divsd:
            outputSSE (code, 0xF2, 0x5E, operand1, operand2);
            break;
        case TX64Op::sqrtsd:
            outputSSE (code, 0xF2, 0x51, operand1, operand2);
            break;
        case TX64Op::cvtsi2sd:
            outputSSE (code, 0xF2, 0x2A, operand1, operand2);
            if (operand2.opSize != TX64OpSize::bit32)
                generateREX_W ();
            break;
        case TX64Op::cvtsd2ss:
            outputSSE (code, 0xF2, 0x5A, operand1, operand2);
            break;
        case TX64Op::cvtss2sd:
            outputSSE (code, 0xF3, 0x5A, operand1, operand2);
            break;
        case TX64Op::movapd:
            outputSSE (code, 0x66, 0x28, operand1, operand2);
            break;
        case TX64Op::comisd:
            outputSSE (code, 0x66, 0x2F, operand1, operand2);
            break;
        case TX64Op::andpd:
            outputSSE (code, 0x66, 0x54, operand1, operand2);
            break;
        case TX64Op::pxor:
            outputSSE (code, 0x66, 0xEF, operand1, operand2);
            break;
        case TX64Op::movd:
        case TX64Op::movq:
            if (operand1.isReg && operand1.reg >= TX64Reg::xmm0)
                outputSSE (code, 0x66, 0x6E, operand1, operand2);
            else
                outputSSE (code, 0x66, 0x7E, operand2, operand1);
            if (operation == TX64Op::movq)
                generateREX_W ();
            break;
        case TX64Op::call:
        case TX64Op::jmp:
            outputJmpCall (code);
            break;
        case TX64Op::data_dd:
            appendBytes (code, operand1.imm, 4);
            break;
        case TX64Op::data_dq:
            appendBytes (code, operand1.imm, 8);
            break;
        case TX64Op::data_diff_dq:
            appendBytes (code, operand1.imm - operand2.imm, 8);
            break;
        case TX64Op::aligncode:
            code.resize (((offset + operand1.imm - 1) & ~(operand1.imm - 1)) - offset, 0x90);
            break;
        default:
            break;
    }
 
    if (rexPrefix)
        code.insert (code.begin (), rexPrefix);
    if (operandSizePrefix)
        code.insert (code.begin (), 0x66);
    if (prefix)
        code.insert (code.begin (), prefix);
}

void TX64Operation::outputCode (std::vector<std::uint8_t> &code, std::size_t offset) {
    assembleInstruction (code, offset);
    bytesUsed = code.size ();
    if (ripValid) {
        code.clear ();
        assembleInstruction (code, offset);	
    }
}
        
}

#ifdef TEST_ASM

#include <iostream>
#include <fstream>
#include <iterator>

using namespace statpascal;

std::ofstream ftext, fbin;

const
    TX64Operand rax (TX64Reg::rax, TX64OpSize::bit64),
                rsi (TX64Reg::rsi, TX64OpSize::bit64),
                rbp (TX64Reg::rbp, TX64OpSize::bit64),
                rsp (TX64Reg::rsp, TX64OpSize::bit64),
                r12 (TX64Reg::r12, TX64OpSize::bit64),
                r15 (TX64Reg::r15, TX64OpSize::bit64),
                none (TX64Reg::none),
                rbp_i1 (TX64Reg::rbp, TX64Reg::none, 1, 0, TX64OpSize::bit64),
                rbp_i2 (TX64Reg::rbp, TX64Reg::rax, 1, 0, TX64OpSize::bit64),
                rbp_i3 (TX64Reg::rbp, TX64Reg::r15, 1, 0, TX64OpSize::bit64),
                rbp_i4 (TX64Reg::rbp, TX64Reg::r12, 2, 0, TX64OpSize::bit64),
                rbp_i5 (TX64Reg::rbp, TX64Reg::none, 1, 0x1234, TX64OpSize::bit64),
                rbp_i6 (TX64Reg::rbp, TX64Reg::rsi, 4, 0x1234, TX64OpSize::bit64);
                
const 
    std::vector<TX64Operand> regs {rax, rsi, rbp, rsp, r12, r15};
    std::vector<TX64Operand> regmem {rax, rsi, rbp, rsp, r12, r15, rbp_i1, rbp_i2, rbp_i3, rbp_i4, rbp_i5, rbp_i6};
                
                

void test (TX64Operation op) {
    std::vector<std::uint8_t> code;
    op.outputCode (code, 0);
//    std::cout << std::hex;
//    std::copy (code.begin (), code.end (), std::ostream_iterator<std::uint32_t> (std::cout, " "));
    std::copy (code.begin (), code.end (), std::ostreambuf_iterator<char> (fbin));
    ftext << op.makeString () << std::endl;
}

void genArith (TX64Op op) {
    for (const TX64Operand &op1: regs)
        for (const TX64Operand &op2: regmem) {
            test (TX64Operation (op, op1, op2));
            test (TX64Operation (op, op2, op1));
        }
}

void genSimple (TX64Op op) {
    for (const TX64Operand &op1: regmem)
        test (TX64Operation (op, op1));
}


int main () {
    ftext.open ("/tmp/x64asm_test.asm");
    fbin.open ("/tmp/x64asm_test.bin");
    ftext << "bits 64" << std::endl;

/*  Compare results:
    diff <(ndisasm -b64 x64asm_test.bin | awk '{ print substr ($0, 29) }') <(nasm -O0 x64asm_test.asm -o /tmp/test.bin ; ndisasm -b 64 /tmp/test.bin | awk '{ print substr ($0, 29) }')
*/    

    for (TX64Op op: {TX64Op::add, TX64Op::xor_, TX64Op::or_, TX64Op::adc, TX64Op::and_, TX64Op::sub, TX64Op::cmp})
        genArith (op);

    for (TX64Op op: {TX64Op::neg, TX64Op::not_, TX64Op::inc, TX64Op::dec, TX64Op::push, TX64Op::pop})
        genSimple (op);

    test (TX64Operation (TX64Op::add, TX64Operand (TX64Reg::r15, TX64OpSize::bit16), TX64Operand (-5)));
    test (TX64Operation (TX64Op::add, TX64Operand (TX64Reg::rax, TX64OpSize::bit64), TX64Operand (6)));
    test (TX64Operation (TX64Op::add, TX64Operand (TX64Reg::rbx, TX64Reg::none, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::xor_, TX64Operand (TX64Reg::r14, TX64Reg::r15, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::add, TX64Operand (TX64Reg::rbp, TX64Reg::none, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::or_, TX64Operand (TX64Reg::r13, TX64Reg::none, 1, 0x1234, TX64OpSize::bit32), TX64Operand (0x56789abc)));
    test (TX64Operation (TX64Op::add, TX64Operand (TX64Reg::rbp, TX64Reg::rax, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::adc, TX64Operand (TX64Reg::rax), TX64Operand (TX64Reg::rbx)));
    test (TX64Operation (TX64Op::add, TX64Operand (TX64Reg::rbp, TX64Reg::rax, 4, 8, TX64OpSize::bit16), TX64Operand (TX64Reg::r13, TX64OpSize::bit16)));
    test (TX64Operation (TX64Op::cmp, TX64Operand (TX64Reg::r13), TX64Operand (TX64Reg::rbp, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::lea, TX64Operand (TX64Reg::r12), TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::test, TX64Operand (TX64Reg::rax), TX64Operand (TX64Reg::rbx)));
    test (TX64Operation (TX64Op::test, TX64Operand (TX64Reg::r14, TX64Reg::r15, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::neg, TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::not_, TX64Operand (TX64Reg::rbp, TX64Reg::r13, 4, 0x12345678, TX64OpSize::bit32)));
    test (TX64Operation (TX64Op::inc, TX64Operand (TX64Reg::rax)));
    test (TX64Operation (TX64Op::inc, TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::dec, TX64Operand (TX64Reg::rbp, TX64Reg::r13, 4, 0x12345678, TX64OpSize::bit32)));
    test (TX64Operation (TX64Op::idiv, TX64Operand (TX64Reg::rax)));
    test (TX64Operation (TX64Op::idiv, TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::push, TX64Operand (TX64Reg::rbx)));
    test (TX64Operation (TX64Op::push, TX64Operand (TX64Reg::r15)));
    test (TX64Operation (TX64Op::push, TX64Operand (0x1234)));
    test (TX64Operation (TX64Op::push, TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::pop, TX64Operand (TX64Reg::rbx)));
    test (TX64Operation (TX64Op::pop, TX64Operand (TX64Reg::r15)));
    test (TX64Operation (TX64Op::pop, TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::cqo));
    test (TX64Operation (TX64Op::leave));
    test (TX64Operation (TX64Op::ret));
    test (TX64Operation (TX64Op::imul, TX64Operand (TX64Reg::rax), TX64Operand (TX64Reg::rbx)));
    test (TX64Operation (TX64Op::imul, TX64Operand (TX64Reg::r15), TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::imul, TX64Operand (TX64Reg::rax), TX64Operand (0x12345678)));
    test (TX64Operation (TX64Op::shl, TX64Operand (TX64Reg::rax), 1));
    test (TX64Operation (TX64Op::shl, TX64Operand (TX64Reg::rax), TX64Operand (TX64Reg::rcx, TX64OpSize::bit8)));
    test (TX64Operation (TX64Op::shl, TX64Operand (TX64Reg::rax), TX64Operand (5)));
    test (TX64Operation (TX64Op::shl, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit8), 1));
    test (TX64Operation (TX64Op::shl, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit16), 1));
    test (TX64Operation (TX64Op::shl, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit32), 1));
    test (TX64Operation (TX64Op::shl, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit64), 1));
    test (TX64Operation (TX64Op::sar, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit64), 1));
    test (TX64Operation (TX64Op::shr, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit64), 1));
    test (TX64Operation (TX64Op::ror, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit64), 1));
    test (TX64Operation (TX64Op::setg, TX64Operand (TX64Reg::r15, TX64OpSize::bit8)));
    test (TX64Operation (TX64Op::setz, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit8)));
    test (TX64Operation (TX64Op::bt, TX64Operand (TX64Reg::r15, TX64Reg::rax, 4, 8, TX64OpSize::bit32), TX64Operand (TX64Reg::r12, TX64OpSize::bit32)));
    test (TX64Operation (TX64Op::bt, TX64Operand (TX64Reg::r12), TX64Operand (0x12)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::r15, TX64OpSize::bit16), TX64Operand (-5)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::rax, TX64OpSize::bit64), TX64Operand (6)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::rbx, TX64Reg::none, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::r14, TX64Reg::r15, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::rbp, TX64Reg::none, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::r13, TX64Reg::none, 1, 0x1234, TX64OpSize::bit32), TX64Operand (0x56789abc)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::rbp, TX64Reg::rax, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::rax), TX64Operand (TX64Reg::rbx)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::rbp, TX64Reg::rax, 4, 8, TX64OpSize::bit16), TX64Operand (TX64Reg::r13, TX64OpSize::bit16)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::r13), TX64Operand (TX64Reg::rbp, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::r12), TX64Operand (TX64Reg::r13, TX64Reg::rax, 4, 8)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::rax), TX64Operand (TX64Reg::rbx)));
    test (TX64Operation (TX64Op::mov, TX64Operand (TX64Reg::r14, TX64Reg::r15, 1, 0, TX64OpSize::bit8), TX64Operand (5)));
    test (TX64Operation (TX64Op::mov, TX64Reg::r14, 0x123456789abcdef));
    test (TX64Operation (TX64Op::movzx, TX64Operand (TX64Reg::r14), TX64Operand (TX64Reg::rbp, TX64Reg::r15, 4 , 4, TX64OpSize::bit8)));
    test (TX64Operation (TX64Op::movsx, TX64Operand (TX64Reg::r14), TX64Operand (TX64Reg::rbp, TX64Reg::r15, 4 , 4, TX64OpSize::bit16)));
    test (TX64Operation (TX64Op::movsxd, TX64Reg::rax, TX64Operand (TX64Reg::rax, TX64OpSize::bit32)));
    test (TX64Operation (TX64Op::addsd, TX64Reg::xmm3, TX64Reg::xmm14));
}

#endif
