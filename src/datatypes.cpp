// ---

#include "datatypes.hpp"
#include "anyvalue.hpp"
#include "config.hpp"

#include <limits>

namespace statpascal {

TType::TType ():
  alignment (0) {
}

bool TType::isSerializable () const {
    return true;
}

void TType::setAlignment (std::size_t n) {
    alignment = n;
}

std::size_t TType::getAlignment () const {
    return alignment;
}

bool TType::isVoid () const {
    return false;
}

bool TType::isReal () const {
    return false;
}

bool TType::isSingle () const {
    return false;
}

bool TType::isString () const {
    return false;
}

bool TType::isEnumerated () const {
    return false;
}

bool TType::isSubrange () const {
    return false;
}

bool TType::isArray () const {
    return false;
}

bool TType::isRecord() const  {
    return false;
}

bool TType::isReference () const {
    return false;
}

bool TType::isPointer () const  {
    return false;
}

bool TType::isRoutine () const {
    return false;
}

bool TType::isSet () const {
    return false;
}

bool TType::isFile () const {
    return false;
}

bool TType::isVector () const {
    return false;
}

TType *TType::getBaseType () const {
    // TOD=: internal error
    throw std::bad_cast ();
}


std::string TVoidType::getName () const {
    return "void";
}

std::size_t TVoidType::getSize () const {
    return 0;
}

bool TVoidType::isVoid () const {
    return true;
}


std::string TUnresOverloadType::getName () const {
    return "unresolved overloaded type";
}

std::size_t TUnresOverloadType::getSize () const {
    return 0;
}


std::string TRealType::getName () const {
    return "real";
}

std::size_t TRealType::getSize () const {
    return sizeof (double);
}

bool TRealType::isReal () const {
    return true;
}


std::string TSingleType::getName () const {
    return "single";
}

std::size_t TSingleType::getSize () const {
    return sizeof (float);
}

bool TSingleType::isSingle () const {
    return true;
}


std::string TStringType::getName () const {
    return "string";
}

std::size_t TStringType::getSize () const {
    return sizeof (void *);
}

bool TStringType::isSerializable () const {
    return false;
}

bool TStringType::isString () const {
    return true;
}


TEnumeratedType::TEnumeratedType (const std::string &name, std::int64_t minVal, std::int64_t maxVal):
  name (name), minVal (minVal), maxVal (maxVal), size (sizeof (std::int64_t)) {
    checkLimits<std::uint8_t> () ||
    checkLimits<std::int8_t> () ||
    checkLimits<std::uint16_t> () ||
    checkLimits<std::int16_t> () ||
    checkLimits<std::uint32_t> () ||
    checkLimits<std::int32_t> ();
}

template<typename T> bool TEnumeratedType::checkLimits () {
    if (static_cast<std::int64_t> (minVal) >= static_cast<std::int64_t> (std::numeric_limits<T>::min ()) && 
        static_cast<std::int64_t> (maxVal) <= static_cast<std::int64_t> (std::numeric_limits<T>::max ())) {
        size = sizeof (T);
        return true;
    }
    return false;
}

std::string TEnumeratedType::getName () const {
    return name;
}

std::size_t TEnumeratedType::getSize () const {
    return size;
}

bool TEnumeratedType::isEnumerated () const {
    return true;
}

std::int64_t TEnumeratedType::getMinVal () const {
    return minVal;
}

std::int64_t TEnumeratedType::getMaxVal () const {
    return maxVal;
}

bool TEnumeratedType::isSigned () const {
    return getMinVal () < 0;
}


TSubrangeType::TSubrangeType (const std::string &name, TType *baseType, std::int64_t minval, std::int64_t maxval):
  inherited (name, minval, maxval),  
  baseType (baseType) {
}

std::string TSubrangeType::getName () const {
    std::string name = inherited::getName ();
    if (name.empty ())
        return "subrange (" + std::to_string (getMinVal ()) + ".." + std::to_string (getMaxVal ()) + ") of " + baseType->getName ();
    else
        return name;
}

bool TSubrangeType::isSubrange () const {
    return true;
}

TType *TSubrangeType::getBaseType () const {
    return baseType;
}


TArrayType::TArrayType (TType *baseType, TEnumeratedType *indexType):
  baseType (baseType), indexType (indexType) {
}

std::string TArrayType::getName () const {
    return "array [" + indexType->getName () + "] of " + baseType->getName ();
}

std::size_t TArrayType::getSize () const {
    // check for overflow !!!!
    return (indexType->getMaxVal () - indexType->getMinVal () + 1) * baseType->getSize ();
}

bool TArrayType::isSerializable () const {
    return baseType->isSerializable ();
}

bool TArrayType::isArray () const {
    return true;
}

TType *TArrayType::getBaseType () const {
    return baseType;
}

TEnumeratedType *TArrayType::getIndexType () const {
    return indexType;
}


TRecordType::TRecordType (TSymbolList *components):
  size (0) {
    recordFields.components = components;
    activeVariants.push (&recordFields);
}

std::string TRecordType::getName () const {
    return "record";
}

std::size_t TRecordType::getSize () const {
    return size;
}

bool TRecordType::isSerializable () const {
    for (const TComponentMap::value_type &it: componentMap)
        if (!it.second->getType ()->isSerializable ())
            return false;
    return true;
}

bool TRecordType::isRecord () const {
    return true;
}

TSymbolList::TAddSymbolResult TRecordType::addComponent (const std::string &name, TType *type) {
    if (TSymbol *s = searchComponent (name))
        return TSymbolList::TAddSymbolResult {s, true};
    else {
        TSymbolList::TAddSymbolResult result =  activeVariants.top ()->components->addVariable (name, type);
        componentMap [name] = result.symbol;
        return result;
    }
}

void TRecordType::enterVariant (TSymbolList *components) {
    activeVariants.top ()->variants.push_back ({components});
    TRecordFields *p = &(activeVariants.top ()->variants.back ());
    activeVariants.push (p);
}

void TRecordType::leaveVariant () {
    activeVariants.pop ();
}

TSymbol *TRecordType::searchComponent (const std::string &name) const {
    const TComponentMap::const_iterator it = componentMap.find (name);
    return (it == componentMap.end ()) ? nullptr : it->second;
}

TRecordType::TRecordFields &TRecordType::getRecordFields () {
    return recordFields;
}

const TRecordType::TRecordFields &TRecordType::getRecordFields () const {
    return recordFields;
}

void TRecordType::setSize (std::size_t n) {
    size = n;
}


TFileType::TFileType (TType *baseType, TSymbolList *components):
  inherited (components),
  baseType (baseType) {
    addComponent ("idx", &stdType.Int64);
    addComponent ("fn", &stdType.String);
    addComponent ("blksize", &stdType.Int64);
    addComponent ("binary", &stdType.Boolean);
}

std::string TFileType::getName () const {
    if (baseType)
        return "file of " + baseType->getName ();
    else
        return "file";
}

bool TFileType::isSerializable () const {
    return false;
}

bool TFileType::isFile () const {
    return true;
}

void TFileType::setBaseType (TType *t) {
    baseType = t;
}

TType *TFileType::getBaseType () const {
    return baseType;
}


TPointerType::TPointerType (TType *baseType):
  baseType (baseType) {
}

TPointerType::TPointerType (const std::string &baseName):
  baseType (nullptr), baseName (baseName) {
}

std::string TPointerType::getName () const {
    std::string result = "pointer to ";
    if (baseType)
        result += baseType->getName ();
    else
        result += " unknown";
    if (!baseName.empty ())
        result += " (" + baseName + ')';
    return result;
}

std::size_t TPointerType::getSize () const {
#ifdef CREATE_9900
    return 2;
#else
    return sizeof (void *);
#endif    
}

bool TPointerType::isSerializable () const {
    return false;
}

bool TPointerType::isPointer () const {
    return true;
}

void TPointerType::setBaseType (TType *baseType_para) {
    baseType = baseType_para;
}

TType *TPointerType::getBaseType () const {
    return baseType;
}

std::string TPointerType::getBaseName () const {
    return baseName;
}


TSetType::TSetType (TEnumeratedType *baseType):
  baseType (baseType) {
}

std::string TSetType::getName () const {
    if (baseType)
        return "set of " + baseType->getName ();
    else
        return "empty set";
}

std::size_t TSetType::getSize () const {
    return sizeof (std::int64_t) * TConfig::setwords;
}

bool TSetType::isSerializable () const {
    return true;
}

bool TSetType::isSet () const {
    return true;
}

TEnumeratedType *TSetType::getBaseType () const {
    return baseType;
}


TVectorType::TVectorType (TType *baseType):
  baseType (baseType) {
}

std::string TVectorType::getName () const {
    return "vector of " + baseType->getName ();
}

std::size_t TVectorType::getSize () const {
    return sizeof (TAnyValue);
}

bool TVectorType::isSerializable () const {
    return false;
}

bool TVectorType::isVector () const {
    return true;
}

TType *TVectorType::getBaseType () const {
    return baseType;
}


TReferenceType::TReferenceType (TType *baseType):
  baseType (baseType) {
}

std::string TReferenceType::getName () const {
    return "reference to " + baseType->getName ();
}

std::size_t TReferenceType::getSize () const {
#ifdef CREATE_9900
    return 2;
#else
    return sizeof (void *);
#endif    
}

bool TReferenceType::isSerializable () const {
    return false;
}

bool TReferenceType::isReference () const {
    return true;
}

TType *TReferenceType::getBaseType () const {
    return baseType;
}


TRoutineType::TRoutineType (TSymbolList &&parameters, TType *returnType):
  parameters (std::move (parameters)), returnType (returnType) {
}

std::string TRoutineType::getName () const {
    std::string s = "routine: ";
    for (const TSymbol *parameter: parameters)
        s.append (parameter->getType ()->getName () + " ");
    s.append ("-> " + returnType->getName ());
    return s;
}

std::size_t TRoutineType::getSize () const {
    return sizeof (void (*)());
}

bool TRoutineType::isSerializable () const {
    return false;
}

bool TRoutineType::isRoutine () const {
    return true;
}

TSymbolList &TRoutineType::getParameter () {
    return parameters;
}

const TSymbolList &TRoutineType::getParameter () const {
    return parameters;
}

TType *TRoutineType::getReturnType () const {
    return returnType;
}

bool TRoutineType::parameterEmpty () const {
    return parameters.empty ();
}

bool TRoutineType::matches (const TRoutineType *other, bool checkFormalParameterNames) const {

    // TODO !!!! Das gibt es bei Zuweisung von prozedurealen Variablen in aehnlicher Form !!!!
    // TODO !!!! Ueberladung behandeln

    if (returnType != other->returnType || parameters.size () != other->parameters.size ())
        return false;
    TSymbolList::TBaseContainer::const_iterator it1 = parameters.begin (), it2 = other->parameters.begin ();
    while (it1 != parameters.end ()) {
        const TSymbol *s1 = *it1++, *s2 = *it2++;
        const TType *t1 = s1->getType (), *t2 = s2->getType ();
        if (t1->isReference () && t2->isReference ()) {
            t1 = t1->getBaseType ();
            t2 = t2->getBaseType ();
        }
        if (t1 != t2 || (checkFormalParameterNames && s1->getName () != s2->getName ()))
            return false;
    }
    return true;
}

bool TRoutineType::matchesForward (const TRoutineType *other) const {
    return matches (other, true);
}

bool TRoutineType::matchesOverload (const TRoutineType *other) const {
    return matches (other, false);
}

TStdType::TStdType ():
  Boolean ("boolean", 0, 1),
  Char ("char", 0, 255), 
  Int64 ("int64", std::numeric_limits<std::int64_t>::min (), std::numeric_limits<std::int64_t>::max ()),
  Uint8 ("uint8", &Int64, std::numeric_limits<std::uint8_t>::min (), std::numeric_limits<std::uint8_t>::max ()), 
  Int8 ("int8", &Int64, std::numeric_limits<std::int8_t>::min (), std::numeric_limits<std::int8_t>::max ()), 
  Uint16 ("uint16", &Int64, std::numeric_limits<std::uint16_t>::min (), std::numeric_limits<std::uint16_t>::max ()), 
  Int16 ("int16", &Int64, std::numeric_limits<std::int16_t>::min (), std::numeric_limits<std::int16_t>::max ()), 
  Uint32 ("uint32", &Int64, std::numeric_limits<std::uint32_t>::min (), std::numeric_limits<std::uint32_t>::max ()), 
  Int32 ("int32", &Int64, std::numeric_limits<std::int32_t>::min (), std::numeric_limits<std::int32_t>::max ()),
  Void (), 
  GenericVar (),
  Real (),
  Single (),
  String (),
  GenericSet (nullptr),
  GenericPointer (&Void),
  UnresOverload (),
  Int64Vector (&Int64), 
  BooleanVector (&Boolean), 
  RealVector (&Real), 
  StringVector (&String), 
  GenericVector (&Void) {
}

const std::size_t TStdType::scalarTypeSizes [static_cast<std::size_t> (TStdType::TScalarTypeCode::count)] =
  {8, 4, 2, 1, 1, 2, 4, 4, 8};

TStdType::TScalarTypeCode TStdType::getScalarTypeCode (const TType *type) {
    if (type == &stdType.Real)
        return TScalarTypeCode::real;
    if (type == &stdType.Single)
        return TScalarTypeCode::single;
    if (type->isEnumerated ()) {
        const TEnumeratedType *enumeratedType = static_cast<const TEnumeratedType *> (type);
        bool isSigned = enumeratedType->isSigned ();
        switch (enumeratedType->getSize ()) {
            case 1: 
                return isSigned ? TScalarTypeCode::s8 : TScalarTypeCode::u8;
            case 2:
                return isSigned ? TScalarTypeCode::s16 : TScalarTypeCode::u16;
            case 4:
                return isSigned ? TScalarTypeCode::s32 : TScalarTypeCode::u32;
            case 8:
                return TScalarTypeCode::s64;
        }
    }
    return TScalarTypeCode::invalid;
}

TStdType stdType;

}
