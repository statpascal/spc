#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace statpascal {

enum class TX64OpSize {bit_default, bit8, bit16, bit32, bit64};

enum class TX64Op {
    add, adc, addsd, and_, andpd, bt, call, cmp, comisd, cqo, cvtsd2ss, cvtsi2sd, cvtss2sd, dec,
    divsd, idiv, imul, inc, jc, je, 
    jg, jge, jl, jle, 	// signed
    jbe, jae, jb, ja, 	// unsigned
    jmp, jnc, jne, lea, leave, mov, movapd,
    movd, movq, movsx, movsxd, movzx, mulsd, neg, not_, or_, pop, push, psllq, psrlq, pxor,
    rep_movsb, ret, ror, seta, setae, setb, setbe, setc, setg, setge, setl, setle,
    setnz, setz, shl, shr, sar, sqrtsd, sub, subsd, test, xor_,
    // pseudo ops
    data_dq, data_diff_dq, data_dd, data_aorg, data_aorgs, def_label, comment, aligncode, end
};

enum class TX64Reg {
    rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15, 
    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, 
    none, nrRegs = none
};

const std::vector<std::string> 
    x64RegName   = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"},
    x64Reg8Name  = {"al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"},
    x64Reg16Name = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"},
    x64Reg32Name = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};

class TX64Operand {
public:
    TX64Operand ();
    
    TX64Operand (TX64Reg, TX64OpSize = TX64OpSize::bit_default);
    
    TX64Operand (TX64Reg base, TX64Reg index, std::uint8_t shift, ssize_t offset, TX64OpSize = TX64OpSize::bit_default);
    TX64Operand (TX64Reg base, ssize_t offset, TX64OpSize = TX64OpSize::bit_default);
    
    TX64Operand (ssize_t imm, TX64OpSize = TX64OpSize::bit_default);
    
    TX64Operand (std::string label, bool ripRelative = false, TX64OpSize = TX64OpSize::bit_default);
    
    TX64Operand (const TX64Operand &) = default;
    TX64Operand (TX64Operand &&) = default;
    TX64Operand &operator = (TX64Operand &&) = default;
    TX64Operand &operator = (const TX64Operand &) = default;
    
    std::string makeString () const;
    bool isValid () const;
    
    bool isReg, isPtr, isImm, isLabel;
    
    TX64Reg reg, base, index;
    std::uint8_t shift;
    ssize_t offset, imm;
    std::string label;
    bool ripRelative;    
    TX64OpSize opSize;
    
    std::string getOpSizeName () const;
    std::string getRegName (TX64Reg, TX64OpSize) const;
};

class TX64Operation {
public:
    TX64Operation (TX64Op, TX64Operand = TX64Operand (), TX64Operand = TX64Operand (), const std::string &comment = std::string ());
    
    TX64Operation (const TX64Operation &) = default;
    TX64Operation (TX64Operation &&) = default;
    
    std::string makeString () const;
    void outputCode (std::vector<std::uint8_t> &code, std::size_t offset);
    
    TX64Op operation;
    TX64Operand operand1, operand2;
    std::string comment;
    
private:
    void generateREX ();
    void generateREX_W ();
    void generateREX_R ();
    void generateREX_X ();
    void generateREX_B ();
    void setModRMReg (const TX64Operand &);
    void setPrefix (std::uint8_t);
    void appendBytes (std::vector<std::uint8_t> &code, std::uint64_t imm, int nrBytes);
    
    void output_RM_SIB (std::vector<std::uint8_t> &code, TX64Operand &op) ;
    void outputRegCode (std::vector<std::uint8_t> &code, std::uint8_t opcode, TX64Reg reg);
    int outputPrefixedOperation (std::vector<std::uint8_t> &code, std::uint8_t opCode, TX64OpSize opSize);
    
    void outputArithmetic (std::vector<std::uint8_t> &code, std::uint8_t opcode, std::uint8_t opcodeImm, std::uint8_t opcodeImmModRW);
    void outputUnary (std::vector<std::uint8_t> &code, std::uint8_t opcode, std::uint8_t opcodeModRW);
    void outputMOV (std::vector<std::uint8_t> &code);
    void outputMOVSXZ (std::vector<std::uint8_t> &code);
    void outputLEA (std::vector<std::uint8_t> &code);
    void outputPush (std::vector<std::uint8_t> &code);
    void outputPop (std::vector<std::uint8_t> &code);
    void outputImul (std::vector<std::uint8_t> &code);
    void outputShifts (std::vector<std::uint8_t> &code, std::uint8_t opcodeModRW);
    void outputSetCC (std::vector<std::uint8_t> &code);
    void outputBt (std::vector<std::uint8_t> &code);
    void outputJmpCall (std::vector<std::uint8_t> &code);
    void outputJcc (std::vector<std::uint8_t> &code);
    
    void outputSSE (std::vector<std::uint8_t> &code, std::uint8_t prefx, std::uint8_t opcode, TX64Operand &, TX64Operand &);
    
    void assembleInstruction (std::vector<std::uint8_t> &code, std::size_t offset);
    
    std::uint8_t prefix, rexPrefix, modRM, sib, bytesUsed;
    std::uint32_t disp;
    bool modRMValid, sibValid, operandSizePrefix, disp8Valid, disp32Valid, ripValid;
};

}
