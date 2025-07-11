/** \file constant.hpp
*/

#pragma once

#include <string>
#include <array>

#include "syntaxtreenode.hpp"
#include "config.hpp"

namespace statpascal {

class TType; class TPointerType; class TArrayType; class TRecordType; class TSetType; class TRoutineValue;

class TConstant: public TSyntaxTreeNode {
public:
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    virtual TType *getType () const = 0;
};

class TSimpleConstant: public TConstant {
using inherited = TConstant;
public:
    explicit TSimpleConstant (TType *type = nullptr);
    TSimpleConstant (std::int64_t val, TType *);
    TSimpleConstant (double, TType *);
    TSimpleConstant (const std::string &, TType *);
    TSimpleConstant (TRoutineValue *);
    
    TSimpleConstant (const TSimpleConstant &) = default;
    TSimpleConstant (TSimpleConstant &&) = default;
    
    TSimpleConstant &operator = (const TSimpleConstant &) = default;
    TSimpleConstant &operator = (TSimpleConstant &&) = default;
    
    ~TSimpleConstant () = default;
    
    void setType (TType *);
    virtual TType *getType () const override;
    
//    void setChar (unsigned char, TType *);
    unsigned char getChar () const;

//    void setInteger (std::int64_t, TType *);
    std::int64_t getInteger () const;
    
//    void setDouble (double, TType *);
    double getDouble () const;
    
//    void setString (const std::string &, TType *);
    std::string getString () const;
    
    TRoutineValue *getRoutineValue () const;
    
    void addSetValue (std::uint64_t);
    void addSetRange (std::uint64_t, std::uint64_t);
    const std::array<std::uint64_t, TConfig::setwords> &getSetValues () const;
    
    bool isValid () const;
    bool hasType (TType *intType, TType *realType) const;
    
private:
    TType *type;
    std::int64_t integerValue;
    double doubleValue;
    std::string stringValue;
    TRoutineValue *routineValue;
    std::array<std::uint64_t, TConfig::setwords> values;
};

class TArrayConstant: public TConstant {
using inherited = TConstant;
public:
    TArrayConstant (TArrayType *type);
    
    void addValue (const TConstant *c);
    const std::vector<const TConstant *> &getValues () const;
    
    virtual TType *getType () const override;
    
private:
    TArrayType *type;
    std::vector<const TConstant *> values;
};

class TRecordConstant: public TConstant {
using inherited = TConstant;
public:
    TRecordConstant (TRecordType *type);
    
    void addValue (const std::string &component, const TConstant *c);
    virtual TType *getType () const override;
    
    struct TRecordValue {
        const std::string component;
        const TConstant *c;
    };
    const std::vector<TRecordValue> &getValues () const;
    
private:
    TRecordType *type;
    std::vector<TRecordValue> values;
};

}
