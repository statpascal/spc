/** stacks.hpp
*/

#pragma once

#include <cstddef>
#include <vector>

#include "anyvalue.hpp"
#include "vectordata.hpp"
#include "runtimeerr.hpp"
#include "config.hpp"

namespace statpascal {

using TStackMemory = unsigned char;

class TStack final {
public:
    TStack (std::size_t stacksize);
    ~TStack ();
    
    TStackMemory *allocate (std::size_t, bool zeroMemory);
    void release (std::size_t);
    
    TStackMemory *getStackPointer () const;
    void align (std::size_t);
    template<typename T> void push (T val);
    template<typename T> void push (T val1, T val2);
    template<typename T> T pop ();
    
private:
    const std::size_t stacksize;
    TStackMemory *arena, *sp;
};


class TCalculatorStack final {
public:
    TCalculatorStack ();

    void pushInt64 (std::int64_t);
    void pushDouble (double);
    void pushPtr (void *);
    
    void pop ();
    void pop (std::size_t n);
    
    using voidptr = void *;
    std::int64_t fetchInt64 ();
    double fetchDouble ();
    voidptr fetchPtr ();
    
    template<std::size_t offset> std::int64_t &iVal ();
    template<std::size_t offset> double &fVal ();
    template<std::size_t offset> voidptr &pVal ();
    
    std::int64_t &iVal (std::size_t offset);
    double &fVal (std::size_t offset);
    voidptr &pVal (std::size_t offset);
    
    template<typename T, template <typename> class TOp> void applyStackOperation ();
    template<typename T, template <typename> class TOp> void applyStackTopOperation ();
    template<typename T> void storeStackTop (TStackMemory *dest);
    
    /** store address is second on stack */
    template<typename T> void storeStackTop ();
    
    void dup ();
    
private:
    void checkSize ();
    void resize ();
    
    std::vector<std::int64_t> stack;
    std::int64_t *sp, *sptop;
    std::size_t stacksize;
};

// inlines

inline TStackMemory *TStack::allocate (std::size_t n, bool zeroMemory) {
    TStackMemory *res = sp;
    sp += n;
    if (sp > arena + stacksize)
        throw TRuntimeError (TRuntimeResult::TCode::StackOverflow);
    if (zeroMemory)
        std::fill (res, sp, 0);
    return res;
}

inline void TStack::align (std::size_t n) {
    std::size_t align1 = (n >= TConfig::alignment) ? TConfig::alignment - 1 : n - 1;
    sp = reinterpret_cast<TStackMemory *> ((reinterpret_cast<std::uint64_t> (sp) + align1) & ~align1);
}

inline void TStack::release (std::size_t n) {
    sp -= n;
}

inline TStackMemory *TStack::getStackPointer () const {
    return sp;
}

template<typename T> inline void TStack::push (T val) {
    align (sizeof (T));
    if (sizeof (T) + (sp - arena) > stacksize)
        throw TRuntimeError (TRuntimeResult::TCode::StackOverflow);
    *reinterpret_cast<T *> (sp) = val;
    sp += sizeof (T);
}

template<typename T> inline T TStack::pop () {
    sp -= sizeof (T);
    return *reinterpret_cast<T *> (sp);
}

template<typename T> void TStack::push (T val1, T val2) {
    align (sizeof (T));
    if (2 * sizeof (T) + (sp - arena) > stacksize)
        throw TRuntimeError (TRuntimeResult::TCode::StackOverflow);
    memcpy (sp, &val1, sizeof (T));
    memcpy (sp + sizeof (T), &val2, sizeof (T));
    sp += 2 * sizeof (T);
}

// 

inline void TCalculatorStack::checkSize () {
    if (sp == sptop) 
        resize ();
}

inline void TCalculatorStack::pushInt64 (const std::int64_t n) {
    checkSize ();
    *++sp = n;
}

inline void TCalculatorStack::pushDouble (const double f) {
    checkSize ();
    *reinterpret_cast<double *> (++sp) = f;
}

inline void TCalculatorStack::pushPtr (void *const p) {
    checkSize ();
    *++sp = reinterpret_cast<std::intptr_t> (p);
}

inline void TCalculatorStack::pop () {
    --sp;
}

inline void TCalculatorStack::pop (const std::size_t n) {
    sp -= n;
}

inline std::int64_t TCalculatorStack::fetchInt64 () {
    return *sp--;
}

inline double TCalculatorStack::fetchDouble () {
    return *static_cast<double *> (static_cast<void *> (sp--));
}

inline TCalculatorStack::voidptr TCalculatorStack::fetchPtr () {
    return *static_cast<voidptr *> (static_cast<void *> (sp--));
}

template<std::size_t offset> inline std::int64_t &TCalculatorStack::iVal () {
    return *(sp - offset);
}

template<std::size_t offset> inline double &TCalculatorStack::fVal () {
    return *static_cast<double *> (static_cast<void *> (sp - offset));
}

template<std::size_t offset> inline TCalculatorStack::voidptr &TCalculatorStack::pVal () {
    return *static_cast<voidptr *> (static_cast<void *> (sp - offset));
}

inline std::int64_t &TCalculatorStack::iVal (std::size_t offset) {
    return *(sp - offset);
}

inline double &TCalculatorStack::fVal (std::size_t offset) {
    return *static_cast<double *> (static_cast<void *> (sp - offset));
}

inline TCalculatorStack::voidptr &TCalculatorStack::pVal (std::size_t offset) {
    return *static_cast<voidptr *> (static_cast<void *> (sp - offset));
}

template<typename T, template <typename> class TOp> inline void TCalculatorStack::applyStackTopOperation () {
    *reinterpret_cast<T *> (sp) = TOp<T> () (*reinterpret_cast<T *> (sp));
}

template<> inline void TCalculatorStack::applyStackOperation<std::int64_t, std::plus> () {
    const std::int64_t b = *sp--, a = *sp;
    if (__builtin_add_overflow (a, b, sp))
        throw TRuntimeError (TRuntimeResult::TCode::Overflow);
}

template<> inline void TCalculatorStack::applyStackOperation<std::int64_t, std::minus> () {
    const std::int64_t b = *sp--, a = *sp;
    if (__builtin_sub_overflow (a, b, sp))
        throw TRuntimeError (TRuntimeResult::TCode::Overflow);
}

template<> inline void TCalculatorStack::applyStackOperation<std::int64_t, std::multiplies> () {
    const std::int64_t b = *sp--, a = *sp;
    if (__builtin_mul_overflow (a, b, sp))
        throw TRuntimeError (TRuntimeResult::TCode::Overflow);
}

template<typename T, template <typename> class TOp> inline void TCalculatorStack::applyStackOperation () {
    using TOpRes = decltype (TOp<T> () (T (), T ()));
    using TRes = typename std::conditional<std::is_same<TOpRes, bool>::value, std::int64_t, TOpRes>::type;
    
    const T val2 = *reinterpret_cast<T *> (sp--);
    const T val1 = *reinterpret_cast<T *> (sp);
    *reinterpret_cast<TRes *> (sp) = TOp<T> () (val1, val2);
}

template<typename T> inline void TCalculatorStack::storeStackTop (TStackMemory *dest) {
//#ifdef IS_BIG_ENDIAN
//    memcpy (dest, reinterpret_cast<unsigned char *> (sp--) + sizeof (std::int64_t) - sizeof (T), sizeof (T));
//#else
//    memcpy (dest, sp--, sizeof (T));
//#endif
    *reinterpret_cast<T *> (dest) = *reinterpret_cast<T *> (sp--);
}

template <> inline void TCalculatorStack::storeStackTop<float> (TStackMemory *dest) {
    *reinterpret_cast<float *> (dest) = *reinterpret_cast<double *> (sp--);
}

template<> inline void TCalculatorStack::storeStackTop<TVectorDataPtr> (TStackMemory *dest) {
    reinterpret_cast<TVectorDataPtr *> (dest)->releaseRef ();
    storeStackTop<voidptr> (dest);
}

template<typename T> inline void TCalculatorStack::storeStackTop () {
    **reinterpret_cast<T **> (sp - 1) = *reinterpret_cast<T *> (sp);
    sp -= 2;
}

template<> inline void TCalculatorStack::storeStackTop<float> () {
    **reinterpret_cast<float **> (sp - 1) = *reinterpret_cast<double *> (sp);
    sp -= 2;
}

template<> inline void TCalculatorStack::storeStackTop<TVectorDataPtr> () {
    storeStackTop<TVectorDataPtr> (static_cast<TStackMemory *> (pVal<1> ()));
    pop ();
}

inline void TCalculatorStack::dup () {
    checkSize ();
    ++sp;
    memcpy (sp, sp - 1, sizeof (*sp));
}

}
