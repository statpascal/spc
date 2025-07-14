#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include <numeric>
#include <unordered_map>

#include <malloc.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "vectorjit.hpp"

namespace statpascal {

#ifdef USE_VECTOR_JIT

#include "vectorjit.asm"

const std::string asmFile = "/tmp/tmp.asm";
const std::string binFile = "/tmp/tmp.bin";

TVectorJIT::TVectorJIT ():
  loadFirst ({
      {TPCode::TScalarTypeCode::s64,    "mov rax, [r10]"},
      {TPCode::TScalarTypeCode::s32,    "movsx rax, dword [r10]"},
      {TPCode::TScalarTypeCode::s16,    "movsx rax, word [r10]"},
      {TPCode::TScalarTypeCode::s8,     "movsx rax, byte [r10]"},
      {TPCode::TScalarTypeCode::u8,     "movzx rax, byte [r10]"},
      {TPCode::TScalarTypeCode::u16,    "movzx rax, word [r10]"},
      {TPCode::TScalarTypeCode::u32,    "mov eax, dword [r10]"},
      {TPCode::TScalarTypeCode::single, "movss xmm0, [r10]\n"
                                        "cvtss2sd xmm0, xmm0"},
      {TPCode::TScalarTypeCode::real,   "movsd xmm0, [r10]"}
  }),
  loadSecond ({
      {TPCode::TScalarTypeCode::s64,    "mov rbx, [r11]"},
      {TPCode::TScalarTypeCode::s32,    "movsx rbx, dword [r11]"},
      {TPCode::TScalarTypeCode::s16,    "movsx rbx, word [r11]"},
      {TPCode::TScalarTypeCode::s8,     "movsx rbx, byte [r11]"},
      {TPCode::TScalarTypeCode::u8,     "movzx rbx, byte [r11]"},
      {TPCode::TScalarTypeCode::u16,    "movzx rbx, word [r11]"},
      {TPCode::TScalarTypeCode::u32,    "mov ebx, dword [r11]"},
      {TPCode::TScalarTypeCode::single, "movss xmm1, [r11] \n"
                                        "cvtss2sd xmm1, xmm1"},
      {TPCode::TScalarTypeCode::real,   "movsd xmm1, [r11]"}
  }),
  intOp ({
      {TPCode::TOperation::AddVec,    "add rax, rbx \n"
                                      "jno %%op_success \n"
                                      "mov r14, 1 \n"
                                      "%%op_success: \n"},
      {TPCode::TOperation::SubVec,    "sub rax, rbx \n"
                                      "jno %%op_success \n"
                                      "mov r14, 1 \n"
                                      "%%op_success: \n"},
      {TPCode::TOperation::OrVec,     "or rax, rbx"},
      {TPCode::TOperation::AndVec,    "and rax, rbx"},
      {TPCode::TOperation::XorVec,    "xor rax, rbx"},
      {TPCode::TOperation::MulVec,    "imul rax, rbx \n"
                                      "jno %%op_success \n"
                                      "mov r14, 1 \n"
                                      "%%op_success: \n"},
      {TPCode::TOperation::DivIntVec, "cqo \n"
                                      "idiv rbx"},
      {TPCode::TOperation::ModVec,    "cqo \n"
                                      "idiv rbx \n"
                                      "mov rax, rdx"}
  }),
  doubleOp ({
      {TPCode::TOperation::AddVec, "addsd xmm0, xmm1"},
      {TPCode::TOperation::SubVec, "subsd xmm0, xmm1"},
      {TPCode::TOperation::MulVec, "mulsd xmm0, xmm1"},
      {TPCode::TOperation::DivVec, "divsd xmm0, xmm1"}
  }),
  storeCompareIntOp ({
      {TPCode::TOperation::EqualVec, "sete [r8]"},
      {TPCode::TOperation::NotEqualVec, "setne [r8]"},
      {TPCode::TOperation::LessVec, "setl [r8]"},
      {TPCode::TOperation::GreaterVec, "setg [r8]"},
      {TPCode::TOperation::LessEqualVec, "setle [r8]"},
      {TPCode::TOperation::GreaterEqualVec, "setge [r8]"}
  }),
  storeCompareDoubleOp ({
      {TPCode::TOperation::EqualVec, "sete [r8]"},
      {TPCode::TOperation::NotEqualVec, "setne [r8]"},
      {TPCode::TOperation::LessVec, "setb [r8]"},
      {TPCode::TOperation::GreaterVec, "seta [r8]"},
      {TPCode::TOperation::LessEqualVec, "setbe [r8]"},
      {TPCode::TOperation::GreaterEqualVec, "setae [r8]"}
  }) {
    pageSize = sysconf(_SC_PAGESIZE);
    currentOffset = pageSize;
    memset (operationTable, 0, sizeof (operationTable));
    memset (conversionTable, 0, sizeof (conversionTable));
}

TVectorJIT::~TVectorJIT () {
    for (void *p: memoryPages)
        free (p);
}

TVectorJIT::TFunctionPtr TVectorJIT::assembleBinary (const std::string &asmName, const std::string &binName) {
    const std::string cmd = "nasm -f bin -o " + binName + " " + asmName;
    system (cmd.c_str ());
    
//    std::cout << "Loading " << fn << std::endl;
    std::ifstream f (binName);
    std::stringstream buf;
    buf << f.rdbuf ();
    const std::string s = buf.str ();
    
    if (currentOffset + s.length () >= pageSize) {
        void *p = valloc (pageSize);
//        std::cout << "Allocating page at: " << p << std::endl;
        mprotect (p, pageSize, PROT_READ | PROT_EXEC | PROT_WRITE);
        memoryPages.push_back (p);
        currentOffset = 0;
    }
    
    void *p = static_cast<char *> (memoryPages.back ()) + currentOffset;
    memcpy (p, s.data (), s.length ());
    currentOffset += s.length ();
    
//    std::cout << "Size is: " << s.length () << std::endl;
//    std::cout << "Func is at: " << p << std::endl;
    
    return reinterpret_cast<TFunctionPtr> (p);
}

void TVectorJIT::createConversionSource (TPCode::TScalarTypeCode srctypeCode, TPCode::TScalarTypeCode dsttypeCode, const std::string &asmName) {
    std::ofstream out (asmName);
    out << "%define srcsize " << TPCode::scalarTypeSizes [static_cast<std::size_t> (srctypeCode)] << std::endl
        << "%define srctype " << TPCode::scalarTypeNames [static_cast<std::size_t> (srctypeCode)] << std::endl
        << "%define dstsize " << TPCode::scalarTypeSizes [static_cast<std::size_t> (dsttypeCode)] << std::endl
        << "%define dsttype " << TPCode::scalarTypeNames [static_cast<std::size_t> (dsttypeCode)] << std::endl
        << asm_conv_src;
//    out.close ();
}

TVectorJIT::TConversionFunctionPtr TVectorJIT::buildConversionFunction (TPCode::TScalarTypeCode srctypeCode, TPCode::TScalarTypeCode dsttypeCode) {
    createConversionSource (srctypeCode, dsttypeCode, asmFile);
    return reinterpret_cast<TConversionFunctionPtr> (assembleBinary (asmFile, binFile));
}

void TVectorJIT::createLoadFirst (std::ofstream &out, TPCode::TScalarTypeCode typeCode1, bool isIntOp, bool isFirstInt) {
    out << "%macro loadfirst 0" << std::endl;
    out << loadFirst.at (typeCode1) << std::endl;
    if (!isIntOp && isFirstInt)
        out << "cvtsi2sd xmm0, rax" << std::endl;
    out << "%endmacro" << std::endl << std::endl;
}

void TVectorJIT::createLoadSecond (std::ofstream &out, TPCode::TScalarTypeCode typeCode2, bool isIntOp, bool isSecondInt) {
    out << "%macro loadsecond 0" << std::endl;
    out << loadSecond.at (typeCode2) << std::endl;
    if (!isIntOp && isSecondInt)
        out << "cvtsi2sd xmm1, rbx" << std::endl;
    out << "%endmacro" << std::endl << std::endl;
}

void TVectorJIT::createOperation (std::ofstream &out, TPCode::TOperation operation, bool isCompare, bool isIntOp) {
    out << "%macro perform_op 0" << std::endl;
    if (isCompare) {
        if (isIntOp) 
            out << "cmp rax, rbx" << std::endl
                << storeCompareIntOp.at (operation) << std::endl;
        else
            out << "comisd xmm0, xmm1" << std::endl
                << storeCompareDoubleOp.at (operation) << std::endl;
        out << "inc r8" << std::endl;
    } else {
        if (isIntOp)
            out << intOp.at (operation) << std::endl
                << "mov [r8], rax" << std::endl;
        else
            out << doubleOp.at (operation) << std::endl
                << "movsd [r8], xmm0" << std::endl;
        out << "add r8, 8" << std::endl;
    }
    out << "%endmacro" << std::endl << std::endl;
}

void TVectorJIT::createOperationSource (TPCode::TOperation operation, TPCode::TScalarTypeCode typeCode1, TPCode::TScalarTypeCode typeCode2, const std::string &asmName) {
    std::ofstream out (asmName);
    
    bool isFirstInt = typeCode1 != TPCode::TScalarTypeCode::real && typeCode1 != TPCode::TScalarTypeCode::single,
         isSecondInt = typeCode2 != TPCode::TScalarTypeCode::real && typeCode2 != TPCode::TScalarTypeCode::single,
         isIntOp = isFirstInt && isSecondInt && operation != TPCode::TOperation::DivVec,
         isCompare = (static_cast<unsigned> (operation) >= static_cast<unsigned> (TPCode::TOperation::EqualVec) &&
                      static_cast<unsigned> (operation) <= static_cast<unsigned> (TPCode::TOperation::GreaterEqualVec));
                 
    out << "bits 64" << std::endl << std::endl
        << "%define src1size " << TPCode::scalarTypeSizes [static_cast<std::size_t> (typeCode1)] << std::endl
        << "%define src2size " << TPCode::scalarTypeSizes [static_cast<std::size_t> (typeCode2)] << std::endl
        << "%define int_op " << (isIntOp ? 1 : 0) << std::endl;

    createLoadFirst (out, typeCode1, isIntOp, isFirstInt);
    createLoadSecond (out, typeCode2, isIntOp, isSecondInt);
    createOperation (out, operation, isCompare, isIntOp);
        
    out << asm_op_src;
}

TVectorJIT::TOperationFunctionPtr TVectorJIT::buildOperationFunction (TPCode::TOperation operation, TPCode::TScalarTypeCode typeCode1, TPCode::TScalarTypeCode typeCode2) {
    createOperationSource (operation, typeCode1, typeCode2, asmFile);
    return reinterpret_cast<TOperationFunctionPtr> (assembleBinary (asmFile, binFile));
}

#endif

}
