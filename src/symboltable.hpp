/** \file symboltable.hpp
*/

#pragma once

#include <array>
#include <vector>
#include <unordered_map>

#include "constant.hpp"
#include "mempoolfactory.hpp"

namespace statpascal {

class TBlock; class TRoutineType;

class TSymbol: public TMemoryPoolObject {
public:
    enum TFlags {
        None = 0,
        Variable = 1,
        StaticVariable = 2,
        Constant = 4,
        Parameter = 8,
        Forward = 16,
        External = 32,
        Export = 64,
        Alias = 128, 
        Absolute = 256,
        Label = 512,
        NamedType = 1024,
        Routine = 2048,
        Intrinsic = 4096,
        AllSymbols = 8192 - 1
    };
    
    TSymbol (const std::string &name, TType *type, std::size_t level, TFlags flags, TSymbol *alias);
    ~TSymbol () = default;

    void setName (const std::string &s);    
    const std::string &getName () const;
    TSymbol *getAlias () const;
    
    void setExternal (const std::string &libName, const std::string &symbolName);
    const std::string &getExtLibName () const;
    const std::string &getExtSymbolName () const;
    
    void setType (TType *);
    TType *getType () const;
    
    void setOffset (ssize_t);
    ssize_t getOffset () const;
    void setParameterPosition (ssize_t);
    ssize_t getParameterPosition () const;

    void setRegister (ssize_t);
    ssize_t getRegister () const;
    
    /** set if an alias to the symbol exists: address operator, absolute declaration or type cast. */
    void setAliased ();
    bool isAliased () const;

    // Block syntax tree node for subroutine symbols    
    void setBlock (TBlock *);
    TBlock *getBlock () const;

    void setLevel (std::size_t);    
    std::size_t getLevel () const;

    void addSymbolFlags (TFlags);
    void removeSymbolFlags (TFlags);
    TFlags getSymbolFlags () const;
    bool checkSymbolFlag (TFlags) const; 
    
    // store values of constant symbols 
    void setConstant (const TConstant *);
    const TConstant *getConstant () const;

    // set level/offset from alias
    void setAliasData ();
    
    static const ssize_t LabelDefined = -1, UndefinedLabelUsed = -2, InvalidRegister = -1;
private:
    std::string name, libName, extSymbolName;
    std::size_t level;
    TFlags flags;
    TSymbol *alias;
    TBlock *block;
    ssize_t offset, parameterPosition, assignedRegister;
    bool aliased;
    TType *type;
    const TConstant *constantValue;
};


class TSymbolList final: public TMemoryPoolObject {
public:
    TSymbolList (TSymbolList *previousLevel, TMemoryPoolFactory &);
    TSymbolList (TSymbolList *symbols, TMemoryPoolFactory &, std::size_t level);

    struct TAddSymbolResult {
        TSymbol *symbol;
        bool alreadyPresent;
    };

    TAddSymbolResult addRoutine (const std::string &name, TRoutineType *type);
    TAddSymbolResult addVariable (const std::string &name, TType *type);
    TAddSymbolResult addStaticVariable (const std::string &name, const TConstant *);
    TAddSymbolResult addParameter (const std::string &name, TType *type);
    TAddSymbolResult addConstant (const std::string &name, const TSimpleConstant *);
    TAddSymbolResult addAlias (const std::string &name, TType *type, TSymbol *alias);
    TAddSymbolResult addAbsolute (const std::string &name, TType *type, std::int64_t addr);
    TAddSymbolResult addLabel (const std::string &name);
    TAddSymbolResult addNamedType (const std::string &name, TType *type);
    
    TSymbol *searchSymbol (const std::string &name, TSymbol::TFlags = TSymbol::AllSymbols) const;
    std::vector<TSymbol *> searchSymbols (const std::string &name, TSymbol::TFlags = TSymbol::AllSymbols) const;
    void removeSymbol (const TSymbol *);
    
    TSymbol *makeLocalLabel (char c);	// -> yields --c - makeUniqueLabelNames in TBaseGenerator provides distinct names. 
                                        // Labels with c = 'l' are removed by optimzier if not referenced in block (case jump tables are outside)
    
    void moveSymbols (TSymbol::TFlags, TSymbolList &dest);
    void copySymbols (TSymbol::TFlags, TSymbolList &dest);
    
    std::size_t getLevel () const;
    
    std::size_t getParameterSize () const;
    std::size_t getLocalSize () const;
    void setParameterSize (std::size_t);
    void setLocalSize (std::size_t);
    
    void setPreviousLevel (TSymbolList *);
    TSymbolList *getPreviousLevel () const;
    
    using TBaseContainer = std::vector<TSymbol *>;
    std::size_t size () const;
    bool empty () const;
    TBaseContainer::iterator begin (), end ();
    TBaseContainer::const_iterator begin () const, end () const;
    
private:
    TAddSymbolResult addSymbol (const std::string &name, TType *type, TSymbol::TFlags flags, TSymbol *alias);
    TSymbolList *previousLevel;    
    TMemoryPoolFactory &memoryPoolFactory;
    TBaseContainer symbols;    
    std::size_t parameterSize, localSize, level;
};

// ---

inline std::size_t TSymbolList::getLevel () const {
    return level;
}

inline std::size_t TSymbolList::size () const {
    return symbols.size ();
}

inline bool TSymbolList::empty () const {
    return symbols.empty ();
}

inline TSymbolList::TBaseContainer::iterator TSymbolList::begin () {
    return symbols.begin ();
}

inline TSymbolList::TBaseContainer::iterator TSymbolList::end () {
    return symbols.end ();
}

inline TSymbolList::TBaseContainer::const_iterator TSymbolList::begin () const {
    return symbols.begin ();
}

inline TSymbolList::TBaseContainer::const_iterator TSymbolList::end () const {
    return symbols.end ();
}


inline void TSymbol::setName (const std::string &s) {
    name = s;
}

inline const std::string &TSymbol::getName () const {
    return name;
}

inline TSymbol *TSymbol::getAlias () const {
    return alias;
}

inline const std::string &TSymbol::getExtLibName () const {
    return libName;
}

inline const std::string &TSymbol::getExtSymbolName () const {
    return extSymbolName;
}

inline void TSymbol::setType (TType *t) {
    type = t;
}

inline TType *TSymbol::getType () const {
    return type;
}

inline void TSymbol::setOffset (ssize_t n) {
    offset = n;
}

inline ssize_t TSymbol::getOffset () const {
    return offset;
}

inline void TSymbol::setParameterPosition (ssize_t n) {
    parameterPosition = n;
}

inline ssize_t TSymbol::getParameterPosition () const {
    return parameterPosition;
}

inline void TSymbol::setRegister (ssize_t n) {
    assignedRegister = n;
}

inline ssize_t TSymbol::getRegister () const {
    return assignedRegister;
}

inline void TSymbol::setAliased () {
    aliased = true;
}

inline bool TSymbol::isAliased () const {
    return aliased || !!alias;
}

inline void TSymbol::setBlock (TBlock *b) {
    block = b;
}

inline TBlock *TSymbol::getBlock () const {
    return block;
}

inline void TSymbol::setLevel (std::size_t n) {
    level = n;
}

inline std::size_t TSymbol::getLevel () const {
    return level;
}

inline TSymbol::TFlags TSymbol::getSymbolFlags () const {
    return flags;
}

inline void TSymbol::addSymbolFlags (TFlags f) {
    flags = static_cast<TFlags> (flags | f);
}

inline void TSymbol::removeSymbolFlags (TFlags f) {
    flags = static_cast<TFlags> (flags & ~f);
}

inline bool TSymbol::checkSymbolFlag (TFlags f) const {
    return (flags & f) == f;
}

inline void TSymbol::setConstant (const TConstant *constant) {
    constantValue = constant;
    type = constant->getType ();
}

inline const TConstant *TSymbol::getConstant () const {
    return constantValue;
}

}
