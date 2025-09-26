/** \file datatypes.hpp
*/

#pragma once

#include <stack>
#include <unordered_map>

#include "symboltable.hpp"
#include "mempoolfactory.hpp"

namespace statpascal {

class TType: public TMemoryPoolObject {
public:
    TType ();
    virtual ~TType () = default;
    
    virtual std::string getName () const = 0;
    virtual std::size_t getSize () const = 0;
    virtual bool isSerializable () const;
    
    void setAlignment (std::size_t);
    std::size_t getAlignment () const;

    virtual bool isVoid () const;    
    virtual bool isReal () const;
    virtual bool isSingle () const;
    virtual bool isString () const;
    virtual bool isShortString () const;
    virtual bool isEnumerated () const;
    virtual bool isSubrange () const;
    virtual bool isArray () const;
    virtual bool isRecord() const;
    virtual bool isReference () const;
    virtual bool isPointer () const;
    virtual bool isRoutine () const;
    virtual bool isSet () const;
    virtual bool isFile () const;
    virtual bool isVector () const;
    
    virtual TType *getBaseType () const;
    
private:
    std::size_t alignment;    
};

class TVoidType: public TType {
typedef TType inherited;
public:
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    
    virtual bool isVoid () const override;
};

class TUnresOverloadType: public TType {
typedef TType inherited;
public:
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
};

class TRealType: public TType {
typedef TType inherited;
public:
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    
    virtual bool isReal () const override;
};

class TSingleType: public TType {
typedef TType inherited;
public:
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    
    virtual bool isSingle () const override;
};

class TStringType: public TType {
typedef TType inherited;
public:
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    
    virtual bool isString () const override;
};

class TEnumeratedType: public TType {
typedef TType inherited;
public:
    TEnumeratedType (const std::string &name, std::int64_t minVal, std::int64_t maxVal);
    
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    
    virtual bool isEnumerated () const override;
    
    std::int64_t getMinVal () const;
    std::int64_t getMaxVal () const;
    bool isSigned () const;
    
private:
    template<typename T> bool checkLimits ();

    const std::string name;
    const std::int64_t minVal, maxVal;
    std::size_t size;
};

class TSubrangeType: public TEnumeratedType {
typedef TEnumeratedType inherited;
public:
    TSubrangeType (const std::string &name, TType *baseType, std::int64_t minval, std::int64_t maxval);
    
    virtual std::string getName () const override;
    virtual bool isSubrange () const override;
    
    virtual TType *getBaseType () const override;

private:
    TType *baseType;
};

class TArrayType: public TType {
typedef TType inherited;
public:
    TArrayType (TType *baseType, TEnumeratedType *indexType);
    
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    virtual bool isArray () const override;
    
    virtual TType *getBaseType () const override;
    TEnumeratedType *getIndexType () const;
    
private:
    TType *baseType;
    TEnumeratedType *indexType;
};

// TODO: common base type for record and file !!!!

class TRecordType: public TType {
typedef TType inherited;
public:
    TRecordType (TSymbolList *components);

    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    virtual bool isRecord () const override;
    
    TSymbolList::TAddSymbolResult addComponent (const std::string &name, TType *type);
    void enterVariant (TSymbolList *components);
    void leaveVariant ();
    
    TSymbol *searchComponent (const std::string &name) const;
    
    struct TRecordFields {
        TSymbolList *components;
        std::vector<TRecordFields> variants;
    };
    
    TRecordFields &getRecordFields ();
    const TRecordFields &getRecordFields () const;
    void setSize (std::size_t);
    
    using TComponentMap = std::unordered_map<std::string, TSymbol *>;
    TComponentMap componentMap;
    
private:
    std::size_t size;
    TRecordFields recordFields;
    std::stack<TRecordFields *> activeVariants;
};

class TFileType: public TRecordType {
typedef TRecordType inherited;
public:
    TFileType (TType *baseType, TSymbolList *components);
    
    virtual std::string getName () const override;
    virtual bool isSerializable () const override;
    virtual bool isFile () const override;
    
    void setBaseType (TType *);
    virtual TType *getBaseType () const override;
    
private:
    TType *baseType;    
};

//

class TPointerType: public TType {
typedef TType inherited;
public:
    explicit TPointerType (TType *baseType);
    explicit TPointerType (const std::string &baseName);
    
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    virtual bool isPointer () const override;
    
    std::string getBaseName () const;
    void setBaseType (TType *baseType);
    virtual TType *getBaseType () const override;
    
private:
    TType *baseType;
    const std::string baseName;
};

class TSetType: public TType {
using inherited = TType;
public:
    TSetType (TEnumeratedType *baseType);
    
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    virtual bool isSet () const override;

    virtual TEnumeratedType *getBaseType () const override;
    
private:
    TEnumeratedType *baseType;
};

class TVectorType: public TType {
typedef TType inherited;
public:
    explicit TVectorType (TType *baseType);
    
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    virtual bool isVector () const override;
    
    virtual TType *getBaseType () const override;
    
private:
    TType *baseType;
};

class TReferenceType: public TType {
typedef TType inherited;
public:
    explicit TReferenceType (TType *baseType);
    
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    virtual bool isReference () const override;
    
    virtual TType *getBaseType () const override;
    
private:
    TType *baseType;
};
    
class TRoutineType: public TType {
typedef TType inherited;
public:
    TRoutineType (TSymbolList &&parameters, TType *returnType, bool farCall = false);
    
    virtual std::string getName () const override;
    virtual std::size_t getSize () const override;
    virtual bool isSerializable () const override;
    virtual bool isRoutine () const override;
    
//    std::size_t getNumberOfParameters () const;
//    const TSymbol *getParameter (std::size_t) const;
    TSymbolList &getParameter ();
    const TSymbolList &getParameter () const;
    TType *getReturnType () const;
    bool parameterEmpty () const;
    
    bool isFarCall () const { return farCall; }		// TMS9900 only;
    void setFarCall (bool f) { farCall = f; }
    void setReturnType (TType *t) { returnType = t; }
    
    bool matchesForward (const TRoutineType *other) const;
    bool matchesOverload (const TRoutineType *other) const;
    
private:
    bool matches (const TRoutineType *other, bool checkFormalParameterNames) const;

    TSymbolList parameters;
    TType *returnType;
    bool farCall;
};

class TShortStringType: public TArrayType {
typedef TArrayType inherited;
public:
    TShortStringType (TEnumeratedType *length);
    
    virtual std::string getName () const override;
    virtual bool isShortString () const override;
};

// standard data types as global constants

class TStdType {
public:
    TStdType ();
    
    TEnumeratedType Boolean, Char, Int64;
    TSubrangeType Uint8, Int8, Uint16, Int16, Uint32, Int32;
    TVoidType Void, GenericVar;
    TRealType Real;
    TSingleType Single;
    TStringType String;
    TShortStringType ShortString;
    TSetType GenericSet;
    TPointerType GenericPointer;
    TUnresOverloadType UnresOverload;
    TVectorType Int64Vector, BooleanVector, RealVector, StringVector, GenericVector;
    
    enum TScalarTypeCode {
        s64, s32, s16, s8, u8, u16, u32, single, real, count, invalid
    };
    static const std::size_t scalarTypeSizes [static_cast<std::size_t> (TScalarTypeCode::count)];

    static TScalarTypeCode getScalarTypeCode (const TType *);
    
};

extern TStdType stdType;


}
