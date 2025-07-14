/** \file symboltable.hpp
*/

#pragma once

#include <array>
#include <vector>

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
        Label = 256,
        NamedType = 512,
        Routine = 1024,
        AllSymbols = 2048 - 1
    };
    
    TSymbol (const std::string &name, TType *type, std::size_t level, TFlags flags, TSymbol *alias);
    ~TSymbol () = default;

    void setName (const std::string &s);    
    std::string getName () const;
    TSymbol *getAlias () const;
    
    void setExternal (const std::string &libName, const std::string &symbolName);
    std::string getExtLibName () const;
    std::string getExtSymbolName () const;
    
    void setOverloadName (const std::string &);
    std::string getOverloadName () const;
    
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

    // forward calls    
    void appendUnresolvedReferencePosition (std::size_t);
    const std::vector<std::size_t> &getUnresolvedReferencePositions () const;
    
    // set level/offset from alias
    void setAliasData ();
    
    static const ssize_t LabelDefined = -1, UndefinedLabelUsed = -2, InvalidRegister = -1;
private:
    std::string name, libName, extSymbolName, overloadName;
    std::size_t level;
    TFlags flags;
    TSymbol *alias;
    TBlock *block;
    ssize_t offset, parameterPosition, assignedRegister;
    bool aliased;
    TType *type;
    const TConstant *constantValue;
    std::vector<std::size_t> unresolvedReferencePositions;
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
    TAddSymbolResult addLabel (const std::string &name);
    TAddSymbolResult addNamedType (const std::string &name, TType *type);
    
    TSymbol *searchSymbol (const std::string &name, TSymbol::TFlags = TSymbol::AllSymbols) const;
    std::vector<TSymbol *> searchSymbols (const std::string &name, TSymbol::TFlags = TSymbol::AllSymbols) const;
    void removeSymbol (const TSymbol *);
    
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
    std::vector<TSymbol *> search (const std::string &name) const;

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

inline void TSymbol::setOverloadName (const std::string &s) {
    overloadName = s;
}

inline std::string TSymbol::getOverloadName () const {
    return overloadName;
}

inline void TSymbol::appendUnresolvedReferencePosition (std::size_t pos) {
    unresolvedReferencePositions.push_back (pos);
}

inline const std::vector<std::size_t> &TSymbol::getUnresolvedReferencePositions () const {
    return unresolvedReferencePositions;
}

}
