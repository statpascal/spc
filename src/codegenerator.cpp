#include "codegenerator.hpp"
#include "anymanager.hpp"
#include "runtime.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <numeric>
#include <string.h>


namespace statpascal {

TCodeGenerator::~TCodeGenerator () {
}

TBaseGenerator::TBaseGenerator (TRuntimeData &runtimeData):
  labelCount (0),
  runtimeData (runtimeData),
  globalDataArea (nullptr)  {
}

TBaseGenerator::~TBaseGenerator () {
    operator delete (globalDataArea);
}

void *TBaseGenerator::getGlobalDataArea () const {
    return globalDataArea;
}

TRuntimeData *TBaseGenerator::getRuntimeData () const {
    return &runtimeData;
}

TType *TBaseGenerator::getMemoryOperationType (TType *const type) {
    static const std::array<TType *, 9> baseTypes = {&stdType.Uint8, &stdType.Int8, &stdType.Uint16, &stdType.Int16, &stdType.Uint32, &stdType.Int32, &stdType.Int64, &stdType.Real, &stdType.Single};
    if (std::find (baseTypes.begin (), baseTypes.end (), type) != baseTypes.end ())
        return type;
    if (type->isEnumerated ()) {
        const TEnumeratedType *enumType = reinterpret_cast<const TEnumeratedType *> (type);
        switch (enumType->getSize ()) {
            case 1:
                return enumType->isSigned () ? &stdType.Int8 : &stdType.Uint8;
            case 2:
                return enumType->isSigned () ? &stdType.Int16 : &stdType.Uint16;
            case 4:
                return enumType->isSigned () ? &stdType.Int32 : &stdType.Uint32;
            case 8:
                return &stdType.Int64;
        }
    }
    if (type->isPointer () || type->isRoutine ())
        return &stdType.Int64;
//    if (type->isVector ())
//        return &stdType.GenericVector;
        
    return &stdType.UnresOverload;
}

bool TBaseGenerator::getSetTypeLimit (const TExpressionBase *expr, std::int64_t &minval, std::int64_t &maxval) {
    const TType *type = expr->getType ();
    if (expr->isTypeCast ())
        type = static_cast<const TTypeCast *> (expr)->getExpression ()->getType ();
    if (type->isEnumerated ()) {
        const TEnumeratedType *valueType = static_cast<const TEnumeratedType *> (type);
        minval = valueType->getMinVal ();
        maxval = valueType->getMaxVal ();
        return true;
    }
    return false;
}

void TBaseGenerator::assignAlignedOffset (TSymbol &s, ssize_t &offset, std::size_t align) {
    offset = (offset + align - 1) & ~(align - 1);
    s.setOffset (offset);
    offset += s.getType ()->getSize ();
}

void TBaseGenerator::alignRecordFields (TRecordType::TRecordFields &recordFields, std::size_t &size, std::size_t &alignment) {
    TSymbolList &symbolList = *recordFields.components;
    ssize_t position = 0;
    for (TSymbol *s: symbolList) {
        TType *type = s->getType ();
        alignType (type);
        const std::size_t fieldAlignment = type->getAlignment ();
        assignAlignedOffset (*s, position, fieldAlignment);
        alignment = std::max (alignment, fieldAlignment);
    }
    if (!recordFields.variants.empty ()) {
        std::size_t variantMaxSize = 0, variantSize = 0, variantMaxAlign = 0, variantAlign = 0;
        for (TRecordType::TRecordFields &r: recordFields.variants) {
            alignRecordFields (r, variantSize, variantAlign);
            variantMaxSize = std::max (variantMaxSize, variantSize);
            variantMaxAlign = std::max (variantMaxAlign, variantAlign);
        }
        position = (position + variantMaxAlign - 1) & ~(variantMaxAlign - 1);
        for (TRecordType::TRecordFields &r: recordFields.variants)
            addRecordFieldOffset (r, position);
        position += variantMaxSize;
        alignment = std::max (alignment, variantMaxAlign);
    }
    size = position;
}

void TBaseGenerator::addRecordFieldOffset (TRecordType::TRecordFields &recordFields, const std::size_t offset) {
    for (TSymbol *s: *recordFields.components)
        s->setOffset (s->getOffset () + offset);
    for (TRecordType::TRecordFields &r: recordFields.variants)
        addRecordFieldOffset (r, offset);
}

void TBaseGenerator::alignType (TType *type) {
    const unsigned maxAlign = TConfig::alignment;
    static const unsigned alignment [] = {1, 1, 2, 4, 4, 8, 8, 8};
    assert (maxAlign <= 8);
    
    if (!type->getAlignment ()) {
        if (type->isArray ()) {
            TArrayType *arrayType = static_cast<TArrayType *> (type);
            alignType (arrayType->getBaseType ());
            arrayType->setAlignment (arrayType->getBaseType ()->getAlignment ());
        } else if (type->isRecord ()) {
            TRecordType *recordType = static_cast<TRecordType *> (type);
            std::size_t size = 0, alignment = 0;
            alignRecordFields (recordType->getRecordFields (), size, alignment);
            recordType->setAlignment (alignment);
            recordType->setSize ((size + alignment - 1) & ~(alignment - 1));
/*            
            std::cout << "Record: " << type->getName () << std::endl;
            for (const TRecordType::TComponentMap::value_type &it: recordType->componentMap)
                std::cout << it.second->getName () << ": " << it.second->getOffset () << std::endl;
            std::cout << std::endl;
*/
        } else {
            const std::size_t size = type->getSize ();
            type->setAlignment (size < maxAlign ? alignment [size] : alignment [maxAlign - 1]);
            if (type->isPointer () || type->isVector () || type->isSet ())
                if (TType *baseType = type->getBaseType ())
                    alignType (baseType);
        }
    }
}

void TBaseGenerator::assignStackOffsets (ssize_t &pos, TSymbolList &symbolList, bool handleParameters) {
    for (TSymbol *s: symbolList)
        if (s->getRegister () == TSymbol::InvalidRegister)
        if ((handleParameters && s->checkSymbolFlag (TSymbol::Parameter)) ||
            (!handleParameters && s->checkSymbolFlag (TSymbol::Variable) && !s->checkSymbolFlag (TSymbol::Alias))) {
            TType *type = s->getType ();
            alignType (type);
            assignAlignedOffset (*s, pos, type->getAlignment ());
        }
}

TBaseGenerator::TTypeAnyManager TBaseGenerator::lookupAnyManager (const TType *type) {
    TTypeAnyManagerMap::const_iterator it = typeAnyManagerMap.find (type);
    if (it != typeAnyManagerMap.end ())
        return it->second;
    if (TAnyManager *anyManager = buildAnyManager (type)) {
        std::size_t index = runtimeData.registerAnyManager (anyManager);
        TTypeAnyManager typeAnyManager {anyManager, index};
        typeAnyManagerMap [type] = typeAnyManager;
        return typeAnyManager;
    }
    return {nullptr, 0};
}

TAnyManager *TBaseGenerator::buildAnyManager (const TSymbolList &symbolList) {
    TAnyRecordManager *result = new TAnyRecordManager ();
    for (const TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Parameter) || s->checkSymbolFlag (TSymbol::Variable)) {
            TType *type = s->getType ();
            if (TAnyManager *anyManager = buildAnyManager (type))
                result->appendComponent (anyManager, s->getOffset ());
        }
        
    if (result->isEmpty ()) {
        delete result;
        result = nullptr;
    }
    return result;
}

TAnyManager *TBaseGenerator::buildAnyManagerRecord (const TRecordType *recordType) {
    return buildAnyManager (*recordType->getRecordFields ().components);
}

TAnyManager *TBaseGenerator::buildAnyManagerArray (const TType *type) {
    std::size_t count = 1;
    while (type->isArray ()) {
        const TArrayType *arrayType = static_cast<const TArrayType *> (type);
        count *= (arrayType->getIndexType ()->getMaxVal () - arrayType->getIndexType ()->getMinVal () + 1);
        type = arrayType->getBaseType ();
    }
    if (TAnyManager *anyManager = buildAnyManager (type))
        return new TAnyArrayManager (anyManager, count, type->getSize ());
    else
        return nullptr;
}

TAnyManager *TBaseGenerator::buildAnyManager (const TType *type) {
    if (type->isArray ())
        return buildAnyManagerArray (type);
    if (type->isRecord ())
        return buildAnyManagerRecord (static_cast<const TRecordType *> (type));
    if (type->isString () || type->isVector ())
        return new TAnySingleValueManager;
    return nullptr;
}

void TCodeGenerator::visit (TSyntaxTreeNode *node) {
    if (node)
        node->acceptCodeGenerator (*this);
}

std::size_t TBaseGenerator::registerAnyManager (TAnyManager *anyManager) {
    return runtimeData.registerAnyManager (anyManager);
}

std::size_t TBaseGenerator::registerStringConstant (const std::string &s) {
    return runtimeData.registerStringConstant (s);
}

std::size_t TBaseGenerator::registerData (const void *p, std::size_t n) {
    return runtimeData.registerData (p, n);
}

void TBaseGenerator::allocateGlobalDataArea (std::size_t n) {
    globalDataArea = operator new (n);
    bzero (globalDataArea, n);
}

void TBaseGenerator::initStaticVariable (char *addr, const TType *t, const TConstant *constant) {
    if (t->isEnumerated ()) {
        std::uint64_t n = static_cast<const TSimpleConstant *> (constant)->getInteger ();
        memcpy (addr, &n, t->getSize ());
    } else if (t->isReal ())
        *reinterpret_cast<double *> (addr) = static_cast<const TSimpleConstant *> (constant)->getDouble ();
    else if (t->isSingle ())
        *reinterpret_cast<float *> (addr) = static_cast<const TSimpleConstant *> (constant)->getDouble ();
    else if (t->isString ()) 
        *reinterpret_cast<TAnyValue *> (addr) = runtimeData.getStringConstant (registerStringConstant (static_cast<const TSimpleConstant *> (constant)->getString ()));
    else if (t->isRoutine ()) 
        initStaticRoutinePtr (reinterpret_cast<std::uint64_t> (addr), static_cast<const TSimpleConstant *> (constant)->getRoutineValue ());
        
/*        *reinterpret_cast<std::size_t *> (addr) = static_cast<const TSimpleConstant *> (constant)->getRoutineValue ()->getSymbol ()->getOffset ();
        outputCode (TX64Op::lea, TX64Reg::rax, TX64Operand (static_cast<const TSimpleConstant *> (constant)->getRoutineValue ()->getSymbol ()->getOverloadName (), true));
        outputCode (TX64Op::mov, TX64Operand (TX64Reg::none, reinterpret_cast<std::uint64_t> (addr)), TX64Reg::rax);

*/        
    else if (t->isSet ())
        for (std::size_t index = 0; index < TConfig::setwords; ++index) 
            reinterpret_cast<std::uint64_t *> (addr) [index] = static_cast<const TSimpleConstant *> (constant)->getSetValues () [index];
    else if (t->isRecord ()) {
        for (const TRecordConstant::TRecordValue &val: static_cast<const TRecordConstant *> (constant)->getValues ())
            if (const TSymbol *symbol = static_cast<const TRecordType *> (t)->searchComponent (val.component))
                initStaticVariable (addr + symbol->getOffset (), symbol->getType (), val.c);
    } else if (t->isArray ()) {
        const TArrayType *arrayType = static_cast<const TArrayType *> (t);
        const std::size_t arraySize = arrayType->getIndexType ()->getMaxVal () - arrayType->getIndexType ()->getMinVal () + 1;
        for (std::size_t index = 0; index < arraySize; ++index) 
            initStaticVariable (addr + index * arrayType->getBaseType ()->getSize (), arrayType->getBaseType (), static_cast<const TArrayConstant *> (constant)->getValues () [index]);
    }
}

void TBaseGenerator::assignGlobals (TSymbolList &blockSymbols) {
    allocateGlobalDataArea (blockSymbols.getLocalSize ());    
    std::uint64_t dataAreaAddr = reinterpret_cast<std::uint64_t> (getGlobalDataArea ());

    bool firstSymbol = true;
    std::uint64_t firstSymbolOffset = 0;
    for (TSymbol *s: blockSymbols)
        if (s->checkSymbolFlag (TSymbol::Variable) || s->checkSymbolFlag (TSymbol::StaticVariable) || s->checkSymbolFlag (TSymbol::Alias)) {
            if (firstSymbol) {
                firstSymbolOffset = s->getOffset ();
                firstSymbol = false;
            }
            s->setOffset (dataAreaAddr + s->getOffset () - firstSymbolOffset);
//            if (s->checkSymbolFlag (TSymbol::Variable))
//                std::cout << s->getName () << ": " << std::hex << s->getOffset () << std::endl;
            if (s->getName () == TConfig::globalRuntimeDataPtr)
                *reinterpret_cast<void **> (s->getOffset ()) = &runtimeData;
        }
}

void TBaseGenerator::initStaticGlobals (TSymbolList &globalSymbols) {
    for (TSymbol *s: globalSymbols)
            if (s->checkSymbolFlag (TSymbol::StaticVariable))
                initStaticVariable (reinterpret_cast<char *> (s->getOffset ()), s->getType (), s->getConstant ());
}

std::vector<std::string> TBaseGenerator::createSymbolList (const std::string &routineName, std::size_t level, TSymbolList &symbolList, const std::vector<std::string> &regNames) {
    std::vector<std::string> headerListing;
    std::stringstream ss;
    switch (level) {
        case 0:
            break;
        case 1:
            ss << "Program: ";
            break;
        default:
            ss << "Subroutine: ";
            break;
    }
    ss << routineName << ", level: " << level;
    headerListing.push_back (ss.str ());
    for (const TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Variable) || s->checkSymbolFlag (TSymbol::Parameter)) {
            ss.str (std::string ());
            ss << std::setw (6);
            if (s->getRegister () != TSymbol::InvalidRegister)
                ss << regNames [s->getRegister ()];
            else
                ss << s->getOffset ();
            ss << "  ";
            if (s->getType ()->isReference ())
                ss << "var ";
            ss << s->getOverloadName () << ": " << s->getType ()->getName ();;
            headerListing.push_back (ss.str ());
        }
    return headerListing;
}

std::string TBaseGenerator::getNextLocalLabel () {
    return ".l" + std::to_string (labelCount++) + '$';
}

std::string TBaseGenerator::getNextCaseLabel () {
    return ".c" + std::to_string (labelCount++) + '$';
}

}
