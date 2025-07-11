#include <cmath>
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <mutex>
#include <semaphore>
#include <functional>

#include "anyvalue.hpp"
#include "vectordata.hpp"
#include "rng.hpp"
#include "runtime.hpp"
#include "config.hpp"
#include "datatypes.hpp"

namespace {

const double MY_PI = 3.1415926;

struct TFileStruct {
    std::int64_t idx;
    statpascal::TAnyValue fn;
    std::int64_t blksize;
    bool binary;
};

struct TSet {
    std::uint64_t data [statpascal::TConfig::setwords];
};

using sp_bool = bool;

}

// string routines

namespace {

const std::string &getString (statpascal::TAnyValue &p) {
    static std::string empty;
    if (p.hasValue ())
        return p.get<std::string> ();
    else
        return empty;
}
    
statpascal::TAnyValue handle_str_copy (const char *s, std::int64_t pos, std::int64_t length) {
    const std::string t (s);
    if (pos <= 0)
        pos = 1;
    if (static_cast<std::size_t> (pos) <= t.length () && length > 0) 
        return t.substr (pos - 1, length);
    else
        return std::string ();
}

template<typename T> class bit_shl {
public:
    constexpr T operator () (const T &lhs, const T &rhs) const {
        return lhs << rhs;
    }
    using result_type = T;
};


template<typename T> class bit_shr {
public:
    constexpr T operator () (const T &lhs, const T &rhs) const {
        return lhs >> rhs;
    }
    using result_type = T;
};


}

extern "C" statpascal::TAnyValue rt_str_make (std::size_t index, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getStringConstant (index);
}

extern "C" statpascal::TAnyValue rt_str_concat (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return getString (a) + getString (b);
}

extern "C" statpascal::TAnyValue rt_str_char (std::uint8_t a) {
    return std::string (1, static_cast<std::string::value_type> (a));
}

extern "C" statpascal::TAnyValue rt_str_copy (const char *s, std::int64_t pos, std::int64_t length) {
    return handle_str_copy (s, pos, length);
}

extern "C" statpascal::TAnyValue rt_file_path (statpascal::TAnyValue t) {
    const std::string &s = getString (t);
    std::string::size_type sep = s.rfind ('/');
    return sep == std::string::npos ? std::string () : s.substr (0, sep + 1);
}

extern "C" statpascal::TAnyValue rt_file_name (statpascal::TAnyValue t) {
    const std::string &s = getString (t);
    std::string::size_type sep = s.rfind ('/');
    return sep == std::string::npos ? s : s.substr (sep + 1);
}

extern "C" statpascal::TAnyValue rt_str_of_char (std::uint8_t a, std::int64_t length) {
    return std::string (std::max<std::int64_t> (0, length), a);
}

extern "C" statpascal::TAnyValue rt_str_upcase (statpascal::TAnyValue a) {
    const std::string &t = getString (a);
    std::string s (t.length (), 0);
    std::transform (t.begin (), t.end (), s.begin (), ::toupper);
    return s;
}

extern "C" sp_bool rt_str_equal (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return getString (a) == getString (b);
}

extern "C" sp_bool rt_str_not_equal (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return getString (a) != getString (b);
}

extern "C" sp_bool rt_str_less (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return getString (a) < getString (b);
}

extern "C" sp_bool rt_str_greater (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return getString (a) > getString (b);
}

extern "C" sp_bool rt_str_less_equal (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return getString (a) <= getString (b);
}

extern "C" sp_bool rt_str_greater_equal (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return getString (a) >= getString (b);
}


extern "C" void rt_str_insert (statpascal::TAnyValue src, statpascal::TAnyValue *dest, std::int64_t pos) {
    const std::string &srcstr = getString (src);
    if (pos <= 0)
        pos = 1;
    if (!dest->hasValue ()) {
        if (pos == 1)
            *dest = src;
    } else if (pos > 0) {
        dest->copyOnWrite ();
        std::string &deststr = dest->get<std::string> ();
        if (static_cast<std::size_t> (pos) <= deststr.length ())
            deststr.insert (pos - 1, srcstr);
        else
            deststr.append (srcstr);
    }
}

extern "C" std::int64_t rt_str_length (statpascal::TAnyValue a) {
    return getString (a).size ();
}

extern "C" std::int64_t rt_str_pos (statpascal::TAnyValue needle, statpascal::TAnyValue haystack) {
    std::string::size_type pos = getString (haystack).find (getString (needle));
    return pos != std::string::npos ? pos + 1 : 0;
}

extern "C" void rt_str_delete (statpascal::TAnyValue *s, std::int64_t pos, std::int64_t length) {
    if (s->hasValue ()) {
        s->copyOnWrite ();
        std::string &t = s->get<std::string> ();
        if (pos >= 1 && static_cast<std::size_t> (pos) <= t.length () && length > 0)
            t.erase (pos - 1, length);
    }
}

// math functions

namespace {

template<typename T> std::int64_t sign (T n) {
    if (n > 0)
        return 1;
    if (n < 0)
        return -1;
    return 0;
}

}

extern "C" double rt_dbl_frac (double x) {
    return (x >= 0) ? x - std::floor (x) : x - std::ceil (x);
}

extern "C" std::int64_t rt_int_abs (std::int64_t n) {
    return (n >= 0) ? n : -n;
}

extern "C" double rt_dbl_abs (double a) {
    return (a >= 0) ? a : -a;
}

extern "C" std::int64_t rt_int_sign (std::int64_t n) {
    return sign<std::int64_t> (n);
}

extern "C" std::int64_t rt_dbl_sign (double x) {
    return sign<double> (x);
}

extern "C" std::int64_t rt_int_sqr (std::int64_t n) {
    return n * n;
}

extern "C" double rt_dbl_sqr (double x) {
    return x * x;
}

extern "C" std::int64_t rt_dbl_trunc (double x) {
    return std::trunc (x);
}

extern "C" std::int64_t rt_dbl_round (double x) {
    return std::round (x);
}

extern "C" char rt_char_upcase (char c) {
    return std::toupper (c);
}

// RNGs

extern "C" void rt_randomize () {
    statpascal::TRNG::randomize ();
}

extern "C" void rt_randomize_seed (std::int64_t n) {
    statpascal::TRNG::randomize (n);
}

extern "C" double rt_dbl_random () {
    return statpascal::TRNG::val ();
}

extern "C" std::int64_t rt_int_random (std::int64_t n) {
    return statpascal::TRNG::val (0, n - 1);
}

extern "C" sp_bool rt_int_odd (std::int64_t n) {
    return n & 1;
}

extern "C" void rt_dyn_alloc (void **p, std::int64_t size) {
    *p = malloc (size);
}

extern "C" void rt_dyn_free (void *p) {
    free (p);
}

extern "C" void rt_fillchar (void *p, std::int64_t size, std::uint8_t value) {
    std::memset (p, value, size);
}

extern "C" void rt_move (void *src, void *dst, std::int64_t size) {
    std::memmove (dst, src, size);
}

extern "C" std::int64_t rt_compare_byte (void *src, void *dst, std::int64_t size) {
    return std::memcmp (dst, src, size);
}

extern "C" std::int64_t rt_val_int (const char *s, std::uint16_t *code) {
    char *endptr;
    std::int64_t result = std::strtoll (s, &endptr, 10);
    if (*endptr) {
        result = 0;
        *code = endptr - s + 1;
    } else
        *code = 0;
    return result;
}

extern "C" double rt_val_dbl (const char *s, std::uint16_t *code) {
    char *endptr;
    double result = std::strtod (s, &endptr);
    if (*endptr) {
        result = 0.0;
        *code = endptr - s + 1;
    } else
        *code = 0;
    return result;

/*    
    const std::string s (t);
    std::size_t pos;
    try {
        *a = std::stod (s, &pos);
        if (pos == s.length ())
            *code = 0;
        else
            *code = pos + 1;
    }
    catch (...) {
        *code = 1;
    }
*/
}

extern "C" std::int64_t rt_min_int (std::int64_t a, std::int64_t b) {
    return std::min (a, b);
}

extern "C" double rt_min_dbl (double a, double b) {
    return std::min (a, b);
}

extern "C" std::int64_t rt_max_int (std::int64_t a, std::int64_t b) {
    return std::max (a, b);
}

extern "C" double rt_max_dbl (double a, double b) {
    return std::max (a, b);
}

// vector routines

namespace {

inline void initOutVector (statpascal::TVectorDataPtr &out, std::size_t size, std::size_t count) {
    if (!out.hasValue () || out->getRefCount () > 1 || out->getElementSize () != size || out->getElementCount () != count)
        out = statpascal::TVectorDataPtr (size, count, nullptr, false);
}

template<typename TEl, typename TRes> TRes vecsum (statpascal::TVectorDataPtr &in) {
    if (in.hasValue ())  {
        TEl *p = &(in->template get<TEl> (0));
        return std::accumulate (p, p + in->getElementCount (), TRes ());
    } else
        return TRes ();
}

template<typename T> statpascal::TVectorDataPtr veccumsum (const statpascal::TVectorDataPtr &a) {
    statpascal::TVectorDataPtr out (a->getElementSize (), a->getElementCount ());
    
    T *x = &(a->template get<T> (0)),
      *y = &(out->template get<T> (0));
    std::partial_sum (x, x + a->getElementCount (), y);
    
    return out;
}

template<typename T> statpascal::TVectorDataPtr vecsort (statpascal::TVectorDataPtr &a) {
    const std::size_t size = a->getElementSize (), count = a->getElementCount ();
    statpascal::TVectorDataPtr out (size, count);
    
    T *x = &(a->template get<T> (0)),
      *y = &(out->template get<T> (0));
    std::memcpy (y, x, size * count);
    std::sort (y, y + count);
    
    return out;
}

statpascal::TVectorDataPtr vecfunc (std::function<double (double)> fn, statpascal::TVectorDataPtr &in) {
    statpascal::TVectorDataPtr out;
    initOutVector (out, in->getElementSize (), in->getElementCount ());
    double *x = &(in->template get<double> (0)),
           *y = &(out->template get<double> (0));
    for (std::size_t i = 0, ei = in->getElementCount (); i < ei; ++i)
        y [i] = fn (x [i]);
    return out;
}

} // namespace

extern "C" statpascal::TVectorDataPtr rt_vdbl_sqr (statpascal::TVectorDataPtr in) {
    return vecfunc ([] (double x) {return x * x; }, in);
}

extern "C" statpascal::TVectorDataPtr rt_vdbl_sqrt (statpascal::TVectorDataPtr in) {
    return vecfunc ([] (double x) {return std::sqrt (x); }, in);
}

extern "C" statpascal::TVectorDataPtr rt_vdbl_sin (statpascal::TVectorDataPtr in) {
    return vecfunc ([] (double x) {return std::sin (x); }, in);
}

extern "C" statpascal::TVectorDataPtr rt_vdbl_cos (statpascal::TVectorDataPtr in, statpascal::TVectorDataPtr *out) {
    return vecfunc ([] (double x) {return std::cos (x); }, in);
}

extern "C" statpascal::TVectorDataPtr rt_vdbl_log (statpascal::TVectorDataPtr in, statpascal::TVectorDataPtr *out) {
    return vecfunc ([] (double x) {return std::log (x); }, in);
}

extern "C" statpascal::TVectorDataPtr rt_vdbl_pow (statpascal::TVectorDataPtr in, double e) {
    return vecfunc ([e] (double x) {return std::pow (x, e); }, in);
}

extern "C" void *rt_vec_index_int (statpascal::TVectorDataPtr in, std::int64_t index) {
    // TODO: range check
    return &in->get<char> (index - 1);
}

extern "C" statpascal::TVectorDataPtr rt_vec_index_vint (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr index) {
    // TODO: range check
    std::int64_t ival = 0;
    statpascal::TVectorDataPtr out (a->getElementSize (), index->getElementCount (), a->getElementAnyManager (), false);
    for (std::size_t i = 0; i < index->getElementCount (); ++i) {
        memcpy (&ival, &index->get<char> (i), index->getElementSize ());
        out->setElement (i, &a->get<unsigned char> (ival - 1));
    }
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_vec_index_vbool (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr index) {
    // TODO: range check
    
    // count number of true 8 bytes in parallel
    const std::size_t indexCount = index->getElementCount ();
    const bool *const indexData = &index->get<bool> (0);
    const std::uint64_t *const indexInt = &index->get<std::uint64_t> (0);
    std::uint64_t count = 0;
    std::size_t i, j = 0;
    if (indexCount >= 16) {
        const std::size_t ei = indexCount / 8;
        for (i = 0; i < ei; i += 255) {
            const std::size_t ej = std::min (i + 255, ei);
            std::uint64_t counter = 0;
            for (j = i; j < ej; ++j)
                counter += indexInt [j];
            for (int k = 0; k < 8; ++k) {
                count += counter & 0xff;
                counter >>= 8;
            }
        }
    }
    for (i = j * 8; i < indexCount; ++i)
        count += indexData [i];
        
    statpascal::TVectorDataPtr out (a->getElementSize (), count, a->getElementAnyManager (), false);
    if (count)
        for (std::size_t i = 0, dst = 0; dst < count; ++i) 
            if (indexData [i])
                out->setElement (dst++, &a->get<unsigned char> (i));
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_intvec (std::int64_t a, std::int64_t b) {
    statpascal::TVectorDataPtr out;
    initOutVector (out, sizeof (std::int64_t), std::max<std::int64_t> (0, b - a + 1));
    if (b >= a) {
        std::int64_t *p = &(out)->get<std::int64_t> (0);
        std::iota (p, p + (b - a + 1), a);
    }
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_realvec (double a, double b, double step) {
    statpascal::TVectorDataPtr out;
    if ((step > 0.0 && b >= a) || (step < 0.0 && b <= a)) {
        const std::size_t count = std::floor (1 + (b - a) / step + 0.5);
        initOutVector (out, sizeof (double), count);
        double *p = &(out)->get<double> (0);
        for (std::size_t i = 0; i < count; ++i)
            *p++ = static_cast<double> (count - 1 - i) / (count - 1) * a + static_cast<double> (i) / (count - 1) * b;
    } else
        initOutVector (out, sizeof (double), 0);
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_makevec_int (std::int64_t size, std::int64_t val) {
    statpascal::TVectorDataPtr out (size, 1);
    out->setElement (0, &val);
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_makevec_dbl (std::int64_t size, double val) {
    statpascal::TVectorDataPtr out (size, 1);
    if (size == sizeof (double))
        out->setElement (0, &val);
    else {
        float val1 = val;
        out->setElement (0, &val1);
    }
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_makevec_str (std::int64_t anyManagerIndex, statpascal::TRuntimeData *runtimeData, statpascal::TAnyValue a) {
    statpascal::TVectorDataPtr out (sizeof (void *), 1, runtimeData->getAnyManager (anyManagerIndex));
    out->setElement (0, &a);
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_makevec_vec (std::int64_t anyManagerIndex, statpascal::TRuntimeData *runtimeData, statpascal::TVectorDataPtr a) {
    statpascal::TVectorDataPtr out (sizeof (void *), 1, runtimeData->getAnyManager (anyManagerIndex));
    out->setElement (0, &a);
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_combinevec_4 (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, statpascal::TVectorDataPtr c, statpascal::TVectorDataPtr d) {
    const std::size_t n = 4;
    std::array<statpascal::TVectorDataPtr, n> in {a, b, c, d};
    std::int64_t count = 0, elsize = 0;
    statpascal::TAnyManager *anyManager = nullptr;
    for (std::size_t i = 0; i < n; ++i)
        if (in [i].hasValue ()) {
            count += in [i]->getElementCount ();
            elsize = in [i]->getElementSize ();
            anyManager = in [i]->getElementAnyManager ();
        }
    statpascal::TVectorDataPtr out (elsize, count, anyManager, false);
    std::int64_t index = 0;
    for (std::size_t i = 0; i < n; ++i)
        if (in [i].hasValue ())
            for (std::size_t j = 0; j < in [i]->getElementCount (); ++j)
                out->setElement (index++, in [i]->getElement (j));
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_combinevec_3 (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, statpascal::TVectorDataPtr c) {
    return rt_combinevec_4 (a, b, c, statpascal::TVectorDataPtr ());
}

extern "C" statpascal::TVectorDataPtr rt_combinevec_2 (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b) {
    return rt_combinevec_4 (a, b,  statpascal::TVectorDataPtr (),  statpascal::TVectorDataPtr ());
}

extern "C" std::int64_t rt_sizevec (statpascal::TVectorDataPtr a) {
    return a->getElementCount ();
}

extern "C" void rt_resizevec (std::int64_t anyManagerIndex, statpascal::TRuntimeData *runtimeData, std::int64_t elsize, statpascal::TVectorDataPtr &a, std::int64_t n) {
    statpascal::TVectorDataPtr out (elsize, n, runtimeData->getAnyManager (anyManagerIndex));
    if (a.hasValue ()) {
        const std::size_t copyCount = std::min<std::size_t> (a->getElementCount (), n);
        if (a->getElementAnyManager ())
            for (std::size_t j = 0; j < copyCount; ++j)
                out->setElement (j, a->getElement (j));
        else
            std::memcpy (out->getElement (0), a->getElement (0), copyCount * a->getElementSize ());
    }
    a = out;
}

extern "C" statpascal::TVectorDataPtr rt_revvec (statpascal::TVectorDataPtr a) {
    const std::size_t n = a->getElementCount ();
    statpascal::TVectorDataPtr out (a->getElementSize (), n, a->getElementAnyManager ());
    for (std::size_t i = 0; i < n; ++i)
        out->setElement (i, a->getElement (n - 1 - i));
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_vint_randomperm (std::int64_t n) {
    statpascal::TVectorDataPtr out;
    initOutVector (out, sizeof (std::int64_t), std::max<std::int64_t> (n, 0));
    if (n >= 1) {
        std::int64_t *p = &(out)->get<std::int64_t> (0);
        std::iota (p, p + n, 1);
        for (std::int64_t i = n - 1; i > 0; --i)
            std::swap (p [i], p [statpascal::TRNG::val (0, i)]);
    }
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_vdbl_random (std::int64_t n) {
    statpascal::TVectorDataPtr out;
    initOutVector (out, sizeof (double), std::max<std::int64_t> (n, 0));
    if (n >= 1) {
        double *p = &(out)->get<double> (0);
        for (std::int64_t i = 0; i <n; ++i)
            p [i] = statpascal::TRNG::val ();
    }
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_vint_random (std::int64_t m, std::int64_t n) {
    statpascal::TVectorDataPtr out;
    initOutVector (out, sizeof (std::int64_t), std::max<std::int64_t> (n, 0));
    if (n >= 1) {
        std::int64_t *p = &(out)->get<std::int64_t> (0);
        for (std::int64_t i = 0; i <n; ++i)
            p [i] = statpascal::TRNG::val (0, m);
    }
    return out;
}

extern "C" statpascal::TVectorDataPtr rt_vint_sort (statpascal::TVectorDataPtr a) {
    return vecsort<std::int64_t> (a);
}

extern "C"  statpascal::TVectorDataPtr rt_vdbl_sort (statpascal::TVectorDataPtr a) {
    return vecsort<double> (a);
}

extern "C" std::int64_t rt_vint_sum (statpascal::TVectorDataPtr in) {
    return vecsum<std::int64_t, std::int64_t> (in);
}

extern "C" double rt_vdbl_sum (statpascal::TVectorDataPtr in) {
    return vecsum<double, double> (in);
}

extern "C" std::int64_t rt_vbool_count (statpascal::TVectorDataPtr in) {
    return vecsum<bool, std::int64_t> (in);
}

extern "C" statpascal::TVectorDataPtr rt_vint_cumsum (statpascal::TVectorDataPtr a) {
    return veccumsum<std::int64_t> (a);
}

extern "C" statpascal::TVectorDataPtr rt_vdbl_cumsum (statpascal::TVectorDataPtr a) {
    return veccumsum<double> (a);
}

namespace {

template<class TOp> statpascal::TVectorDataPtr applyVectorOperation (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    TOp op;
    using T = typename TOp::result_type;
    
    statpascal::TVectorDataPtr out;
    initOutVector (out, sizeof (T), std::max (a->getElementCount (), b->getElementCount ()));
    T *result = &out->get<T> (0);
    
    const char *srcbegin1 = &a->get<char> (0),
               *srcbegin2 = &b->get<char> (0),
               *srcit1 = srcbegin1,
               *srcit2 = srcbegin2;
    const std::size_t src1size = statpascal::TStdType::scalarTypeSizes [tca],
                      src2size = statpascal::TStdType::scalarTypeSizes [tcb];
    const char *srcend1 = srcbegin1 + a->getElementCount () * src1size,
               *srcend2 = srcbegin2 + b->getElementCount () * src2size;
    const bool inc1 = a->getElementCount () > 1,
               inc2 = b->getElementCount () > 1;
    const bool intOp1 = tca != statpascal::TStdType::TScalarTypeCode::single && tca != statpascal::TStdType::TScalarTypeCode::real,
               intOp2 = tcb != statpascal::TStdType::TScalarTypeCode::single && tcb != statpascal::TStdType::TScalarTypeCode::real;
         
    double realop1 = 0.0, realop2 = 0.0;
    std::int64_t intop1 = 0, intop2 = 0;

    for (std::size_t i = 0; i < out->getElementCount (); ++i) {
        switch (tca) {
            case statpascal::TStdType::TScalarTypeCode::s64:
                intop1 = *reinterpret_cast<const std::int64_t *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::s32:
                intop1 = *reinterpret_cast<const std::int32_t *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::s16:
                intop1 = *reinterpret_cast<const std::int16_t *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::s8:
                intop1 = *reinterpret_cast<const std::int8_t *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::u8:
                intop1 = *reinterpret_cast<const std::uint8_t *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::u16:
                intop1 = *reinterpret_cast<const std::uint16_t *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::u32:
                intop1 = *reinterpret_cast<const std::uint32_t *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::single:
                realop1 = *reinterpret_cast<const float *> (srcit1);
                break;
            case statpascal::TStdType::TScalarTypeCode::real:
                realop1 = *reinterpret_cast<const double *> (srcit1);
                break;
            default:
                break;
        }
        switch (tcb) {
            case statpascal::TStdType::TScalarTypeCode::s64:
                intop2 = *reinterpret_cast<const std::int64_t *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::s32:
                intop2 = *reinterpret_cast<const std::int32_t *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::s16:
                intop2 = *reinterpret_cast<const std::int16_t *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::s8:
                intop2 = *reinterpret_cast<const std::int8_t *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::u8:
                intop2 = *reinterpret_cast<const std::uint8_t *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::u16:
                intop2 = *reinterpret_cast<const std::uint16_t *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::u32:
                intop2 = *reinterpret_cast<const std::uint32_t *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::single:
                realop2 = *reinterpret_cast<const float *> (srcit2);
                break;
            case statpascal::TStdType::TScalarTypeCode::real:
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
            if (srcit1 == srcend1)
                srcit1 = srcbegin1;
        }
        if (inc2) {
            srcit2 += src2size;
            if (srcit2 == srcend2)
                srcit2 = srcbegin2;
        }
    }
    
    return out;
}

template<template<typename T> class TOp> statpascal::TVectorDataPtr applyVectorOperation (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    using enum statpascal::TStdType::TScalarTypeCode;
    if (tca != real && tcb != real && tca != single && tcb != single)
        return applyVectorOperation<TOp<std::int64_t>> (a, b, tca, tcb);
    else
        return applyVectorOperation<TOp<double>> (a, b, tca, tcb);
}

} // anonymous namespace

extern "C" statpascal::TVectorDataPtr rt_vec_add (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::plus> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_sub (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::minus> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_or (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::bit_or<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_xor (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::bit_xor<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_mul (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::multiplies> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_div (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::divides> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_mod (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::modulus<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_and (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::bit_and<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_shl (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<bit_shl<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_shr (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<bit_shr<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_equal (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::equal_to> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_not_equal (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::not_equal_to> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_less_equal (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::less_equal> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_greater_equal (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::greater_equal> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_less (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::less> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_greater (statpascal::TVectorDataPtr a, statpascal::TVectorDataPtr b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::greater> (a, b, tca, tcb);
}

extern "C" statpascal::TVectorDataPtr rt_vec_conv (statpascal::TVectorDataPtr a, std::int64_t tcs, std::int64_t tcd) {
    using enum statpascal::TStdType::TScalarTypeCode;
    
    const std::size_t 
        srcSize = statpascal::TStdType::scalarTypeSizes [tcs],
        dstSize = statpascal::TStdType::scalarTypeSizes [tcd],
        n = a->getElementCount ();
    statpascal::TVectorDataPtr out;
    initOutVector (out, dstSize, n);
    const bool srcint = tcs != single && tcs != real;
    const char *srcit = &a->get<char> (0);
    char *dstit = &out->get<char> (0);
    
    double fVal = 0.0;
    std::int64_t iVal = 0;
    for (std::size_t i = 0; i < n; ++i) {
        switch (tcs) {
            case s64:
                iVal = *reinterpret_cast<const std::int64_t *> (srcit);
                break;
            case s32:
                iVal = *reinterpret_cast<const std::int32_t *> (srcit);
                break;
            case s16:
                iVal = *reinterpret_cast<const std::int16_t *> (srcit);
                break;
            case s8:
                iVal = *reinterpret_cast<const std::int8_t *> (srcit);
                break;
            case u8:
                iVal = *reinterpret_cast<const std::uint8_t *> (srcit);
                break;
            case u16:
                iVal = *reinterpret_cast<const std::uint16_t *> (srcit);
                break;
            case u32:
                iVal = *reinterpret_cast<const std::uint32_t *> (srcit);
                break;
            case single:
                fVal = *reinterpret_cast<const float *> (srcit);
                break;
            case real:
                fVal = *reinterpret_cast<const double *> (srcit);
                break;
            default:
                break;
        }
        srcit += srcSize;
        switch (tcd) {
            case s64:
                *reinterpret_cast<std::int64_t *> (dstit) = iVal;
                break;
            case s32:
                *reinterpret_cast<std::int32_t *> (dstit) = iVal;
                break;
            case s16:
                *reinterpret_cast<std::int16_t *> (dstit) = iVal;
                break;
            case s8:
                *reinterpret_cast<std::int8_t *> (dstit) = iVal;
                break;
            case u8:
                *reinterpret_cast<std::uint8_t *> (dstit) = iVal;
                break;
            case u16:
                *reinterpret_cast<std::uint16_t *> (dstit) = iVal;
                break;
            case u32:
                *reinterpret_cast<std::uint32_t *> (dstit) = iVal;
                break;
            case single:
                if (srcint)
                    *reinterpret_cast<float *> (dstit) = iVal;
                else
                    *reinterpret_cast<float *> (dstit) = fVal;
                break;
            case real:
                if (srcint)
                    *reinterpret_cast<double *> (dstit) = iVal;
                else
                    *reinterpret_cast<double *> (dstit) = fVal;
                break;
            default:
                break;
        }
        dstit += dstSize;        
    }    
    return out;    
}

// text files

namespace {

void openTextFile (TFileStruct *f, void (statpascal::TFileHandler::*op) (), statpascal::TRuntimeData *runtimeData) {
    f->binary = false;
    const std::string s = f->fn.get<std::string> ();
    statpascal::TTextFileBaseHandler *fh;
    if (s.empty ())
        fh = new statpascal::TStdioHandler ();
    else
        fh = new statpascal::TTextFileHandler (s);
        
    f->idx = runtimeData->registerFileHandler (fh);
    (fh->*op) ();
}

void openBinFile (TFileStruct *f, std::size_t recordSize, void (statpascal::TFileHandler::*op) (), statpascal::TRuntimeData *runtimeData) {
    f->binary = true;
    f->blksize = recordSize;
    const std::string s = f->fn.get<std::string> ();
    statpascal::TBinaryFileHandler *fh;
    fh = new statpascal::TBinaryFileHandler (s);
    f->idx = runtimeData->registerFileHandler (fh);
    (fh->*op) ();
}

}

extern "C" void rt_text_reset (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    openTextFile (f, &statpascal::TFileHandler::reset, runtimeData);
}

extern "C" void rt_text_rewrite (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    openTextFile (f, &statpascal::TFileHandler::rewrite, runtimeData);
}

extern "C" void rt_bin_reset (TFileStruct *f, std::int64_t recordSize, statpascal::TRuntimeData *runtimeData) {
    openBinFile (f, recordSize, &statpascal::TFileHandler::reset, runtimeData);
}

extern "C" void rt_bin_rewrite (TFileStruct *f, std::int64_t recordSize, statpascal::TRuntimeData *runtimeData) {
    openBinFile (f, recordSize, &statpascal::TFileHandler::rewrite, runtimeData);
}

extern "C" void rt_text_append (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    openTextFile (f, &statpascal::TFileHandler::append, runtimeData);
}

extern "C" void rt_flush (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    runtimeData->getFileHandler (f->idx).flush ();
}

extern "C" void rt_bin_seek (TFileStruct *f, std::int64_t pos, statpascal::TRuntimeData *runtimeData) {
    runtimeData->getBinaryFileHandler (f->idx).seek (pos * f->blksize);
}

extern "C" std::int64_t rt_bin_filesize (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    runtimeData->getFileHandler (f->idx).flush ();
    return std::filesystem::file_size (f->fn.get<std::string> ()) / f->blksize;
}

extern "C" std::int64_t rt_bin_filepos (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getBinaryFileHandler (f->idx).filepos () / f->blksize;
}

extern "C" void rt_close (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    runtimeData->closeFileHandler (f->idx);
    f->idx = -1;
}

extern "C" void rt_erase (TFileStruct *f) {
    const std::string fn = f->fn.get<std::string> ();
    if (!fn.empty ())
        // TODO: Runtime error if file is open !!!!
        std::filesystem::remove (fn);
}

extern "C" sp_bool rt_eof (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getFileHandler (f->idx).eof ();
}

extern "C" sp_bool rt_text_eoln (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getTextFileBaseHandler (f->idx).eoln ();
}

extern "C" sp_bool rt_file_exists (const char *fn) {
    return std::filesystem::exists (fn);
}

extern "C" sp_bool rt_file_delete (const char *fn) {
    return std::filesystem::remove (fn);
}

extern "C" void rt_bin_write (TFileStruct *f, void *buf, std::int64_t blocks, std::int64_t *blocksWritten, statpascal::TRuntimeData *runtimeData) {
    std::int64_t bytesWritten = runtimeData->getBinaryFileHandler (f->idx).write (buf, f->blksize * blocks);
    *blocksWritten = bytesWritten / f->blksize;
}

extern "C" void rt_bin_read (TFileStruct *f, void *buf, std::int64_t blocks, std::int64_t *blocksRead, statpascal::TRuntimeData *runtimeData) {
    std::int64_t bytesRead = runtimeData->getBinaryFileHandler (f->idx).read (buf, f->blksize * blocks);
    *blocksRead = bytesRead / f->blksize;
}

extern "C" void rt_write_lf (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    runtimeData->getTextFileBaseHandler (f->idx).getOutputStream () << std::endl;
}

extern "C" void rt_read_lf (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    runtimeData->getTextFileBaseHandler (f->idx).skipLine ();
}

extern "C" void rt_write_int (TFileStruct *f, std::int64_t n, std::int64_t length, std::int64_t prec, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    if (length >= 0)
        os << std::setw (length);
    os << n;
}

extern "C" void rt_write_char (TFileStruct *f, char ch, std::int64_t length, std::int64_t prec, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    if (length >= 0)
        os << std::setw (length);
    os << ch;
}

extern "C" void rt_write_string (TFileStruct *f, const char *s, std::int64_t length, std::int64_t prec, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    if (length >= 0)
        os << std::setw (length);
    os << std::string (s);
}

extern "C" void rt_write_bool (TFileStruct *f, bool b, std::int64_t length, std::int64_t prec, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    if (length >= 0)
        os << std::setw (length);
    os << (b ? "TRUE" : "FALSE");
}

extern "C" void rt_write_dbl (TFileStruct *f, double a, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    std::streamsize prec = os.precision ();
    if (length >= 0)
        os << std::setw (length);
    if (precision >= 0)
        os << std::fixed << std::setprecision (precision);
    os << a << std::setprecision (prec);
}

extern "C" std::int64_t rt_read_int (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getTextFileBaseHandler (f->idx).getInteger ();
}

extern "C" char rt_read_char (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getTextFileBaseHandler (f->idx).getChar ();
}

extern "C" statpascal::TAnyValue rt_read_string (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getTextFileBaseHandler (f->idx).getString ();
}

extern "C" double rt_read_dbl (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getTextFileBaseHandler (f->idx).getDouble ();
}

extern "C" void rt_write_vint (TFileStruct *f, statpascal::TVectorDataPtr v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    for (std::size_t i = 0, ei = v->getElementCount (); i < ei; ++i) {
        if (length >= 0)
            os << std::setw (length) << v->get<std::int64_t> (i);
        else
            os << v->get<std::int64_t> (i) << ' ';
    }
}

extern "C" void rt_write_vchar (TFileStruct *f, statpascal::TVectorDataPtr in, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    for (std::size_t i = 0, ei = in->getElementCount (); i < ei; ++i) {
        if (length >= 0)
            os << std::setw (length) << in->get<unsigned char> (i);
        else
            os << in->get<unsigned char> (i) << ' ';
    }
}

extern "C" void rt_write_vstring (TFileStruct *f, statpascal::TVectorDataPtr in, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    for (std::size_t i = 0, ei = in->getElementCount (); i < ei; ++i) {
        if (length >= 0)
            os << std::setw (length) << in->get<statpascal::TAnyValue> (i).get<std::string> ();
        else
            os << in->get<statpascal::TAnyValue> (i).get<std::string> () << ' ';
    }
}

extern "C" void rt_write_vbool (TFileStruct *f, statpascal::TVectorDataPtr in, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    for (std::size_t i = 0, ei = in->getElementCount (); i < ei; ++i) {
        if (length >= 0)
            os << std::setw (length);
        os << (in->get<bool> (i) ? "TRUE" : "FALSE");
        if (length < 0)
            os << ' ';
    }
}

extern "C" void rt_write_vdbl (TFileStruct *f, statpascal::TVectorDataPtr in, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    std::streamsize prec = os.precision ();
    if (precision >= 0)
        os << std::fixed << std::setprecision (precision);
    for (std::size_t i = 0, ei = in->getElementCount (); i < ei; ++i) {
        if (length >= 0)
            os << std::setw (length);
        os  << in->get<double> (i);
        if (length < 0)
            os << ' ';
    }
    os << std::setprecision (prec);
}

extern "C" void rt_str_int (std::int64_t n, std::int64_t length, std::int64_t precision, statpascal::TAnyValue *res) {
    // TODO: make faster !!!!
    std::stringstream ss;
    if (length >= 0)
        ss << std::setw (length);
    ss << n;
    *res = ss.str ();
}

extern "C" void rt_str_dbl (double a, std::int64_t length, std::int64_t precision, statpascal::TAnyValue *res) {
    std::stringstream ss;
    if (length >= 0)
        ss << std::setw (length);
    if (precision >= 0)
        ss << std::fixed << std::setprecision (precision);
    ss << a;
    *res = ss.str ();
}

// set handling

extern "C" TSet rt_get_set_const (std::uint64_t index, statpascal::TRuntimeData *runtimeData) {
    TSet result;
    memcpy (result.data, runtimeData->getData (index), sizeof (result.data));
    return result;
}

extern "C" TSet rt_set_add_val (std::uint64_t n, TSet *s) {
    TSet result (*s);
    if (n < 64 * statpascal::TConfig::setwords)
        result.data [n / 64] |= (1LL << (n % 64));
    return result;
}
    
extern "C" TSet rt_set_add_range (std::int64_t a, std::int64_t b, TSet *s) {
    TSet result (*s);
    const std::uint64_t n1 = std::min<std::uint64_t> (b, 64 * statpascal::TConfig::setwords);
    for (std::uint64_t n = a; n <= n1; ++n)
        result.data [n / 64] |= (1LL << (n % 64));
    return result;
}

extern "C" sp_bool rt_in_set (std::uint64_t v, TSet *s) {
    return  v < 64 * statpascal::TConfig::setwords && !!(s->data [v / 64] & (1LL << (v % 64)));
}

extern "C" TSet rt_set_union (TSet *s, TSet *t) {
    TSet result (*s);
    for (std::size_t i = 0; i < statpascal::TConfig::setwords; ++i)
        result.data [i] |= t->data [i];
    return result;
}

extern "C" TSet rt_set_intersection (TSet *s, TSet *t) {
    TSet result (*s);
    for (std::size_t i = 0; i < statpascal::TConfig::setwords; ++i)
        result.data [i] &= t->data [i];
    return result;
}

extern "C" TSet rt_set_diff (TSet *s, TSet *t) {
    TSet result (*s);
    for (std::size_t i = 0; i < statpascal::TConfig::setwords; ++i)
        result.data [i] &= ~t->data [i];
    return result;
}

namespace {

bool set_equal (const TSet *s, const TSet *t) {
    return !std::memcmp (s->data, t->data, statpascal::TConfig::setwords * sizeof (std::int64_t));
}

bool set_super (TSet *s, TSet *t) {
    for (std::size_t i = 0; i < statpascal::TConfig::setwords; ++i)
        if ((s->data [i] & t->data [i]) != t->data [i])
            return false;
    return true;
}

}

extern "C" sp_bool rt_set_equal (TSet *s, TSet *t) {
    return set_equal (s, t);
}

extern "C" sp_bool rt_set_not_equal (TSet *s, TSet *t) {
    return !set_equal (s, t);
}

extern "C" sp_bool rt_set_sub (TSet *s, TSet *t) {
    return set_super (t, s);
}

extern "C" sp_bool rt_set_super (TSet *s, TSet *t) {
    return set_super (s, t);
}

extern "C" sp_bool rt_set_sub_not_equal (TSet *s, TSet *t) {
    return set_super (t, s) && !set_equal (t, s);
}

extern "C" sp_bool rt_set_super_not_equal (TSet *s, TSet *t) {
    return set_super (s, t) && !set_equal (s, t);
}



// threading

extern "C" void rt_thread_create (void *(*p) (void *), void *arg, std::int64_t *threadId) {
    pthread_t tid;
    pthread_create (&tid, nullptr, p, arg);
    *threadId = tid;
}

extern "C" void rt_thread_join (std::int64_t threadId, std::int64_t timeout) {
    pthread_t tid = threadId;
    pthread_join (tid, nullptr);
}

extern "C" void rt_mutex_init (void **p) {
    *p = new std::mutex;
}

extern "C" void rt_mutex_lock (void *p) {
    reinterpret_cast<std::mutex *> (p)->lock ();
}

extern "C" void rt_mutex_unlock (void *p) {
    reinterpret_cast<std::mutex *> (p)->unlock ();
}

extern "C" void rt_mutex_destroy (void *p) {
    delete reinterpret_cast<std::mutex *> (p);
}

using TSem = std::counting_semaphore<std::numeric_limits<std::int32_t>::max ()>;

extern "C" void rt_sem_init (void **p, std::int32_t init) {
    *p = new TSem (init);
}

extern "C" void rt_sem_acquire (void *p) {
    reinterpret_cast<TSem *> (p)->acquire ();
}

extern "C" void rt_sem_acquire_timeout (void *p, std::uint32_t timeout, bool *result) {
    *result = reinterpret_cast<TSem *> (p)->try_acquire_for (std::chrono::milliseconds (timeout));
}

extern "C" void rt_sem_release (void *p) {
    reinterpret_cast<TSem *> (p)->release ();
}

extern "C" void rt_sem_destroy (void *p) {
    delete reinterpret_cast<TSem *> (p);
}

