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
    const static std::string empty;
    if (p.hasValue ())
        return p.get<std::string> ();
    else
        return empty;
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

extern "C" statpascal::TAnyValue rt_str_copy (statpascal::TAnyValue a, std::int64_t pos, std::int64_t length) {
    const std::string &t = getString (a);
    if (pos <= 0)
        pos = 1;
    if (static_cast<std::size_t> (pos) <= t.length () && length > 0) 
        return t.substr (pos - 1, length);
    else
        return std::string ();
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
    return std::move (s);
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

extern "C" std::int64_t rt_val_int (statpascal::TAnyValue a, std::uint16_t *code) {
    char *endptr;
    const char *s = getString (a).c_str ();
    std::int64_t result = std::strtoll (s, &endptr, 10);
    if (*endptr) {
        result = 0;
        *code = endptr - s + 1;
    } else
        *code = 0;
    return result;
}

extern "C" double rt_val_dbl (statpascal::TAnyValue a, std::uint16_t *code) {
    char *endptr;
    const char *s = getString (a).c_str ();
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

template<typename TEl, typename TRes> TRes vecsum (statpascal::TAnyValue &in) {
    if (in.hasValue ())  {
        TEl *p = &(in.get<statpascal::TVectorData> ().template get<TEl> (0));
        return std::accumulate (p, p + in.get<statpascal::TVectorData> ().getElementCount (), TRes ());
    } else
        return TRes ();
}

template<typename T> statpascal::TAnyValue veccumsum (const statpascal::TAnyValue &a) {
    const statpascal::TVectorData &in = a.get<statpascal::TVectorData> ();
    statpascal::TVectorData out (in.getElementSize (), in.getElementCount ());
    
    const T *x = &(in.template get<T> (0));
    T *y = &(out.template get<T> (0));
    std::partial_sum (x, x + in.getElementCount (), y);
    
    return std::move (out);
}

template<typename T> statpascal::TAnyValue vecsort (statpascal::TAnyValue &a) {
    const statpascal::TVectorData &in = a.get<statpascal::TVectorData> ();
    const std::size_t size = in.getElementSize (), count = in.getElementCount ();
    statpascal::TVectorData out (size, count);
    
    const T *x = &(in.template get<T> (0));
    T *y = &(out.template get<T> (0));
    std::memcpy (y, x, size * count);
    std::sort (y, y + count);
    
    return std::move (out);
}

statpascal::TAnyValue vecfunc (std::function<double (double)> fn, const statpascal::TAnyValue &a) {
    const statpascal::TVectorData &in = a.get<statpascal::TVectorData> ();
    statpascal::TVectorData out (in.getElementSize (), in.getElementCount ());
    const double *x = &(in.get<double> (0));
    double *y = &(out.get<double> (0));
    for (std::size_t i = 0, ei = in.getElementCount (); i < ei; ++i)
        y [i] = fn (x [i]);
    return std::move (out);
}

} // namespace

extern "C" statpascal::TAnyValue rt_vdbl_sqr (statpascal::TAnyValue in) {
    return vecfunc ([] (double x) {return x * x; }, in);
}

extern "C" statpascal::TAnyValue rt_vdbl_sqrt (statpascal::TAnyValue in) {
    return vecfunc ([] (double x) {return std::sqrt (x); }, in);
}

extern "C" statpascal::TAnyValue rt_vdbl_sin (statpascal::TAnyValue in) {
    return vecfunc ([] (double x) {return std::sin (x); }, in);
}

extern "C" statpascal::TAnyValue rt_vdbl_cos (statpascal::TAnyValue in) {
    return vecfunc ([] (double x) {return std::cos (x); }, in);
}

extern "C" statpascal::TAnyValue rt_vdbl_log (statpascal::TAnyValue in) {
    return vecfunc ([] (double x) {return std::log (x); }, in);
}

extern "C" statpascal::TAnyValue rt_vdbl_pow (statpascal::TAnyValue in, double e) {
    return vecfunc ([e] (double x) {return std::pow (x, e); }, in);
}

extern "C" void *rt_vec_index_int (statpascal::TAnyValue in, std::int64_t index) {
    // TODO: range check, COW?
    return &in.get<statpascal::TVectorData> ().get<char> (index - 1);
}

extern "C" statpascal::TAnyValue rt_vec_index_vint (statpascal::TAnyValue a, statpascal::TAnyValue index) {
    // TODO: range check
    std::int64_t ival = 0;
    const statpascal::TVectorData 
        &src = a.get<statpascal::TVectorData> (),
        &ind = index.get<statpascal::TVectorData> ();
    
    statpascal::TVectorData out (src.getElementSize (), ind.getElementCount (), src.getElementAnyManager (), false);
    for (std::size_t i = 0; i < ind.getElementCount (); ++i) {
        memcpy (&ival, &ind.get<char> (i), ind.getElementSize ());
        out.setElement (i, &src.get<unsigned char> (ival - 1));
    }
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_vec_index_vbool (statpascal::TAnyValue a, statpascal::TAnyValue index) {
    // TODO: range check
    const statpascal::TVectorData 
        &src = a.get<statpascal::TVectorData> (),
        &ind = index.get<statpascal::TVectorData> ();
    
    // count number of true 8 bytes in parallel
    const std::size_t indexCount = ind.getElementCount ();
    const bool *const indexData = &ind.get<bool> (0);
    const std::uint64_t *const indexInt = &ind.get<std::uint64_t> (0);
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
        
    statpascal::TVectorData out (src.getElementSize (), count, src.getElementAnyManager (), false);
    if (count)
        for (std::size_t i = 0, dst = 0; dst < count; ++i) 
            if (indexData [i])
                out.setElement (dst++, &src.get<unsigned char> (i));
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_intvec (std::int64_t a, std::int64_t b) {
    statpascal::TVectorData out (sizeof (std::int64_t), std::max<std::int64_t> (0, b - a + 1));
    if (b >= a) {
        std::int64_t *p = &out.get<std::int64_t> (0);
        std::iota (p, p + (b - a + 1), a);
    }
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_realvec (double a, double b, double step) {
    if ((step > 0.0 && b >= a) || (step < 0.0 && b <= a)) {
        const std::size_t count = std::floor (1 + (b - a) / step + 0.5);
        statpascal::TVectorData out (sizeof (double), count);
        double *p = &out.get<double> (0);
        for (std::size_t i = 0; i < count; ++i)
            *p++ = static_cast<double> (count - 1 - i) / (count - 1) * a + static_cast<double> (i) / (count - 1) * b;
        return std::move (out);
    } else
        return statpascal::TVectorData (sizeof (double), 0);
}

extern "C" statpascal::TAnyValue rt_makevec_int (std::int64_t size, std::int64_t val) {
    statpascal::TVectorData out (size, 1);
    out.setElement (0, &val);
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_makevec_dbl (std::int64_t size, double val) {
    statpascal::TVectorData out (size, 1);
    if (size == sizeof (double))
        out.setElement (0, &val);
    else {
        float val1 = val;
        out.setElement (0, &val1);
    }
    return std::move (out);
}

// TODO: unify!

extern "C" statpascal::TAnyValue rt_makevec_str (std::int64_t anyManagerIndex, statpascal::TRuntimeData *runtimeData, statpascal::TAnyValue a) {
    statpascal::TVectorData out (sizeof (void *), 1, runtimeData->getAnyManager (anyManagerIndex));
    out.setElement (0, &a);
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_makevec_vec (std::int64_t anyManagerIndex, statpascal::TRuntimeData *runtimeData, statpascal::TAnyValue a) {
    statpascal::TVectorData out (sizeof (void *), 1, runtimeData->getAnyManager (anyManagerIndex));
    out.setElement (0, &a);
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_combinevec_4 (statpascal::TAnyValue a, statpascal::TAnyValue b, statpascal::TAnyValue c, statpascal::TAnyValue d) {
    const std::size_t n = 4;
    std::array<statpascal::TAnyValue, n> in {a, b, c, d};
    std::int64_t count = 0, elsize = 0;
    statpascal::TAnyManager *anyManager = nullptr;
    for (std::size_t i = 0; i < n; ++i)
        if (in [i].hasValue ()) {
            const statpascal::TVectorData &vectorData = in [i].get<statpascal::TVectorData> ();
            count += vectorData.getElementCount ();
            elsize = vectorData.getElementSize ();
            anyManager = vectorData.getElementAnyManager ();
        }
    statpascal::TVectorData out (elsize, count, anyManager, false);
    std::int64_t index = 0;
    for (std::size_t i = 0; i < n; ++i)
        if (in [i].hasValue ()) {
            const statpascal::TVectorData &vectorData = in [i].get<statpascal::TVectorData> ();
            for (std::size_t j = 0; j < vectorData.getElementCount (); ++j)
                out.setElement (index++, vectorData.getElement (j));
        }
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_combinevec_3 (statpascal::TAnyValue a, statpascal::TAnyValue b, statpascal::TAnyValue c) {
    return rt_combinevec_4 (a, b, c, statpascal::TAnyValue ());
}

extern "C" statpascal::TAnyValue rt_combinevec_2 (statpascal::TAnyValue a, statpascal::TAnyValue b) {
    return rt_combinevec_4 (a, b,  statpascal::TAnyValue (),  statpascal::TAnyValue ());
}

extern "C" std::int64_t rt_sizevec (statpascal::TAnyValue a) {
    return a.get<statpascal::TVectorData> ().getElementCount ();
}

extern "C" void rt_resizevec (std::int64_t anyManagerIndex, statpascal::TRuntimeData *runtimeData, std::int64_t elsize, statpascal::TAnyValue &a, std::int64_t n) {
    statpascal::TVectorData out (elsize, n, runtimeData->getAnyManager (anyManagerIndex));
    if (a.hasValue ()) {
        const std::size_t copyCount = std::min<std::size_t> (a.get<statpascal::TVectorData> ().getElementCount (), n);
        if (out.getElementAnyManager ())
            for (std::size_t j = 0; j < copyCount; ++j)
                out.setElement (j, a.get<statpascal::TVectorData> ().getElement (j));
        else
            std::memcpy (out.getElement (0), a.get<statpascal::TVectorData> ().getElement (0), copyCount * elsize);
    }
    a = out;
}

extern "C" statpascal::TAnyValue rt_revvec (statpascal::TAnyValue a) {
    statpascal::TVectorData &vectorData = a.get<statpascal::TVectorData> ();
    const std::size_t n = vectorData.getElementCount ();
    statpascal::TVectorData out (vectorData.getElementSize (), n, vectorData.getElementAnyManager ());
    for (std::size_t i = 0; i < n; ++i)
        out.setElement (i, vectorData.getElement (n - 1 - i));
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_vint_randomperm (std::int64_t n) {
    statpascal::TVectorData out (sizeof (std::int64_t), std::max<std::int64_t> (n, 0));
    if (n >= 1) {
        std::int64_t *p = &out.get<std::int64_t> (0);
        std::iota (p, p + n, 1);
        for (std::int64_t i = n - 1; i > 0; --i)
            std::swap (p [i], p [statpascal::TRNG::val (0, i)]);
    }
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_vdbl_random (std::int64_t n) {
    statpascal::TVectorData out (sizeof (double), std::max<std::int64_t> (n, 0));
    if (n >= 1) {
        double *p = &out.get<double> (0);
        for (std::int64_t i = 0; i <n; ++i)
            p [i] = statpascal::TRNG::val ();
    }
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_vint_random (std::int64_t m, std::int64_t n) {
    statpascal::TVectorData out (sizeof (std::int64_t), std::max<std::int64_t> (n, 0));
    if (n >= 1) {
        std::int64_t *p = &out.get<std::int64_t> (0);
        for (std::int64_t i = 0; i <n; ++i)
            p [i] = statpascal::TRNG::val (0, m);
    }
    return std::move (out);
}

extern "C" statpascal::TAnyValue rt_vint_sort (statpascal::TAnyValue a) {
    return vecsort<std::int64_t> (a);
}

extern "C"  statpascal::TAnyValue rt_vdbl_sort (statpascal::TAnyValue a) {
    return vecsort<double> (a);
}

extern "C" std::int64_t rt_vint_sum (statpascal::TAnyValue in) {
    return vecsum<std::int64_t, std::int64_t> (in);
}

extern "C" double rt_vdbl_sum (statpascal::TAnyValue in) {
    return vecsum<double, double> (in);
}

extern "C" std::int64_t rt_vbool_count (statpascal::TAnyValue in) {
    return vecsum<bool, std::int64_t> (in);
}

extern "C" statpascal::TAnyValue rt_vint_cumsum (statpascal::TAnyValue a) {
    return veccumsum<std::int64_t> (a);
}

extern "C" statpascal::TAnyValue rt_vdbl_cumsum (statpascal::TAnyValue a) {
    return veccumsum<double> (a);
}

namespace {

template<class TOp> statpascal::TAnyValue applyVectorOperation (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    TOp op;
    using T = typename TOp::result_type;
    
    const statpascal::TVectorData &av = a.get<statpascal::TVectorData> (), &bv = b.get<statpascal::TVectorData> ();
    
    statpascal::TVectorData out (sizeof (T), std::max (av.getElementCount (), bv.getElementCount ()));
    T *result = &out.get<T> (0);
    
    const char *srcbegin1 = &av.get<char> (0),
               *srcbegin2 = &bv.get<char> (0),
               *srcit1 = srcbegin1,
               *srcit2 = srcbegin2;
    const std::size_t src1size = statpascal::TStdType::scalarTypeSizes [tca],
                      src2size = statpascal::TStdType::scalarTypeSizes [tcb];
    const char *srcend1 = srcbegin1 + av.getElementCount () * src1size,
               *srcend2 = srcbegin2 + bv.getElementCount () * src2size;
    const bool inc1 = av.getElementCount () > 1,
               inc2 = bv.getElementCount () > 1;
    const bool intOp1 = tca != statpascal::TStdType::TScalarTypeCode::single && tca != statpascal::TStdType::TScalarTypeCode::real,
               intOp2 = tcb != statpascal::TStdType::TScalarTypeCode::single && tcb != statpascal::TStdType::TScalarTypeCode::real;
         
    double realop1 = 0.0, realop2 = 0.0;
    std::int64_t intop1 = 0, intop2 = 0;

    for (std::size_t i = 0; i < out.getElementCount (); ++i) {
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
    
    return std::move (out);
}

template<template<typename T> class TOp> statpascal::TAnyValue applyVectorOperation (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    using enum statpascal::TStdType::TScalarTypeCode;
    if (tca != real && tcb != real && tca != single && tcb != single)
        return applyVectorOperation<TOp<std::int64_t>> (a, b, tca, tcb);
    else
        return applyVectorOperation<TOp<double>> (a, b, tca, tcb);
}

} // anonymous namespace

extern "C" statpascal::TAnyValue rt_vec_add (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::plus> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_sub (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::minus> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_or (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::bit_or<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_xor (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::bit_xor<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_mul (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::multiplies> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_div (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::divides> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_mod (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::modulus<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_and (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::bit_and<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_shl (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<bit_shl<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_shr (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<bit_shr<std::int64_t>> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_equal (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::equal_to> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_not_equal (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::not_equal_to> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_less_equal (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::less_equal> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_greater_equal (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::greater_equal> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_less (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::less> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_greater (statpascal::TAnyValue a, statpascal::TAnyValue b, std::int64_t tca, std::int64_t tcb) {
    return applyVectorOperation<std::greater> (a, b, tca, tcb);
}

extern "C" statpascal::TAnyValue rt_vec_conv (statpascal::TAnyValue a, std::int64_t tcs, std::int64_t tcd) {
    using enum statpascal::TStdType::TScalarTypeCode;
    
    const std::size_t 
        srcSize = statpascal::TStdType::scalarTypeSizes [tcs],
        dstSize = statpascal::TStdType::scalarTypeSizes [tcd],
        n = a.get<statpascal::TVectorData> ().getElementCount ();
    statpascal::TVectorData out (dstSize, n);
    const bool srcint = tcs != single && tcs != real;
    const char *srcit = &a.get<statpascal::TVectorData> ().get<char> (0);
    char *dstit = &out.get<char> (0);
    
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
    return std::move (out);
}

// text files

namespace {

void openTextFile (TFileStruct *f, void (statpascal::TFileHandler::*op) (), statpascal::TRuntimeData *runtimeData) {
    f->binary = false;
    const std::string s = getString (f->fn);
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
    const std::string s = getString (f->fn);
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
    return std::filesystem::file_size (getString (f->fn)) / f->blksize;
}

extern "C" std::int64_t rt_bin_filepos (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    return runtimeData->getBinaryFileHandler (f->idx).filepos () / f->blksize;
}

extern "C" void rt_close (TFileStruct *f, statpascal::TRuntimeData *runtimeData) {
    runtimeData->closeFileHandler (f->idx);
    f->idx = -1;
}

extern "C" void rt_erase (TFileStruct *f) {
    const std::string fn = getString (f->fn);
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

extern "C" sp_bool rt_file_exists (statpascal::TAnyValue a) {
    return std::filesystem::exists (getString (a));
}

extern "C" sp_bool rt_file_delete (statpascal::TAnyValue a) {
    return std::filesystem::remove (getString (a));
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

namespace {

enum class mybool: std::uint8_t { FALSE, TRUE };

std::ostream &operator << (std::ostream &os, mybool f) {
    os << (f == mybool::TRUE ? "TRUE" : "FALSE");
    return os;
}

template<typename T> void rt_write_val (TFileStruct *f, T v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    std::streamsize prec = os.precision ();
    if (length >= 0)
        os << std::setw (length);
    if (precision >= 0)
        os << std::fixed << std::setprecision (precision);
    os << v << std::setprecision (prec);
}

template<typename T> void rt_write_vector (TFileStruct *f, statpascal::TAnyValue v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    std::ostream &os = runtimeData->getTextFileBaseHandler (f->idx).getOutputStream ();
    std::streamsize prec = os.precision ();
    if (precision >= 0)
        os << std::fixed << std::setprecision (precision);
    for (std::size_t i = 0, ei = v.get<statpascal::TVectorData> ().getElementCount (); i < ei; ++i) {
        if (length >= 0)
            os << std::setw (length);
        os << v.get<statpascal::TVectorData> ().get<T> (i);
        if (length < 0)
            os << ' ';
    }
    os << std::setprecision (prec);
}

}

extern "C" void rt_write_int (TFileStruct *f, std::int64_t n, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_val<std::int64_t> (f, n, length, precision, runtimeData);
}

extern "C" void rt_write_char (TFileStruct *f, char ch, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_val<unsigned char> (f, ch, length, precision, runtimeData);
}

extern "C" void rt_write_string (TFileStruct *f, statpascal::TAnyValue s, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_val<std::string> (f, getString (s), length, precision, runtimeData);
}

extern "C" void rt_write_bool (TFileStruct *f, bool b, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_val<mybool> (f, static_cast<mybool> (b), length, precision, runtimeData);
}

extern "C" void rt_write_dbl (TFileStruct *f, double a, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_val<double> (f, a, length, precision, runtimeData);
}

extern "C" void rt_write_vint (TFileStruct *f, statpascal::TAnyValue v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_vector<std::int64_t> (f, v, length, precision, runtimeData);
}

extern "C" void rt_write_vchar (TFileStruct *f, statpascal::TAnyValue v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_vector<unsigned char> (f, v, length, precision, runtimeData);
}

extern "C" void rt_write_vstring (TFileStruct *f, statpascal::TAnyValue v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_vector<std::string> (f, v, length, precision, runtimeData);
}

extern "C" void rt_write_vbool (TFileStruct *f, statpascal::TAnyValue v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_vector<mybool> (f, v, length, precision, runtimeData);
}

extern "C" void rt_write_vdbl (TFileStruct *f, statpascal::TAnyValue v, std::int64_t length, std::int64_t precision, statpascal::TRuntimeData *runtimeData) {
    rt_write_vector<double> (f, v, length, precision, runtimeData);
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

