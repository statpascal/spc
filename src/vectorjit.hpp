#pragma once

#include <cstdint>
#include <vector>

#include "pcodeops.hpp"

namespace statpascal {

#ifdef USE_VECTOR_JIT

class TVectorJIT final {
public:
    TVectorJIT ();
    ~TVectorJIT ();

    /** returns true in case of overflow */
    typedef bool (*TOperationFunctionPtr) (const void *src1, std::size_t src1len, const void *src2, std::size_t src2len, void *dst, void *dstend);
    typedef void (*TConversionFunctionPtr) (const void *src, void *dst, std::size_t count);
    
    TOperationFunctionPtr getVectorOperation (TPCode::TOperation operation, TPCode::TScalarTypeCode typeCode1, TPCode::TScalarTypeCode typeCode2);
    TConversionFunctionPtr getVectorConversion (TPCode::TScalarTypeCode typecodeSrc, TPCode::TScalarTypeCode typecodeDst);
    
private:
    typedef void (*TFunctionPtr) ();
    TFunctionPtr assembleBinary (const std::string &asmName, const std::string &binName);

    void createLoadFirst (std::ofstream &out, TPCode::TScalarTypeCode typeCode1, bool isIntOp, bool isFirstInt);
    void createLoadSecond (std::ofstream &out, TPCode::TScalarTypeCode typeCode2, bool isIntOp, bool isSecondInt);
    void createOperation (std::ofstream &out, TPCode::TOperation operation, bool isCompare, bool isIntOp);        
    void createOperationSource (TPCode::TOperation operation, TPCode::TScalarTypeCode typeCode1, TPCode::TScalarTypeCode typeCode2, const std::string &asmName);
    TOperationFunctionPtr buildOperationFunction (TPCode::TOperation operation, TPCode::TScalarTypeCode typeCode1, TPCode::TScalarTypeCode typeCode2);
    
    void createConversionSource (TPCode::TScalarTypeCode srctypeCode, TPCode::TScalarTypeCode dsttypeCode, const std::string &asmName);
    TConversionFunctionPtr buildConversionFunction (TPCode::TScalarTypeCode srctypeCode, TPCode::TScalarTypeCode dsttypeCode);
    
    TOperationFunctionPtr operationTable [static_cast<std::size_t> (TPCode::TOperation::GreaterEqualVec) - static_cast<std::size_t> (TPCode::TOperation::AddVec) + 1][static_cast<std::size_t> (TPCode::TScalarTypeCode::count)][static_cast<std::size_t> (TPCode::TScalarTypeCode::count)];
    TConversionFunctionPtr conversionTable [static_cast<std::size_t> (TPCode::TScalarTypeCode::count)][static_cast<std::size_t> (TPCode::TScalarTypeCode::count)];
    
    std::size_t pageSize, currentOffset;
    std::vector<void *> memoryPages;
    
    const std::unordered_map<TPCode::TScalarTypeCode, std::string> loadFirst, loadSecond;
    const std::unordered_map<TPCode::TOperation, std::string> intOp, doubleOp, storeCompareIntOp, storeCompareDoubleOp;
};

inline TVectorJIT::TOperationFunctionPtr TVectorJIT::getVectorOperation (TPCode::TOperation operation, TPCode::TScalarTypeCode typeCode1, TPCode::TScalarTypeCode typeCode2) {
    TOperationFunctionPtr &functionPtr = operationTable [static_cast<std::size_t> (operation) - static_cast<std::size_t> (TPCode::TOperation::AddVec)][static_cast<std::size_t> (typeCode1)][static_cast<std::size_t> (typeCode2)];
    if (!functionPtr) 
        functionPtr = buildOperationFunction (operation, typeCode1, typeCode2);
    return functionPtr;
}

inline TVectorJIT::TConversionFunctionPtr TVectorJIT::getVectorConversion (TPCode::TScalarTypeCode typecodeSrc, TPCode::TScalarTypeCode typecodeDst) {
    TConversionFunctionPtr &functionPtr = conversionTable [static_cast<std::size_t> (typecodeSrc)][static_cast<std::size_t> (typecodeDst)];
    if (!functionPtr)
        functionPtr = buildConversionFunction (typecodeSrc, typecodeDst);
    return functionPtr;
}

#endif

}
