/** \file anyvalue.hpp
*/

#pragma once

#include <algorithm>
#include <iostream>

namespace statpascal {

class TAnyValue {
public:
    TAnyValue ();
    TAnyValue (const TAnyValue &);
    TAnyValue (TAnyValue &&);
    TAnyValue (void *p);
    template<typename T, 
             typename U = typename std::remove_cv<typename std::remove_reference<T>::type>::type,
             typename V = typename std::enable_if<!std::is_same<TAnyValue, U>::value>::type> TAnyValue (T &&);
             
    TAnyValue &operator = (TAnyValue);
    
    ~TAnyValue ();
    
    TAnyValue &swap (TAnyValue &);
    
    /** creates copy if value is shared. */
    void copyOnWrite ();
    
    /** returns true if a value has been stored. */
    bool hasValue () const;
    
    template<typename T> T &get ();
    template<typename T> const T &get () const;
    
private:
    class TValue {
    public:
        TValue (): refcount (1) {};
        virtual ~TValue () = default;
        virtual TValue *clone () = 0;
        
        std::size_t refcount;
    };
    
    template<typename T> class TConcreteValue: public TValue {
    public:
        TConcreteValue (const T &);
        TConcreteValue (T &&);
        virtual ~TConcreteValue () = default;
        
        virtual TValue *clone () override;

        T concreteValue;
    };
    
    TValue *value;
};

template<typename T> inline TAnyValue::TConcreteValue<T>::TConcreteValue (const T &val): concreteValue (val) {
}

template<typename T> inline TAnyValue::TConcreteValue<T>::TConcreteValue (T &&val): concreteValue (std::move (val)) {
}

template<typename T> inline TAnyValue::TValue *TAnyValue::TConcreteValue<T>::clone () {
    return new TConcreteValue (concreteValue);
}

inline TAnyValue::TAnyValue (): 
  value (nullptr) {
}

inline TAnyValue::TAnyValue (void *p):
  value (static_cast<TValue *> (p)) {
      if (value)
          ++value->refcount;
}

inline TAnyValue::~TAnyValue () {
    if (value && !--value->refcount)
        delete value;
}

inline TAnyValue::TAnyValue (const TAnyValue &other):
  value (other.value) {
    if (value)
      ++value->refcount;

}

inline TAnyValue::TAnyValue (TAnyValue &&other):
  value (other.value) {
    other.value = nullptr;
}

template<typename T, typename U, typename V> inline TAnyValue::TAnyValue (T &&val):
  value (new TConcreteValue<U> (std::forward<T> (val))) {
}

inline TAnyValue &TAnyValue::operator = (TAnyValue other) {
    return swap (other);
}

inline TAnyValue &TAnyValue::swap (TAnyValue &other) {
    std::swap (value, other.value);
    return *this;
}

template<typename T> inline T &TAnyValue::get () {
    return static_cast<TConcreteValue<T> *> (value)->concreteValue;
}

template<typename T> inline const T &TAnyValue::get () const {
    return static_cast<const TConcreteValue<T> *> (value)->concreteValue;
}

inline void TAnyValue::copyOnWrite () {
    if (value && value->refcount > 1) {
        --value->refcount;
        value = value->clone ();
    }
}

inline bool TAnyValue::hasValue () const {
    return !!value;
}

}

#ifdef TEST

// Test

#include <iostream>

using namespace statpascal;

class TTestClass {
public:
    TTestClass (int n): n (n) { std::cout << "Konstruktor: " << n << std::endl; }
    ~TTestClass () { std::cout << "Destruktor: " << n << std::endl; }
    TTestClass (const TTestClass &other): n (other.n) { std::cout << "Copy Constructor: " << n << std::endl; }
    TTestClass (TTestClass &&other): n (other.n)  { std::cout << "Move Contructor: " << n << std::endl; other.n = 0; }
    
    int getn () { return n;}
private:
    int n;
};

int main () {
//    TAnyValue any (TTestClass (5));
    
    TTestClass t (7);
    TAnyValue any (TTestClass (6));
    std::cout << "Size is: " << sizeof (any) << std::endl << " " << t.getn () << std::endl;
    TAnyValue any1 (std::move (any));
    TAnyValue any2 = any1;
    TAnyValue any3 = any1;
    any1 = any1;
    std::cout << "Any is: " << any1.get<TTestClass> ().getn () << std::endl;
}

#endif
