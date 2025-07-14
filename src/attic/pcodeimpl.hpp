/** \file pcodeimpl.hpp
*/

#pragma once

#if 0

#include "pcodeops.hpp"
#include "pcode.hpp"
#include "anymanager.hpp"
#include "vectordata.hpp"
#include "filehandler.hpp"
#include "stacks.hpp"

#include <vector>
#include <deque>
#include <cstring>
#include <unordered_map>
#include <type_traits>
#include <random>
#include <memory>

#include <iostream>
#include <sstream>

#ifdef USE_VECTOR_JIT
  #include "vectorjit.hpp"
#endif

namespace statpascal {

struct TCallbackDescription;
class TPCodeInterpreterImpl;
class TExternalRoutine;
class TCallbackData;

enum TVectorCombineOption { CombineScalar, CombineSingle, CombineString, CombineVector, CombineLValue };

class TPCodeOperation final {
public:
    TPCodeOperation (TPCode::TOperation pcode, std::int64_t para1 = 0, std::int64_t para2 = 0);
    TPCodeOperation (TPCode::TOperation pcode, double para);
    TPCodeOperation (TPCode::TOperation pcode, float paraf);
    
    TPCode::TOperation getPCode () const;
    std::int64_t getPara1 () const;
    std::int64_t getPara2 () const;
    double getPara () const;
    
private:
    TPCode::TOperation pCode;
    union {
        double para;
        float paraf;
        std::int64_t para1;
    };
    std::int64_t para2;
};

// ---

class TPCodeInterpreterImpl final {
public:
    TPCodeInterpreterImpl (std::size_t stacksize);
    ~TPCodeInterpreterImpl ();
    TPCodeInterpreterImpl (const TPCodeInterpreterImpl &) = delete;
    TPCodeInterpreterImpl &operator = (const TPCodeInterpreterImpl &) = delete;
    TPCodeInterpreterImpl (TPCodeInterpreterImpl &&) = default;
    TPCodeInterpreterImpl &operator = (TPCodeInterpreterImpl &&) = default;

    std::size_t outputPCode (const TPCodeOperation &);
    void outputPCode (std::size_t address, const TPCodeOperation &);
    void fixupRoutineAddress (std::size_t address, std::size_t destinationAddress);
    std::size_t getOutputPosition () const;
    
    std::size_t registerStringConstant (const std::string &);
    std::size_t registerBlob (const void *, std::size_t);
    std::size_t registerAnyManager (TAnyManager *);
    std::size_t registerExternalRoutine (TExternalRoutine &&);
    void registerCallback (const std::string &name, std::size_t pcodePosition, const TCallbackDescription &callbackDescription);
    TCallbackData &getCallbackData (std::size_t pcodePosition, const TCallbackDescription &callbackDescription);
    
//    TStack &getStack ();
    void callRoutine (std::size_t pos);
    
    TPCodeInterpreter::TRuntimeResult run (std::size_t startAddress = 0);
    TPCodeInterpreter::TRuntimeResult resume ();
    
    TPCodeInterpreter::TCallback getCallback (const std::string &name);
    
    void appendListing (std::vector<std::string> &output, std::size_t begin, std::size_t end);

    std::size_t getRequiredAlignment () const;

    // TODO: seperate class for file management !!!!
    
    static const std::size_t invalidIndex = std::numeric_limits<std::size_t>::max ();
    TTextFileBaseHandler &getTextFileBaseHandler (std::size_t index);
    TBinaryFileHandler &getBinaryFileHandler (std::size_t index);
    TFileHandler &getFileHandler (std::size_t index);
    std::size_t registerFileHandler (TFileHandler *);
    void closeFileHandler (std::size_t index);

private:
    TStackMemory *lookupDisplay (const TPCodeOperation &);
    
    void performStringPtr ();
    void performStringIndex ();    
    void performAlloc (const TPCodeOperation &op);
    void performFree ();
    
    /** random value in interval [0, 1). */
    double random ();
    
    /** random value betwween zero and max - 1. */
    std::int64_t random (std::int64_t max);
    
    template<typename Cmp> void stringCmp ();
    template<typename T> void incMemOp (const TPCodeOperation &);
    template<typename T> void decMemOp (const TPCodeOperation &);
    template<double (*fn)(double)> void applyFunction ();


#ifdef USE_VECTOR_JIT
    TVectorJIT vectorJIT;
    void applyVectorOperation (TPCode::TOperation operation, TPCode::TScalarTypeCode typecode1, TPCode::TScalarTypeCode typecode2);
#else    
    struct TVectorOpParameter {
        const void *src1Begin, *src1End;
        TPCode::TScalarTypeCode typecode1;
        const void *src2Begin, *src2End;
        TPCode::TScalarTypeCode typecode2;
        void *result;
        std::size_t resultsize;
        TVectorDataPtr resultVector;
    };
    template<class TOp, typename T1, typename T2> void applyVectorOperation (const TVectorOpParameter *parameter);
    template<class TOp, typename T1> void applyVectorOperation (const TVectorOpParameter *parameter);
    template<class TOp> void applyVectorOperation (const TVectorOpParameter *parameter);
    template<class TOp> void applyVectorOperation (TPCode::TScalarTypeCode typecode1, TPCode::TScalarTypeCode typecode2, bool isBooleanOp);
    template<template<typename T> class TOp> void applyVectorOperation (TPCode::TScalarTypeCode typecode1, TPCode::TScalarTypeCode typecode2, bool isBooleanOp);
#endif    
    
    template<typename TSrc, typename TDst> void applyVectorConversion (const void *srcptr, void *dstptr, std::size_t count);
    template<typename TSrc> void applyVectorConversion (const void *srcptr, void *dstptr, TPCode::TScalarTypeCode typecodeDst, std::size_t count);
    void applyVectorConversion (const void *srcptr, TPCode::TScalarTypeCode typecodeSrc, void *dstptr, TPCode::TScalarTypeCode typecodeDst, std::size_t count);
    void applyVectorConversion (TPCode::TScalarTypeCode typecodeSrc, TPCode::TScalarTypeCode typecodeDst);

    template<double (*fn)(double)> void applyFunctionVector ();        
    
    void randomPermVector ();
    
    template<typename T> void applyIndexIntVec ();
    void applyIndexIntVec (const TPCodeOperation &op);
    
    template<typename T> void copyBooleanVec (const bool *index, TVectorDataPtr &result, const TVectorDataPtr &srcData, std::size_t indexCount);
    void applyIndexBoolVec ();
    void applyIndexInt ();
    
    void performCombine (const TPCodeOperation &op);
    void performResize (const TPCodeOperation &op);
    void performVectorRev ();
    void performInvertVec (bool isLogical);
    
    template<typename T> void performVectorRangeCheck (const TVectorDataPtr, std::int64_t minval, std::int64_t maxval);
    void performVectorRangeCheck (TPCode::TScalarTypeCode typecode);
    
    TCalculatorStack calculatorStack;
    TStack stack;
    std::vector<TStackMemory *> displayMem;
    TStackMemory **display;
    
    std::vector<TPCodeOperation> programMemory;
    std::deque<TAnyValue> stringPool;
    std::vector<std::vector<unsigned char>> blobPool;
    std::vector<TAnyManager *> anyManagers;
    std::vector<TExternalRoutine> externalRoutines;
    
    using TCallbackMap = std::unordered_map<std::size_t, TCallbackData *>;
    using TNamedCallbackMap = std::unordered_map<std::string, std::size_t>;
    TCallbackMap callbackMap;
    TNamedCallbackMap namedCallbackMap;
    
    std::vector<TFileHandler *> fileHandler;
    
    std::int64_t acc;
    std::size_t breakProgramCounter;
    TStackMemory *breakBasePtr;
    
    struct TFileData {
        std::int64_t fileIndex;
        TAnyValue fileName;
    };
    
    struct TArrayData {
        std::size_t size, count;
        std::size_t anyManagerIndex;
    };
    typedef std::unordered_map<void *, TArrayData> TDynamicArrayMap;
    TDynamicArrayMap dynamicArrayMap;
    
    std::random_device rd;
    std::mt19937 generator;
    std::uniform_real_distribution<> uniform;
    
    std::stringstream writebuf;
};

}

#endif

