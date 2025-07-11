#pragma once

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <cstring>
#include <unordered_map>
#include <type_traits>
#include <random>

#include <iostream>
#include <sstream>

#include "pcodeops.hpp"
#include "anymanager.hpp"
#include "vectordata.hpp"
#include "filehandler.hpp"
#include "stacks.hpp"
#include "external.hpp"
#include "runtime.hpp"

#ifdef USE_VECTOR_JIT
  #include "vectorjit.hpp"
#endif


namespace statpascal {

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


class TPCodeInterpreter;

class TPCodeMemory {
public:
    TPCodeMemory ();
    ~TPCodeMemory ();
    TPCodeMemory (const TPCodeMemory &) = delete;
    TPCodeMemory &operator = (const TPCodeMemory &) = delete;
    TPCodeMemory (TPCodeMemory &&) = default;
    TPCodeMemory &operator = (TPCodeMemory &&) = default;
    
    // methods for compiler
    std::size_t outputPCode (const TPCodeOperation &);
    void outputPCode (std::size_t address, const TPCodeOperation &);
    void fixupRoutineAddress (std::size_t address, std::size_t destinationAddress);
    std::size_t getOutputPosition () const;
    void appendListing (std::vector<std::string> &output, std::size_t begin, std::size_t end);
    
    std::size_t registerExternalRoutine (TExternalRoutine &&);
    void registerExportedRoutine (const std::string &name, std::size_t pcodePosition, const TCallbackDescription &callbackDescription);

    /** initializes and starts pcode interpreter with  main thread */
    TRuntimeResult run ();
    
    TRuntimeResult resume ();

    /** returns a thread local instance */
    TPCodeInterpreter &getPCodeInterpreter ();    
    
    using TFunctionPtr = void (*) ();
    TFunctionPtr getExportedRoutine (const std::string &name);
    TFunctionPtr getExportedRoutine (std::size_t pcodePosition);
    TCallbackData &getCallbackData (std::size_t pcodePosition, const TCallbackDescription &callbackDescription);
    
    TRuntimeData &getRuntimeData () { return runtimeData; }
    
    void setGlobalData (void *p) { globalData = p; }
    void *getGlobalData () { return globalData; }

private:
    TRuntimeData runtimeData;

    std::vector<TPCodeOperation> programMemory;
    std::vector<TExternalRoutine> externalRoutines;
    TPCodeInterpreter *mainInterpreter;
    
    struct TExportedRoutine {
        std::size_t pcodeAddress;
        std::string name;
        TCallbackDescription parameter;
    };
    std::deque<TExportedRoutine> exportedRoutines;
    
    using TCallbackMap = std::unordered_map<std::size_t, TCallbackData *>;
    TCallbackMap callbackMap;
    
    void *globalData;
    
    friend class TPCodeInterpreter;
};


class TPCodeInterpreter final {
public:
    TPCodeInterpreter (TPCodeMemory &pcodeMemory, std::size_t stacksize);
    ~TPCodeInterpreter ();
    TPCodeInterpreter (const TPCodeInterpreter &) = delete;
    TPCodeInterpreter &operator = (const TPCodeInterpreter &) = delete;
//    TPCodeInterpreter (TPCodeInterpreter &&) = default;
//    TPCodeInterpreter &operator = (TPCodeInterpreter &&) = default;
    
    TRuntimeResult run (std::size_t startAddress = 0);
    TRuntimeResult resume ();
    
    TStack &getStack () { return stack; }
    void callRoutine (std::size_t pos);
    
    TPCodeMemory &getPCodeMemory () { return pcodeMemory; }
    
    void copyThreadGlobals (TPCodeInterpreter &other);
    
    static const std::size_t invalidAddress = std::numeric_limits<std::size_t>::max ();

private:
    TStackMemory *lookupDisplay (const TPCodeOperation &);
    
    void performStringPtr ();
    void performStringIndex ();    
    
//    template<typename Cmp> void stringCmp ();
    template<typename T> void incMemOp (const TPCodeOperation &);
    template<typename T> void decMemOp (const TPCodeOperation &);
    template<double (*fn)(double)> void applyFunction ();


#ifdef USE_VECTOR_JIT
    // !!!! TODO -> TPCodeMemory, compile before exectuion
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

    TPCodeMemory &pcodeMemory;    
    TCalculatorStack calculatorStack;
    TStack stack;
    std::vector<TStackMemory *> display;
    
    std::int64_t acc;
    std::size_t breakProgramCounter;
    TStackMemory *breakBasePtr;
    
};

}
