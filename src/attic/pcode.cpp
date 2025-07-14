#include "pcode.hpp"
#include "anymanager.hpp"
#include "rng.hpp"
#include "external.hpp"
#include "runtimeerr.hpp"

#include <fstream>
#include <iomanip>
#include <cmath>
#include <functional>
#include <map>
#include <numeric>


namespace statpascal {

template<typename T> class bit_shl {
public:
    constexpr T operator () (const T &lhs, const T &rhs) const {
        return lhs << rhs;
    }
};


template<typename T> class bit_shr {
public:
    constexpr T operator () (const T &lhs, const T &rhs) const {
        return lhs >> rhs;
    }
};


TPCodeOperation::TPCodeOperation (TPCode::TOperation pCode, std::int64_t para1, std::int64_t para2):
  pCode (pCode), para1 (para1), para2 (para2) { 
}

TPCodeOperation
::TPCodeOperation (TPCode::TOperation pCode, double para):
  pCode (pCode), para (para), para2 (0) {
}
    
TPCodeOperation::TPCodeOperation (TPCode::TOperation pCode, float paraf):
  pCode (pCode), paraf (paraf), para2 (0) {
}
    
inline TPCode::TOperation TPCodeOperation::getPCode () const {
    return pCode;
}
 
inline std::int64_t TPCodeOperation::getPara1 () const {
    return para1;
}

inline std::int64_t TPCodeOperation::getPara2 () const {
    return para2;
}

inline double TPCodeOperation::getPara () const {
    return para;
}

// 

using TPCodeInterpreterMap = std::map<TPCodeMemory *, std::unique_ptr<TPCodeInterpreter>>;
thread_local TPCodeInterpreterMap interpreterMap;

TPCodeMemory::TPCodeMemory () {
    runtimeData.setPCodeMemory (this);
}

TPCodeMemory::~TPCodeMemory () {
    for (TCallbackMap::value_type &it: callbackMap)
        delete it.second;
}

std::size_t TPCodeMemory::outputPCode (const TPCodeOperation &code) {
    programMemory.push_back (code);
    return getOutputPosition () - 1;
}

void TPCodeMemory::outputPCode (std::size_t address, const TPCodeOperation &code) {
    programMemory [address] = code;
}

void TPCodeMemory::fixupRoutineAddress (std::size_t address, std::size_t destinationAddress) {
    TPCodeOperation &op = programMemory [address];
    op = TPCodeOperation (op.getPCode (), destinationAddress, op.getPara2 ());
}

std::size_t TPCodeMemory::getOutputPosition () const {
    return programMemory.size ();
}

std::size_t TPCodeMemory::registerExternalRoutine (TExternalRoutine &&r) {
    externalRoutines.push_back (std::move (r));
    return externalRoutines.size () - 1;
}

void TPCodeMemory::registerExportedRoutine (const std::string &name, std::size_t pcodePosition, const TCallbackDescription &callbackDescription) {
    exportedRoutines.push_back ({pcodePosition, name, callbackDescription});
}

TRuntimeResult TPCodeMemory::run () {
    interpreterMap [this] = std::make_unique<TPCodeInterpreter> (*this, 25 * 1000 * 1000);
    mainInterpreter = interpreterMap [this].get ();
    return mainInterpreter->run ();
}

TRuntimeResult TPCodeMemory::resume () {
    return interpreterMap [this]->resume ();
}

TPCodeInterpreter &TPCodeMemory::getPCodeInterpreter () {
    TPCodeInterpreterMap::iterator it = interpreterMap.find (this);
    if (it == interpreterMap.end ()) {
        it = interpreterMap.insert (std::make_pair (this, std::make_unique<TPCodeInterpreter> (*this, 1000 * 1000))).first;
        it->second->copyThreadGlobals (*mainInterpreter);
    }
    return *(it->second.get ());
}

TCallbackData &TPCodeMemory::getCallbackData (std::size_t pcodePosition, const TCallbackDescription &callbackDescription) {
    // TODO: stack in key aufnehmen !!!!
    TCallbackMap::iterator it = callbackMap.find (pcodePosition);
    if (it == callbackMap.end ()) 
        it = callbackMap.insert (std::make_pair (pcodePosition, new TCallbackData (callbackDescription, *this, pcodePosition))).first;
    return *it->second;
}

TPCodeMemory::TFunctionPtr TPCodeMemory::getExportedRoutine (const std::string &name) {
    for (const TExportedRoutine &exportedRoutine: exportedRoutines)
        if (exportedRoutine.name == name)
            return getCallbackData (exportedRoutine.pcodeAddress, exportedRoutine.parameter).getFunctionPtr ();
    return nullptr;
}

TPCodeMemory::TFunctionPtr TPCodeMemory::getExportedRoutine (std::size_t pcodePosition) {
    for (const TExportedRoutine &exportedRoutine: exportedRoutines)
        if (exportedRoutine.pcodeAddress == pcodePosition)
            return getCallbackData (exportedRoutine.pcodeAddress, exportedRoutine.parameter).getFunctionPtr ();
    return nullptr;
}

void TPCodeMemory::appendListing (std::vector<std::string> &output, std::size_t begin, std::size_t end) {
    enum TParameterType {None, Int1, Int2, Double};
    static const std::pair<std::string, TParameterType> pcode [] = {
        {"LoadInt64Const", Int1}, {"LoadDoubleConst", Double}, {"LoadExportAddr", None}, 
        {"LoadAddress", Int2}, {"CalcIndex", Int2}, {"StrIndex", None}, {"StrPtr", None},
        {"LoadAcc", None}, {"StoreAcc", None}, {"LoadRuntimePtr", None},
        
        {"LoadInt8", None}, {"LoadInt16", None}, {"LoadInt32", None}, {"LoadInt64", None}, {"LoadUint8", None}, {"LoadUint16", None}, {"LoadUint32", None}, {"LoadDouble", None}, {"LoadSingle", None}, {"LoadVec", None}, {"LoadPtr", None},
        {"Store8", None}, {"Store16", None}, {"Store32", None}, {"Store4", None}, {"StoreDouble", None}, {"StoreSingle", None}, {"StoreVec", None}, {"StorePtr", None}, 
        {"Push8", None}, {"Push16", None}, {"Push32", None}, {"Push64", None}, {"PushDouble", None}, {"PushSingle", None}, {"PushVec", None}, {"PushPtr", None},
        
        {"CopyMem", Int1}, {"CopyMemAny", Int2}, {"CopyMemVec", Int2}, {"PushMem", Int1}, {"PushMemAny", Int2}, {"PushMemVec", Int2}, {"AlignStack", Int1}, {"Destructors", Int1}, {"ReleaseStack", Int1},
        {"Alloc", Int2}, {"Free", None},
        
        {"CallStack", None}, {"Call", Int1}, {"CallFFI", Int1}, {"CallExtern", Int2}, {"Enter", Int2}, {"Leave", Int2}, {"Return", Int1}, {"jump", Int1}, {"JumpZero", Int1}, {"JumpNotZero", Int1},
        {"JmpAccEqual", Int2}, {"CmpAccRange", Int2},
        
        {"AddInt64Const", Int1}, {"Dup", None},
        
        {"AddInt", None}, {"SubInt", None}, {"OrInt", None}, {"XorInt", None}, {"MulInt", None}, {"DivInt", None}, {"ModInt", None}, {"AndInt", None}, {"ShlInt", None}, {"ShrInt", None},
        {"EqualInt", None}, {"NotEqualInt", None}, {"LessInt", None}, {"GreaterInt", None}, {"LessEqualInt", None}, {"GreaterEqualInt", None},
        
        {"NotInt", None}, {"NegInt", None}, {"NegDouble", None}, {"NotBool", None},
        
        {"AddDouble", None}, {"SubDouble", None}, {"MulDouble", None}, {"DivDouble", None},
        {"EqualDouble", None}, {"NotEqualDouble", None}, {"LessDouble", None}, {"GreaterDouble", None}, {"LessEqualDouble", None}, {"GreaterEqualDouble", None},
        
        {"ConvertIntDouble", None},
        
        {"MakeVec", Int2}, {"MakeVecMem", Int2}, {"MakeVecZero", Int2}, {"ConvertVec", Int2}, {"CombineVec", Int2}, {"ResizeVec", Int2}, 
        {"VecIndexIntVec", Int1}, {"VecIndexBoolVec", None}, {"VecIndexInt", None},
        
        {"NotIntVec", None}, {"NotBoolVec", None}, 
        {"AddVec", Int2}, {"SubVec", Int2}, {"OrVec", Int2}, {"XorVec", Int2}, {"MulVec", Int2}, {"DivVec", Int2}, {"DivIntVec", Int2}, {"ModVec", Int2}, {"AndVec", Int2},
        {"EqualVec", Int2}, {"NotEqualVec", Int2}, {"LessVec", Int2}, {"GreaterVec", Int2}, {"LessEqualVec", Int2}, {"GreaterEqualVec", Int2},
        
        {"VecLength", None}, {"VecRev", None}, {"RangeCheckVec", Int1},
        
        {"RangeCheck", Int2}, {"NoOperation", None}, {"Stop", None}, {"Break", None}, {"Halt", None}, {"IllegalOperation", None}
    };

    std::stringstream ss;    
    for (std::size_t count = begin; count < end; ++count) {
        ss.str (std::string ());
        TPCodeOperation &op = programMemory [count];
        ss  << std::setw (8) << count << "   " << std::setw (20) << std::left << pcode [static_cast<std::size_t> (op.getPCode ())].first << std::right;
        switch (pcode [static_cast<std::size_t> (op.getPCode ())].second) {
            case None:
                break;
            case Int1:
                ss << ' ' << op.getPara1 ();
                break;
            case Int2:
                ss << ' ' << op.getPara1 () << ", " << op.getPara2 ();
                break;
            case Double:
                ss << ' ' << op.getPara ();
                break;
        }
        output.push_back (ss.str ());
    }
}

// ---

TPCodeInterpreter::TPCodeInterpreter (TPCodeMemory &pcodeMemory, const std::size_t stacksize):
  pcodeMemory (pcodeMemory),
  stack (stacksize),
  display (16, 0),
  breakProgramCounter (0),
  breakBasePtr (nullptr) {
//  uniform (0.0, 1.0) {
}

TPCodeInterpreter::~TPCodeInterpreter () {
}

void TPCodeInterpreter::copyThreadGlobals (TPCodeInterpreter &other) {
    display = other.display;
}

#ifndef USE_VECTOR_JIT

#ifdef USE_VECTOR_TEMPLATES

template<typename TSrc, typename TDst> void TPCodeInterpreter::applyVe ctorConversion (const void *srcptr, void *dstptr, std::size_t count) {
    const TSrc *src = static_cast<const TSrc *> (srcptr);
    TDst *dst = static_cast<TDst *> (dstptr);
    while (count--)
        *dst++ = *src++;
}

template<typename TSrc> void TPCodeInterpreter::applyVectorConversion (const void *srcptr, void *dstptr, TPCode::TScalarTypeCode typecodeDst, std::size_t count) {
    switch (typecodeDst) {
        case TPCode::TScalarTypeCode::s64:
            applyVectorConversion<TSrc, std::int64_t> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::s32:
            applyVectorConversion<TSrc, std::int32_t> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::s16:
            applyVectorConversion<TSrc, std::int16_t> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::s8:
            applyVectorConversion<TSrc, std::int8_t> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::u8:
            applyVectorConversion<TSrc, std::uint8_t> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::u16:
            applyVectorConversion<TSrc, std::uint16_t> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::u32:
            applyVectorConversion<TSrc, std::uint32_t> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::real:
            applyVectorConversion<TSrc, double> (srcptr, dstptr, count);
            break;
        case TPCode::TScalarTypeCode::single:
            applyVectorConversion<TSrc, float> (srcptr, dstptr, count);
            break;
        default:
            break;
    }
}

void TPCodeInterpreter::applyVectorConversion (const void *srcptr, TPCode::TScalarTypeCode typecodeSrc, void *dstptr, TPCode::TScalarTypeCode typecodeDst, std::size_t count) {
    switch (typecodeSrc) {
        case TPCode::TScalarTypeCode::s64:
            applyVectorConversion<std::int64_t> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::s32:
            applyVectorConversion<std::int32_t> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::s16:
            applyVectorConversion<std::int16_t> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::s8:
            applyVectorConversion<std::int8_t> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::u8:
            applyVectorConversion<std::uint8_t> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::u16:
            applyVectorConversion<std::uint16_t> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::u32:
            applyVectorConversion<std::uint32_t> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::real:
            applyVectorConversion<double> (srcptr, dstptr, typecodeDst, count);
            break;
        case TPCode::TScalarTypeCode::single:
            applyVectorConversion<float> (srcptr, dstptr, typecodeDst, count);
            break;
        default:
            break;
    }
}

#else	// USE_VECTOR_TEMPLATES

void TPCodeInterpreter::applyVectorConversion (const void *srcptr, TPCode::TScalarTypeCode typecodeSrc, void *dstptr, TPCode::TScalarTypeCode typecodeDst, std::size_t count) {
    const std::size_t srcsize = TPCode::scalarTypeSizes [static_cast<std::size_t> (typecodeSrc)],
                      dstsize = TPCode::scalarTypeSizes [static_cast<std::size_t> (typecodeDst)];
    bool srcint = typecodeSrc != TPCode::TScalarTypeCode::single && typecodeSrc != TPCode::TScalarTypeCode::real;
    const unsigned char *srcit = reinterpret_cast<const unsigned char *> (srcptr);
    unsigned char *dstit = reinterpret_cast<unsigned char *> (dstptr);
    double fVal = 0.0;
    std::int64_t iVal = 0;
    for (std::size_t i = 0; i < count; ++i) {
        switch (typecodeSrc) {
            case TPCode::TScalarTypeCode::s64:
                iVal = *reinterpret_cast<const std::int64_t *> (srcit);
                break;
            case TPCode::TScalarTypeCode::s32:
                iVal = *reinterpret_cast<const std::int32_t *> (srcit);
                break;
            case TPCode::TScalarTypeCode::s16:
                iVal = *reinterpret_cast<const std::int16_t *> (srcit);
                break;
            case TPCode::TScalarTypeCode::s8:
                iVal = *reinterpret_cast<const std::int8_t *> (srcit);
                break;
            case TPCode::TScalarTypeCode::u8:
                iVal = *reinterpret_cast<const std::uint8_t *> (srcit);
                break;
            case TPCode::TScalarTypeCode::u16:
                iVal = *reinterpret_cast<const std::uint16_t *> (srcit);
                break;
            case TPCode::TScalarTypeCode::u32:
                iVal = *reinterpret_cast<const std::uint32_t *> (srcit);
                break;
            case TPCode::TScalarTypeCode::single:
                fVal = *reinterpret_cast<const float *> (srcit);
                break;
            case TPCode::TScalarTypeCode::real:
                fVal = *reinterpret_cast<const double *> (srcit);
                break;
            default:
                break;
        }
        srcit += srcsize;
        switch (typecodeDst) {
            case TPCode::TScalarTypeCode::s64:
                *reinterpret_cast<std::int64_t *> (dstit) = iVal;
                break;
            case TPCode::TScalarTypeCode::s32:
                *reinterpret_cast<std::int32_t *> (dstit) = iVal;
                break;
            case TPCode::TScalarTypeCode::s16:
                *reinterpret_cast<std::int16_t *> (dstit) = iVal;
                break;
            case TPCode::TScalarTypeCode::s8:
                *reinterpret_cast<std::int8_t *> (dstit) = iVal;
                break;
            case TPCode::TScalarTypeCode::u8:
                *reinterpret_cast<std::uint8_t *> (dstit) = iVal;
                break;
            case TPCode::TScalarTypeCode::u16:
                *reinterpret_cast<std::uint16_t *> (dstit) = iVal;
                break;
            case TPCode::TScalarTypeCode::u32:
                *reinterpret_cast<std::uint32_t *> (dstit) = iVal;
                break;
            case TPCode::TScalarTypeCode::single:
                if (srcint)
                    *reinterpret_cast<float *> (dstit) = iVal;
                else
                    *reinterpret_cast<float *> (dstit) = fVal;
                break;
            case TPCode::TScalarTypeCode::real:
                if (srcint)
                    *reinterpret_cast<double *> (dstit) = iVal;
                else
                    *reinterpret_cast<double *> (dstit) = fVal;
                break;
            default:
                break;
        }
        dstit += dstsize;        
    }    
}

#endif // USE_VECTOR_TEMPLATES

#endif  // !USE_VECTOR_JIT

void TPCodeInterpreter::applyVectorConversion (TPCode::TScalarTypeCode typecodeSrc, TPCode::TScalarTypeCode typecodeDst) {
    TVectorDataPtr src (calculatorStack.fetchPtr (), false);
    const void *srcptr = &src->get<std::uint8_t> (0);
    
    TVectorDataPtr result (TPCode::scalarTypeSizes [static_cast<std::size_t> (typecodeDst)], src->getElementCount (), nullptr, false);
    void *dstptr = &result->get<std::uint8_t> (0);

#ifdef USE_VECTOR_JIT
    vectorJIT.getVectorConversion (typecodeSrc, typecodeDst) (srcptr, dstptr, src->getElementCount ());
#else    
    applyVectorConversion (srcptr, typecodeSrc, dstptr, typecodeDst, src->getElementCount ());
#endif
    
    calculatorStack.pushPtr (result.makeRef ());
}

#ifdef USE_VECTOR_JIT

void TPCodeInterpreter::applyVectorOperation (TPCode::TOperation operation, TPCode::TScalarTypeCode typecode1, TPCode::TScalarTypeCode typecode2) {
    const TVectorDataPtr src2 (calculatorStack.fetchPtr (), false),
                         src1 (calculatorStack.fetchPtr (), false);
    TVectorDataPtr resultVector (operation >= TPCode::TOperation::EqualVec && operation <= TPCode::TOperation::GreaterEqualVec ? sizeof (bool) : sizeof (std::int64_t), std::max (src1->getElementCount (), src2->getElementCount ()), nullptr, false);
    std::uint8_t *dstptr = &resultVector->get<std::uint8_t> (0);
    
    if (vectorJIT.getVectorOperation (operation, typecode1, typecode2) (&src1->get<std::uint8_t> (0), src1->getElementCount (), &src2->get<std::uint8_t> (0), src2->getElementCount (), dstptr, dstptr + resultVector->getElementSize () * resultVector->getElementCount ()))
        throw TRuntimeError (TRuntimeResult::TCode::Overflow);

    calculatorStack.pushPtr (resultVector.makeRef ());
}

#else

#ifdef USE_VECTOR_TEMPLATES

template<class TOp, typename T1, typename T2> void TPCodeInterpreter::applyVectorOperation (const TVectorOpParameter *parameter) {
    TOp op;
    typedef decltype (op (T1 (), T2 ())) T;
    
    T *result = static_cast<T *> (parameter->result);
    const T1 *const src1Begin = static_cast<const T1 *> (parameter->src1Begin), 
             *const src1End = static_cast<const T1 *> (parameter->src1End),
             *srcit1 = src1Begin;
    const T2 *const src2Begin = static_cast<const T2 *> (parameter->src2Begin), 
             *const src2End = static_cast<const T2 *> (parameter->src2End),
             *srcit2 = src2Begin;
    const bool inc1 = src1End > src1Begin + 1,
               inc2 = src2End > src2Begin + 1;

    for (std::size_t i = 0; i < parameter->resultsize; ++i) {
        *result = op (*srcit1, *srcit2);
        ++result;
        if (inc1 && ++srcit1 == src1End)
            srcit1 = src1Begin;
        if (inc2 && ++srcit2 == src2End)
            srcit2 = src2Begin;
    }
        
}

template<class TOp, typename T1> void TPCodeInterpreter::applyVectorOperation (const TVectorOpParameter *parameter) {
    switch (parameter->typecode2) {
        case TPCode::TScalarTypeCode::s64:
            applyVectorOperation<TOp, T1, std::int64_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::s32:
            applyVectorOperation<TOp, T1, std::int32_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::s16:
            applyVectorOperation<TOp, T1, std::int16_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::s8:
            applyVectorOperation<TOp, T1, std::int8_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::u8:
            applyVectorOperation<TOp, T1, std::uint8_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::u16:
            applyVectorOperation<TOp, T1, std::uint16_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::u32:
            applyVectorOperation<TOp, T1, std::uint32_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::single:
            applyVectorOperation<TOp, T1, float> (parameter);
            break;
        case TPCode::TScalarTypeCode::real:
            applyVectorOperation<TOp, T1, double> (parameter);
            break;
        default:
            break;
    }
}

template<class TOp> void TPCodeInterpreter::applyVectorOperation (const TVectorOpParameter *parameter) {
    switch (parameter->typecode1) {
        case TPCode::TScalarTypeCode::s64:
            applyVectorOperation<TOp, std::int64_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::s32:
            applyVectorOperation<TOp, std::int32_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::s16:
            applyVectorOperation<TOp, std::int16_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::s8:
            applyVectorOperation<TOp, std::int8_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::u8:
            applyVectorOperation<TOp, std::uint8_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::u16:
            applyVectorOperation<TOp, std::uint16_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::u32:
            applyVectorOperation<TOp, std::uint32_t> (parameter);
            break;
        case TPCode::TScalarTypeCode::single:
            applyVectorOperation<TOp, float> (parameter);
            break;
        case TPCode::TScalarTypeCode::real:
            applyVectorOperation<TOp, double> (parameter);
            break;
        default:
            break;
    }
}

#else	// USE_VECTOR_TEMPLATES

template<class TOp> void TPCodeInterpreter::applyVectorOperation (const TVectorOpParameter *parameter) {
    TOp op;
    using T = typename TOp::result_type;
    
    T *result = static_cast<T *> (parameter->result);
    const char *srcit1 = reinterpret_cast<const char *> (parameter->src1Begin),
               *srcit2 = reinterpret_cast<const char *> (parameter->src2Begin);
    const std::size_t src1size = TPCode::scalarTypeSizes [static_cast<std::size_t> (parameter->typecode1)],
                      src2size = TPCode::scalarTypeSizes [static_cast<std::size_t> (parameter->typecode2)];
    const bool inc1 = (srcit1 + src1size != parameter->src1End),
               inc2 = (srcit2 + src2size != parameter->src2End);
    const bool intOp1 = parameter->typecode1 != TPCode::TScalarTypeCode::single && parameter->typecode1 != TPCode::TScalarTypeCode::real,
               intOp2 = parameter->typecode2 != TPCode::TScalarTypeCode::single && parameter->typecode2 != TPCode::TScalarTypeCode::real;
         
    double realop1 = 0.0, realop2 = 0.0;
    std::int64_t intop1 = 0, intop2 = 0;

    for (std::size_t i = 0; i < parameter->resultsize; ++i) {
        switch (parameter->typecode1) {
            case TPCode::TScalarTypeCode::s64:
                intop1 = *reinterpret_cast<const std::int64_t *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::s32:
                intop1 = *reinterpret_cast<const std::int32_t *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::s16:
                intop1 = *reinterpret_cast<const std::int16_t *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::s8:
                intop1 = *reinterpret_cast<const std::int8_t *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::u8:
                intop1 = *reinterpret_cast<const std::uint8_t *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::u16:
                intop1 = *reinterpret_cast<const std::uint16_t *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::u32:
                intop1 = *reinterpret_cast<const std::uint32_t *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::single:
                realop1 = *reinterpret_cast<const float *> (srcit1);
                break;
            case TPCode::TScalarTypeCode::real:
                realop1 = *reinterpret_cast<const double *> (srcit1);
                break;
            default:
                break;
        }
        switch (parameter->typecode2) {
            case TPCode::TScalarTypeCode::s64:
                intop2 = *reinterpret_cast<const std::int64_t *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::s32:
                intop2 = *reinterpret_cast<const std::int32_t *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::s16:
                intop2 = *reinterpret_cast<const std::int16_t *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::s8:
                intop2 = *reinterpret_cast<const std::int8_t *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::u8:
                intop2 = *reinterpret_cast<const std::uint8_t *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::u16:
                intop2 = *reinterpret_cast<const std::uint16_t *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::u32:
                intop2 = *reinterpret_cast<const std::uint32_t *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::single:
                realop2 = *reinterpret_cast<const float *> (srcit2);
                break;
            case TPCode::TScalarTypeCode::real:
                realop2 = *reinterpret_cast<const double *> (srcit2);
                break;
            default:
                break;
        }
        
        if (intOp1)
            if (intOp2)
                *result = op (intop1, intop2);
            else
                *result = op (intop1, realop2);
        else
            if (intOp2)
                *result = op (realop1, intop2);
            else
                *result = op (realop1, realop2);
            
        ++result;
        if (inc1) {
            srcit1 += src1size;
            if (srcit1 == parameter->src1End)
                srcit1 = reinterpret_cast<const char *> (parameter->src1Begin);
        }
        if (inc2) {
            srcit2 += src2size;
            if (srcit2 == parameter->src2End)
                srcit2 = reinterpret_cast<const char *> (parameter->src2Begin);
        }
    }
}

#endif	// USE_VECTOR_TEMPLATES

template<class TOp>  void TPCodeInterpreter::applyVectorOperation (TPCode::TScalarTypeCode typecode1, TPCode::TScalarTypeCode typecode2, bool isBooleanOp) {
    // !!!! Error falls einer der beiden leer ist: count pruefern !!!!
    const TVectorDataPtr src2 (calculatorStack.fetchPtr (), false),
                         src1 (calculatorStack.fetchPtr (), false);
    
    TVectorOpParameter parameter;
    parameter.src1Begin = &src1->get<std::uint8_t> (0);
    parameter.src1End = static_cast<const std::uint8_t *> (parameter.src1Begin) + src1->getElementCount () * src1->getElementSize ();
    parameter.typecode1 = typecode1;
    parameter.src2Begin = &src2->get<std::uint8_t> (0);
    parameter.src2End = static_cast<const std::uint8_t *> (parameter.src2Begin) + src2->getElementCount () * src2->getElementSize ();
    parameter.typecode2 = typecode2;
    parameter.resultVector = TVectorDataPtr (isBooleanOp ? sizeof (bool) : sizeof (std::int64_t), std::max (src1->getElementCount (), src2->getElementCount ()), nullptr, false);
    parameter.result = &parameter.resultVector->get<std::uint8_t> (0);
    parameter.resultsize = parameter.resultVector->getElementCount ();
    
    applyVectorOperation<TOp> (&parameter);
    calculatorStack.pushPtr (parameter.resultVector.makeRef ());
}

template<template<typename T> class TOp> void TPCodeInterpreter::applyVectorOperation (TPCode::TScalarTypeCode typecode1, TPCode::TScalarTypeCode typecode2, bool isBooleanOp) {
    if (typecode1 != TPCode::TScalarTypeCode::real && typecode2 != TPCode::TScalarTypeCode::real &&
        typecode1 != TPCode::TScalarTypeCode::single && typecode2 != TPCode::TScalarTypeCode::single)
        applyVectorOperation<TOp<std::int64_t>> (typecode1, typecode2, isBooleanOp);
    else
        applyVectorOperation<TOp<double>> (typecode1, typecode2, isBooleanOp);
}

#endif	// USE_VECTOR_JIT

template<typename T> void TPCodeInterpreter::applyIndexIntVec () {
    const TVectorDataPtr indexData (calculatorStack.fetchPtr (), false),
                         srcData (calculatorStack.fetchPtr (), false);
                      
    TVectorDataPtr result (srcData->getElementSize (), indexData->getElementCount (), srcData->getElementAnyManager (), false);
    for (std::size_t i = 0; i < indexData->getElementCount (); ++i)
        result->setElement (i, &srcData->get<unsigned char> (indexData->get<T> (i) - 1));
        
    calculatorStack.pushPtr (result.makeRef ());
}

void TPCodeInterpreter::applyIndexIntVec (const TPCodeOperation &op) {
    static std::map<TPCode::TScalarTypeCode, void (TPCodeInterpreter::*) ()> vectorIndexMap = {
        {TPCode::TScalarTypeCode::s8, &TPCodeInterpreter::applyIndexIntVec<std::uint8_t>},
        {TPCode::TScalarTypeCode::u8, &TPCodeInterpreter::applyIndexIntVec<std::uint8_t>},
        {TPCode::TScalarTypeCode::s16, &TPCodeInterpreter::applyIndexIntVec<std::uint16_t>},
        {TPCode::TScalarTypeCode::u16, &TPCodeInterpreter::applyIndexIntVec<std::uint16_t>},
        {TPCode::TScalarTypeCode::s32, &TPCodeInterpreter::applyIndexIntVec<std::uint32_t>},
        {TPCode::TScalarTypeCode::u32, &TPCodeInterpreter::applyIndexIntVec<std::uint32_t>},
        {TPCode::TScalarTypeCode::s64, &TPCodeInterpreter::applyIndexIntVec<std::uint64_t>}
    };
    (this->*vectorIndexMap [static_cast<TPCode::TScalarTypeCode> (op.getPara1 ())]) (); 
}

template<typename T> void /* __attribute__ ((noinline)) */ inline TPCodeInterpreter::copyBooleanVec (const bool *index, TVectorDataPtr &result, const TVectorDataPtr &srcData, const std::size_t indexCount) {
    T *dst = &result->get<T> (0);
    const T* src = &srcData->get<T> (0);
    const T *end = dst + result->getElementCount ();
    const std::uint64_t *index64 = reinterpret_cast<const std::uint64_t *> (index);

    std::size_t i = 0;

    if (indexCount >= sizeof (std::uint64_t))
        while (i <= indexCount - sizeof (std::uint64_t)) {
            std::uint64_t ind = *index64++;
            std::size_t ei = i;
            while (ind) {
                if (ind & 0xff)
                    *dst++ = src [ei];
                ++ei;
                ind >>= 8;
            }
            if (dst == end) 
                return;
            i += sizeof (std::uint64_t);
        }
    while (i < indexCount) {
        if (index [i])
            *dst++ = src [i];
         ++i;
    }
}

// Stack Overflow 18473134

/*
inline std::uint64_t bytesum1 (const std::uint64_t n) {
    const std::uint64_t pair_bits = 0x0001000100010001LLU,
                        mask = pair_bits * 0xff,
                        pair_sum = (n & mask) + ((n >> 8) & mask);
    return (pair_sum * pair_bits) >> (64 - 16);
}

inline std::uint64_t bytesum (const std::uint64_t n) {
    const std::uint64_t mask1 = 0x00ff00ff00ff00ffLLU,
                        mask2 = 0x0000ffff0000ffffLLU,
                        mask3 = 0x00000000ffffffffLLU;
    const std::uint64_t
        n1 = (n & mask1) + ((n >> 8) & mask1),
        n2 = (n1 & mask2) + ((n1 >> 16) & mask2);
    return (n2 & mask3) + (n2 >> 32);
}
*/

void TPCodeInterpreter::applyIndexBoolVec () {
    const TVectorDataPtr indexData (calculatorStack.fetchPtr (), false),
                         srcData (calculatorStack.fetchPtr (), false);
    const std::size_t indexCount = indexData->getElementCount ();
    
    if (indexCount > srcData->getElementCount ()) 
        throw TRuntimeError (TRuntimeResult::TCode::RangeCheck, "Boolean index vector is too long");
    
    const bool *const index = &indexData->get<bool> (0);
    const std::uint64_t *const indexint = &indexData->get<std::uint64_t> (0);

    std::uint64_t count = 0;
    std::size_t i, j = 0;

//    const std::uint64_t pair_bits = 0x0001000100010001LLU,
//                        mask = pair_bits * 0xff;
                        
    if (indexCount >= 16) {
        const std::size_t ei = indexCount / 8;
        for (i = 0; i < ei; i += 255) {
            const std::size_t ej = std::min (i + 255, ei);
            std::uint64_t counter = 0;
            for (j = i; j < ej; ++j)
                counter += indexint [j];
                
//            count += (((counter & mask) + ((counter >> 8) & mask)) * pair_bits) >> 48;
//            const unsigned char *tmp = reinterpret_cast<const unsigned char *> (&counter);
            for (int k = 0; k < 8; ++k) {
                count += counter & 0xff;
                counter >>= 8;
            }
//            count += bytesum (counter);
        }
    }
    for (i = j * 8; i < indexCount; ++i)
        count += index [i];
        

//    std::uint64_t count = std::accumulate (index, index + indexCount, 0);        

    TVectorDataPtr result (srcData->getElementSize (), count, srcData->getElementAnyManager (), false);
    if (count) {
        if (srcData->getElementAnyManager ()) {
            for (std::size_t i = 0, dstIndex = 0; i < indexCount; ++i)
                if (index [i])
                    result->setElement (dstIndex++, &srcData->get<unsigned char> (i));
        } else {
            const std::size_t size = srcData->getElementSize ();
            // avoid memcpy for small values
            switch (size) {
                case 1: 
                    copyBooleanVec<std::uint8_t> (index, result, srcData, indexCount);
                    break; 
                case 2: 
                    copyBooleanVec<std::uint16_t> (index, result, srcData, indexCount);
                    break;
                case 4: 
                    copyBooleanVec<std::uint32_t> (index, result, srcData, indexCount);
                    break;
                case 8: 
                    copyBooleanVec<std::uint64_t> (index, result, srcData, indexCount);
                    break;
                default: {
                    unsigned char *dst = &result->get<unsigned char> (0);
                    const unsigned char *src = &srcData->get<unsigned char> (0);
                    std::size_t counter = count;
                    for (std::size_t i = 0; i < indexCount; ++i) {
                        if (index [i]) {
                            memcpy (dst, src, size);
                            dst += size;
                            if (!--counter) break;
                        }
                        src += size;
                    }
                    break; }
            }
        }
    }            
    calculatorStack.pushPtr (result.makeRef ());
}

void TPCodeInterpreter::applyIndexInt () {
    std::int64_t index = calculatorStack.fetchInt64 ();
    TVectorDataPtr *p = static_cast<TVectorDataPtr *> (calculatorStack.pVal<0> ());
    if (!p)
        throw TRuntimeError (TRuntimeResult::TCode::InvalidArgument, "Accessing uninitialized dynamic data");
    p->copyOnWrite ();
    calculatorStack.pVal<0> () = &((*p)->get<unsigned char> (index - 1));
}

void TPCodeInterpreter::performCombine (const TPCodeOperation &op) {
    std::size_t argCount = calculatorStack.fetchInt64 (),
                count = 0;
                
    for (std::size_t i = 0; i < argCount; ++i) {
        if (calculatorStack.iVal (2 * i) == CombineVector) {
            if (calculatorStack.pVal (2 * i + 1)) 
                count += (*reinterpret_cast<TVectorDataPtr *> (&calculatorStack.pVal (2 * i + 1)))->getElementCount ();
        } else
            ++count;
    }
    TVectorDataPtr result (op.getPara1 (), count, pcodeMemory.getRuntimeData ().getAnyManager (op.getPara2 ()), false);

    std::size_t dstIndex = 0;
    for (std::size_t i = 0; i < argCount; ++i) {
        switch (calculatorStack.fetchInt64 ()) {
            case CombineString:
            case CombineScalar:
                std::memcpy (&result->get<unsigned char> (dstIndex++), &calculatorStack.iVal<0> (), result->getElementSize ());
                calculatorStack.pop ();
                break;
            case CombineSingle:
                result->get<float> (dstIndex++) = calculatorStack.fetchDouble ();
                break;
            case CombineVector: {
                const TVectorDataPtr srcVector (calculatorStack.fetchPtr (), false);
                if (srcVector.hasValue ()) {
                    if (result->getElementAnyManager ())
                        for (std::size_t j = 0; j < srcVector->getElementCount (); ++j) 
                            result->setElement (dstIndex++, &srcVector->get<unsigned char> (j));
                    else {
                        std::memcpy (&result->get<unsigned char> (dstIndex), &srcVector->get<unsigned char> (0), srcVector->getElementCount () * srcVector->getElementSize ());
                        dstIndex += srcVector->getElementCount ();
                    }
                }
                break; }
            case CombineLValue:
                result->setElement (dstIndex++, calculatorStack.fetchPtr ());
                break;
        }
    }
    calculatorStack.pushPtr (result.makeRef ());
}

void TPCodeInterpreter::performResize (const TPCodeOperation &op) {
    TVectorDataPtr resizedData (op.getPara1 (), calculatorStack.fetchInt64 (), pcodeMemory.getRuntimeData ().getAnyManager (op.getPara2 ())),
                   &oldData = *static_cast<TVectorDataPtr *> (calculatorStack.fetchPtr ());
    if (oldData.hasValue ()) {
        const std::size_t copyCount = std::min (oldData->getElementCount (), resizedData->getElementCount ());
        if (resizedData->getElementAnyManager ())
            for (std::size_t j = 0; j < copyCount; ++j)
                resizedData->setElement (j, &(oldData->get<unsigned char> (j)));
        else
            std::memcpy (&resizedData->get<unsigned char> (0), &(oldData->get<unsigned char> (0)), copyCount * resizedData->getElementSize ());
    }
    oldData = resizedData;
}

template<typename T> void TPCodeInterpreter::performVectorRangeCheck (const TVectorDataPtr vectorDataPtr, const std::int64_t minval, const std::int64_t maxval) {
    const T *data = &vectorDataPtr->get<T> (0);
    for (std::size_t i = 0, ei = vectorDataPtr->getElementCount (); i < ei; ++i) {
        if (*data < minval || *data > maxval)
            throw TRuntimeError (TRuntimeResult::TCode::RangeCheck, std::string ());
        ++data;
    }
}

void TPCodeInterpreter::performVectorRev () {
    TVectorDataPtr vectorDataPtr (calculatorStack.pVal<0> (), false);
    const std::size_t size = vectorDataPtr->getElementSize (), count = vectorDataPtr->getElementCount ();
    if (count > 1) {
        vectorDataPtr.copyOnWrite ();
        unsigned char *p0 = &vectorDataPtr->get<unsigned char> (0), *p1 = p0 + (count - 1) * size;
        while (p1 > p0) {
            std::swap_ranges (p0, p0 + size, p1);
            p0 += size;
            p1 -= size;
        }
    }    
    calculatorStack.pVal<0> () = vectorDataPtr.makeRef ();   
}

void TPCodeInterpreter::performInvertVec (bool isLogical) {
    TVectorDataPtr vectorDataPtr (calculatorStack.pVal<0> (), false);
    vectorDataPtr.copyOnWrite ();
    unsigned char *p = &vectorDataPtr->get<unsigned char> (0);
    const std::size_t n = vectorDataPtr->getElementSize () * vectorDataPtr->getElementCount ();
    if (isLogical)
        for (std::size_t i = 0; i < n; i++)
            p [i] = !p [i];
    else
        for (std::size_t i = 0; i < n; i++)
            p [i] = ~p [i];
    calculatorStack.pVal<0> () = vectorDataPtr.makeRef ();
}

void TPCodeInterpreter::performVectorRangeCheck (TPCode::TScalarTypeCode typecode) {
    std::int64_t maxval = calculatorStack.fetchInt64 (),
                 minval = calculatorStack.fetchInt64 ();
    const TVectorDataPtr vectorDataPtr (calculatorStack.pVal<0> ());
    static std::map<TPCode::TScalarTypeCode, void (TPCodeInterpreter::*) (const TVectorDataPtr, std::int64_t, std::int64_t)> rangeCheckOp = {
        {TPCode::TScalarTypeCode::s64, &TPCodeInterpreter::performVectorRangeCheck<std::int64_t>},
        {TPCode::TScalarTypeCode::s32, &TPCodeInterpreter::performVectorRangeCheck<std::int32_t>},
        {TPCode::TScalarTypeCode::s16, &TPCodeInterpreter::performVectorRangeCheck<std::int16_t>},
        {TPCode::TScalarTypeCode::s8 , &TPCodeInterpreter::performVectorRangeCheck<std::int8_t>},
        {TPCode::TScalarTypeCode::u8 , &TPCodeInterpreter::performVectorRangeCheck<std::uint8_t>},
        {TPCode::TScalarTypeCode::u16, &TPCodeInterpreter::performVectorRangeCheck<std::uint16_t>},
        {TPCode::TScalarTypeCode::u32, &TPCodeInterpreter::performVectorRangeCheck<std::uint32_t>}
    };
    (this->*rangeCheckOp [typecode]) (vectorDataPtr, minval, maxval);
}

inline TStackMemory *TPCodeInterpreter::lookupDisplay (const TPCodeOperation &op) {
    return display [op.getPara1 ()] + op.getPara2 ();
}

template<typename T> inline void TPCodeInterpreter::incMemOp (const TPCodeOperation &op) {
    ++*reinterpret_cast<T *> (lookupDisplay (op));
}

template<typename T> inline void TPCodeInterpreter::decMemOp (const TPCodeOperation & op) {
    --*reinterpret_cast<T *> (lookupDisplay (op));
}

template<double (*fn)(double)> inline void TPCodeInterpreter::applyFunction () {
    calculatorStack.fVal<0> () = fn (calculatorStack.fVal<0> ());
}

//

void TPCodeInterpreter::callRoutine (std::size_t pos) {
    stack.push (std::size_t (0));
    run (pos);
}

void TPCodeInterpreter::performStringIndex () {
    std::size_t index = calculatorStack.fetchInt64 ();
    if (void *p = calculatorStack.fetchPtr ()) {
        std::string &s = static_cast<TAnyValue *> (p)->get<std::string> ();
        if (index >= 1 && index <= s.length ()) {
            calculatorStack.pushPtr (&s [index - 1]);
            return;
        } else if (!index) {
            calculatorStack.pushInt64 (s.length ());
            return;
        }
    }
    throw TRuntimeError (TRuntimeResult::TCode::RangeCheck, "Index " + std::to_string (index) + " out of range");
}

void TPCodeInterpreter::performStringPtr () {
    static std::string empty;
    if (void *p = calculatorStack.fetchPtr ()) {
        TAnyValue *any = static_cast<TAnyValue *> (p);
        if (any->hasValue ())
            calculatorStack.pushPtr (any->get<std::string> ().data ());
        else
            calculatorStack.pushPtr (empty.data ());
    } else
        throw TRuntimeError (TRuntimeResult::TCode::InvalidArgument, "String is not initialized");
}

TRuntimeResult TPCodeInterpreter::resume () {
    return run (breakProgramCounter);
}

TRuntimeResult TPCodeInterpreter::run (std::size_t startAddress) {
    TStackMemory *basePtr = breakBasePtr;
    std::size_t programCounter = startAddress;
    TPCodeOperation *const progMem = &pcodeMemory.programMemory [0];
    
    try {
        // check if this is not a callback (startAddress != 0)
        
        // TODO: move to PCodeMemory !!!!
        if (!startAddress) 
            for (TExternalRoutine &ext: pcodeMemory.externalRoutines)
                ext.load ();
        for (;;) {
//            std::cout << "PC: " << programCounter << ", SP: " << stack.getStackPointer () << std::endl;
            TPCodeOperation &op = progMem [programCounter];

            #ifdef TRACE
                std::vector<std::string> listing;
                pcodeMemory.appendListing (listing, programCounter, programCounter + 1);
                std::cout << listing [0] << std::endl;
            #endif            

            ++programCounter;
            
            switch (op.getPCode ()) {
                case TPCode::TOperation::LoadInt64Const:
                    calculatorStack.pushInt64 (op.getPara1 ());
                    break;
                case TPCode::TOperation::LoadDoubleConst:
                    calculatorStack.pushDouble (op.getPara ());
                    break;
                case TPCode::TOperation::LoadExportAddr: 
                    calculatorStack.pVal<0> () = reinterpret_cast<void *> (pcodeMemory.getExportedRoutine (calculatorStack.iVal<0> ()));
                    break;
                //
                case TPCode::TOperation::LoadAddress:
                    calculatorStack.pushPtr (lookupDisplay (op));
                    break;
                case TPCode::TOperation::CalcIndex:
                    calculatorStack.iVal<1> () += (calculatorStack.iVal<0> () - op.getPara1 ()) * op.getPara2 ();
                    calculatorStack.pop ();
                    break;
                case TPCode::TOperation::StrIndex: 
                    performStringIndex ();
                    break;
                case TPCode::TOperation::StrPtr:
                    performStringPtr ();
                    break;
                //
                case TPCode::TOperation::LoadAcc:
                    calculatorStack.pushInt64 (acc);
                    break;
                case TPCode::TOperation::StoreAcc:
                    acc = calculatorStack.fetchInt64 ();
                    break;
                case TPCode::TOperation::LoadRuntimePtr:
                    calculatorStack.pushPtr (&pcodeMemory.getRuntimeData ());
                    break;
                //
                case TPCode::TOperation::LoadInt8:
                    calculatorStack.iVal<0> () = *static_cast<std::int8_t*> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadInt16:
                    calculatorStack.iVal<0> () = *static_cast<std::int16_t *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadInt32:
                    calculatorStack.iVal<0> () = *static_cast<std::int32_t *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadInt64:
                    calculatorStack.iVal<0> () = *static_cast<std::int64_t *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadUint8:
                    calculatorStack.iVal<0> () = *static_cast<std::uint8_t *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadUint16:
                    calculatorStack.iVal<0> () = *static_cast<std::uint16_t *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadUint32:
                    calculatorStack.iVal<0> () = *static_cast<std::uint32_t *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadDouble:
                    calculatorStack.fVal<0> () = *static_cast<double *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadSingle:
                    calculatorStack.fVal<0> () = *static_cast<float *> (calculatorStack.pVal<0> ());
                    break;
                case TPCode::TOperation::LoadVec:
                    calculatorStack.iVal<0> () = reinterpret_cast<std::intptr_t> (reinterpret_cast<TVectorDataPtr *> (calculatorStack.pVal<0> ())->makeRef ());
                    break;
                case TPCode::TOperation::LoadPtr:
                    calculatorStack.iVal<0> () = reinterpret_cast<std::intptr_t> (*static_cast<void **> (calculatorStack.pVal<0> ()));
                    break;
                //
                case TPCode::TOperation::Store8:
                    calculatorStack.storeStackTop<std::uint8_t> ();
                    break;
                case TPCode::TOperation::Store16:
                    calculatorStack.storeStackTop<std::uint16_t> ();
                    break;
                case TPCode::TOperation::Store32:
                    calculatorStack.storeStackTop<std::uint32_t> ();
                    break;
                case TPCode::TOperation::Store64:
                    calculatorStack.storeStackTop<std::int64_t> ();
                    break;
                case TPCode::TOperation::StoreDouble:
                    calculatorStack.storeStackTop<double> ();
                    break;
                case TPCode::TOperation::StoreSingle:
                    calculatorStack.storeStackTop<float> ();
                    break;
                case TPCode::TOperation::StoreVec:
                    calculatorStack.storeStackTop<TVectorDataPtr> ();
                    break;
                case TPCode::TOperation::StorePtr:
                    calculatorStack.storeStackTop<void *> ();
                    break;
                //
                case TPCode::TOperation::Push8:
                    stack.push (static_cast<std::uint8_t> (calculatorStack.fetchInt64 ()));
                    break;
                case TPCode::TOperation::Push16:
                    stack.push (static_cast<std::uint16_t> (calculatorStack.fetchInt64 ()));
                    break;
                case TPCode::TOperation::Push32:
                    stack.push (static_cast<std::uint32_t> (calculatorStack.fetchInt64 ()));
                    break;
                case TPCode::TOperation::Push64:
                    stack.push (calculatorStack.fetchInt64 ());
                    break;
                case TPCode::TOperation::PushDouble:
                    stack.push (calculatorStack.fetchDouble ());
                    break;
                case TPCode::TOperation::PushSingle:
                    stack.push (static_cast<float> (calculatorStack.fetchDouble ()));
                    break;
                case TPCode::TOperation::PushVec:
                    // same
                case TPCode::TOperation::PushPtr:
                    stack.push (calculatorStack.fetchPtr ());
                    break;
                //
                case TPCode::TOperation::CopyMem:
                    memcpy (calculatorStack.pVal<1> (), calculatorStack.pVal<0> (), op.getPara1 ());
                    calculatorStack.pop (2);
                    break;
                case TPCode::TOperation::CopyMemAny:
                case TPCode::TOperation::CopyMemVec:
                    rt_copy_mem (calculatorStack.pVal<1> (), calculatorStack.pVal<0> (), op.getPara1 (), op.getPara2 (), true, &pcodeMemory.getRuntimeData ());
                    calculatorStack.pop (2);
                    break;
                case TPCode::TOperation::PushMem:
                    memcpy (stack.allocate (op.getPara1 (), false), calculatorStack.fetchPtr (), op.getPara1 ());
                    break;
                case TPCode::TOperation::PushMemAny:
                case TPCode::TOperation::PushMemVec: 
                    rt_copy_mem (stack.allocate (op.getPara1 (), false), calculatorStack.fetchPtr (), op.getPara1 (), op.getPara2 (), false, &pcodeMemory.getRuntimeData ());
                    break; 
                case TPCode::TOperation::AlignStack:
                    stack.align (op.getPara1 ());
                    break;
                case TPCode::TOperation::Destructors:
                    pcodeMemory.getRuntimeData ().getAnyManager (op.getPara1 ())->destroy (basePtr);
                    break;
                case TPCode::TOperation::ReleaseStack:
                    stack.release (op.getPara1 ());
                    break;
                case TPCode::TOperation::Alloc: 
                    *reinterpret_cast<void **> (calculatorStack.fetchPtr ()) = pcodeMemory.getRuntimeData ().allocateMemory (calculatorStack.fetchInt64 (), op.getPara1 (), op.getPara2 ());
                    break;
                case TPCode::TOperation::Free:
                    pcodeMemory.getRuntimeData ().releaseMemory (calculatorStack.fetchPtr ());
                    break;
                //
                case TPCode::TOperation::CallStack:
                case TPCode::TOperation::Call:
                    stack.push (programCounter);
                    programCounter = op.getPCode () == TPCode::TOperation::CallStack ? calculatorStack.fetchInt64 () : op.getPara1 ();
                    break;                
                case TPCode::TOperation::CallFFI:
                    pcodeMemory.externalRoutines [op.getPara1 ()].callFFI (basePtr, *this);
                    break;
                case TPCode::TOperation::CallExtern:
                    stack.align (sizeof (void *));
                    stack.allocate (2 * sizeof (void *), false);
                    pcodeMemory.externalRoutines [op.getPara1 ()].callDirect (stack.getStackPointer (), &pcodeMemory.getRuntimeData ());
                    if (op.getPara2 ())
                        pcodeMemory.getRuntimeData ().getAnyManager (op.getPara2 ())->destroy (stack.getStackPointer ());
                    stack.release (2 * sizeof (void *));
                    break;
                case TPCode::TOperation::Enter:
                    if (static_cast<std::size_t> (op.getPara1 ()) >= display.size ())
                        display.resize (op.getPara1 () + 1);
                    if (op.getPara1 () > 1) {
                        stack.push (display [op.getPara1 ()], basePtr);
                        basePtr = display [op.getPara1 ()] = stack.getStackPointer ();
                        stack.allocate (op.getPara2 (), op.getPara1 () > 1);
                    }
                    break;
                case TPCode::TOperation::Leave:
                    if (op.getPara1 () > 1) {                    
                        stack.release (op.getPara2 ());
                        basePtr = stack.pop<TStackMemory *> ();
                        display [op.getPara1 ()] = stack.pop<TStackMemory *> ();
                    }
                    break;
                case TPCode::TOperation::Return:
                    programCounter = stack.pop<decltype (programCounter)> ();
                    stack.release (op.getPara1 ());
                    if (!programCounter)
                         return TRuntimeResult (TRuntimeResult::TCode::Done, programCounter - 1, std::string ());
                    break;
                case TPCode::TOperation::Jump:
                    programCounter = op.getPara1 ();
                    break;
                case TPCode::TOperation::JumpZero:
                    if (!calculatorStack.fetchInt64 ())
                        programCounter = op.getPara1 ();
                    break;
                case TPCode::TOperation::JumpNotZero:
                    if (calculatorStack.fetchInt64 ())
                        programCounter = op.getPara1 ();
                    break;
                case TPCode::TOperation::JumpAccEqual:
                    if (acc == op.getPara1 ())
                        programCounter = op.getPara2 ();
                    break;
                case TPCode::TOperation::CmpAccRange:
                    calculatorStack.pushInt64 (acc >= op.getPara1 () && acc <= op.getPara2 ());
                    break;
                // 
                case TPCode::TOperation::AddInt64Const:
                    calculatorStack.iVal<0> () += op.getPara1 ();
                    break;
                case TPCode::TOperation::Dup:
                    calculatorStack.dup ();
                    break;
                //
                case TPCode::TOperation::AddInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::plus> ();
                    break;
                case TPCode::TOperation::SubInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::minus> ();
                    break;
                case TPCode::TOperation::OrInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::bit_or> ();
                    break;
                case TPCode::TOperation::XorInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::bit_xor> ();
                    break;
                case TPCode::TOperation::MulInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::multiplies> ();
                    break;
                case TPCode::TOperation::DivInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::divides> ();
                    break;
                case TPCode::TOperation::ModInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::modulus> (); 
                    break;
                case TPCode::TOperation::AndInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::bit_and> ();
                    break;
                case TPCode::TOperation::ShlInt:
                    calculatorStack.applyStackOperation<std::int64_t, bit_shl> ();
                    break;
                case TPCode::TOperation::ShrInt:
                    calculatorStack.applyStackOperation<std::int64_t, bit_shr> ();
                    break;
                //
                case TPCode::TOperation::EqualInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::equal_to> ();
                    break;
                case TPCode::TOperation::NotEqualInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::not_equal_to> ();
                    break;
                case TPCode::TOperation::LessInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::less> ();
                    break;
                case TPCode::TOperation::GreaterInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::greater> ();
                    break;
                case TPCode::TOperation::LessEqualInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::less_equal> ();
                    break;
                case TPCode::TOperation::GreaterEqualInt:
                    calculatorStack.applyStackOperation<std::int64_t, std::greater_equal> ();
                    break;
                //
                case TPCode::TOperation::NotInt:
                    calculatorStack.applyStackTopOperation<std::int64_t, std::bit_not> ();
                    break;
                case TPCode::TOperation::NegInt:
                    calculatorStack.applyStackTopOperation<std::int64_t, std::negate> ();
                    break;
                case TPCode::TOperation::NegDouble:
                    calculatorStack.applyStackTopOperation<double, std::negate> ();
                    break;
                case TPCode::TOperation::NotBool:
                    calculatorStack.applyStackTopOperation<std::int64_t, std::logical_not> ();
                    break;
                //
                case TPCode::TOperation::AddDouble:
                    calculatorStack.applyStackOperation<double, std::plus> ();
                    break;
                case TPCode::TOperation::SubDouble:
                    calculatorStack.applyStackOperation<double, std::minus> ();
                    break;
                case TPCode::TOperation::MulDouble:
                    calculatorStack.applyStackOperation<double, std::multiplies> ();
                    break;
                case TPCode::TOperation::DivDouble:
                    calculatorStack.applyStackOperation<double, std::divides> ();
                    break;
                //
                case TPCode::TOperation::EqualDouble:
                    calculatorStack.applyStackOperation<double, std::equal_to> ();
                    break;
                case TPCode::TOperation::NotEqualDouble:
                    calculatorStack.applyStackOperation<double, std::not_equal_to> ();
                    break;
                case TPCode::TOperation::LessDouble:
                    calculatorStack.applyStackOperation<double, std::less> ();
                    break;
                case TPCode::TOperation::GreaterDouble:
                    calculatorStack.applyStackOperation<double, std::greater> ();
                    break;
                case TPCode::TOperation::LessEqualDouble:
                    calculatorStack.applyStackOperation<double, std::less_equal> ();
                    break;
                case TPCode::TOperation::GreaterEqualDouble:
                    calculatorStack.applyStackOperation<double, std::greater_equal> ();
                    break;
                //
                case TPCode::TOperation::ConvertIntDouble:
                    calculatorStack.fVal<0> () = calculatorStack.iVal<0> ();
                    break;
                //
                case TPCode::TOperation::MakeVec:
                case TPCode::TOperation::MakeVecMem: {
                    TVectorDataPtr vectorData (op.getPara1 (), 1, pcodeMemory.getRuntimeData ().getAnyManager (op.getPara2 ()));
                    if (op.getPCode () == TPCode::TOperation::MakeVec) {
                        // Will do a move for a TAnyValue
                        std::memcpy (&vectorData->get<unsigned char> (0), &calculatorStack.iVal<0> (), vectorData->getElementSize ());
                        calculatorStack.pop ();
                    } else
                        vectorData->setElement (0, calculatorStack.fetchPtr ());
                    calculatorStack.pushPtr (vectorData.makeRef ()); 
                    break; }
                case TPCode::TOperation::MakeVecZero: {
                    TVectorDataPtr vectorData (op.getPara1 (), op.getPara2 ());
                    std::memset (&vectorData->get<unsigned char> (0), 0, vectorData->getElementSize () * vectorData->getElementCount ());
                    calculatorStack.pushPtr (vectorData.makeRef ()); 
                    break; }
                case TPCode::TOperation::ConvertVec:
                    applyVectorConversion (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()));
                    break;
                case TPCode::TOperation::CombineVec:
                    performCombine (op);
                    break;
                case TPCode::TOperation::ResizeVec:
                    performResize (op);
                    break;
                //
                case TPCode::TOperation::VecIndexIntVec: 
                    applyIndexIntVec (op);
                    break;
                case TPCode::TOperation::VecIndexBoolVec:
                    applyIndexBoolVec ();
                    break;
                case TPCode::TOperation::VecIndexInt:
                    applyIndexInt ();
                    break;
                //

                case TPCode::TOperation::NotIntVec:
                    performInvertVec (false);
                    break;
                case TPCode::TOperation::NotBoolVec:
                    performInvertVec (true);
                    break;
    #ifdef USE_VECTOR_JIT
                case TPCode::TOperation::AddVec:
                case TPCode::TOperation::SubVec:
                case TPCode::TOperation::OrVec:
                case TPCode::TOperation::XorVec:
                case TPCode::TOperation::MulVec:
                case TPCode::TOperation::DivVec:
                case TPCode::TOperation::DivIntVec:
                case TPCode::TOperation::ModVec:
                case TPCode::TOperation::AndVec:
                    
                case TPCode::TOperation::EqualVec:
                case TPCode::TOperation::NotEqualVec:
                case TPCode::TOperation::LessVec:
                case TPCode::TOperation::GreaterVec:
                case TPCode::TOperation::LessEqualVec:
                case TPCode::TOperation::GreaterEqualVec:
                    applyVectorOperation (op.getPCode (), static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()));
                    break;
    #else
                case TPCode::TOperation::AddVec:
                    applyVectorOperation<std::plus> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::SubVec:
                    applyVectorOperation<std::minus> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::OrVec:
                    applyVectorOperation<std::bit_or<std::int64_t>> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::XorVec:
                    applyVectorOperation<std::bit_xor<std::int64_t>> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::MulVec:
                    applyVectorOperation<std::multiplies> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::DivVec:
                    applyVectorOperation<std::divides<double>> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::DivIntVec:
                    applyVectorOperation<std::divides<std::int64_t>> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::ModVec:
                    applyVectorOperation<std::modulus<std::int64_t>> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                case TPCode::TOperation::AndVec:
                    applyVectorOperation<std::bit_and<std::int64_t>> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), false);
                    break;
                    
                case TPCode::TOperation::EqualVec:
                    applyVectorOperation<std::equal_to> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), true);
                    break;
                case TPCode::TOperation::NotEqualVec:
                    applyVectorOperation<std::not_equal_to> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), true);
                    break;
                case TPCode::TOperation::LessVec:
                    applyVectorOperation<std::less> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), true);
                    break;
                case TPCode::TOperation::GreaterVec:
                    applyVectorOperation<std::greater> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), true);
                    break;
                case TPCode::TOperation::LessEqualVec:
                    applyVectorOperation<std::less_equal> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), true);
                    break;
                case TPCode::TOperation::GreaterEqualVec:
                    applyVectorOperation<std::greater_equal> (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()), static_cast<TPCode::TScalarTypeCode> (op.getPara2 ()), true);
                    break;
    #endif	// USE_VECTOR_JIT
    
                case TPCode::TOperation::VecLength:
                    if (calculatorStack.pVal<0> ()) {
                        const std::size_t length = TVectorDataPtr (calculatorStack.fetchPtr (), false)->getElementCount ();
                        calculatorStack.pushInt64 (length);
                    } else
                        calculatorStack.iVal<0> () = 0  ;
                    break;
                case TPCode::TOperation::VecRev:
                    performVectorRev ();
                    break;
                case TPCode::TOperation::RangeCheckVec:
                    performVectorRangeCheck (static_cast<TPCode::TScalarTypeCode> (op.getPara1 ()));
                    break;
                //
                case TPCode::TOperation::RangeCheck:
                    if (calculatorStack.iVal<0> () < op.getPara1 () || calculatorStack.iVal<0> () > op.getPara2 ()) 
                        throw TRuntimeError (TRuntimeResult::TCode::RangeCheck, "Value " + std::to_string (calculatorStack.iVal<0> ()) + " is out of range " + std::to_string (op.getPara1 ()) + " to " + std::to_string (op.getPara2 ()));
                    break;
                case TPCode::TOperation::NoOperation:
                    break;
                case TPCode::TOperation::Stop:
                    return TRuntimeResult (TRuntimeResult::TCode::Done, programCounter - 1, std::string ());
                case TPCode::TOperation::Break:
                    breakProgramCounter = programCounter;
                    breakBasePtr = basePtr;
                    return TRuntimeResult (TRuntimeResult::TCode::Breakpoint, programCounter - 1, std::string ());
                case TPCode::TOperation::Halt:
                    return TRuntimeResult (TRuntimeResult::TCode::Halted, programCounter - 1, std::string ());
                case TPCode::TOperation::IllegalOperation:
                default:
                    return TRuntimeResult (TRuntimeResult::TCode::InvalidOpcode, programCounter - 1, std::string ());
            }
        } 
    }
    
    catch (const TRuntimeError &runtimeError) {
//    
        TRuntimeResult result (runtimeError.getRuntimeResultCode (), std::max<std::size_t> (1, programCounter) - 1, runtimeError.getAdditionalMessage ());
        std::cout << result.getMessage () << " at address " << result.getProgramCounter () << std::endl;
        exit (1);
//    
    
        return TRuntimeResult (runtimeError.getRuntimeResultCode (), std::max<std::size_t> (1, programCounter) - 1, runtimeError.getAdditionalMessage ());
    }
    catch (const std::bad_alloc &exception) {
        std::cout << "OUT OF MEM" << std::endl; exit (1);
        return TRuntimeResult (TRuntimeResult::TCode::OutOfMemory, std::max<std::size_t> (1, programCounter) - 1, std::string ());
    }
    catch (const std::exception &exception) {
        std::cout << "EXCEPTION!" << exception.what () << std::endl; exit (1);
    }
}

}
