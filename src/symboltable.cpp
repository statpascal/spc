#include "datatypes.hpp"
#include "anyvalue.hpp"

#include <limits>

#include <iostream>

namespace statpascal {

const ssize_t TSymbol::LabelDefined, TSymbol::UndefinedLabelUsed, TSymbol::InvalidRegister;

TSymbol::TSymbol (const std::string &name, TType *type, std::size_t level, TFlags flags, TSymbol *alias):
  name (name), overloadName (name), level (level), flags (flags), alias (alias), block (nullptr), offset (0), parameterPosition (0), assignedRegister (InvalidRegister), aliased (false), constantValue (nullptr) {
    setType (type);
}

void TSymbol::setExternal (const std::string &lib, const std::string &sym) {
    libName = lib;
    extSymbolName = sym;
    addSymbolFlags (External);
}

void TSymbol::setAliasData () {
    level = alias->getLevel ();
    offset = alias->getOffset ();
}

// ---

TSymbolList::TSymbolList (TSymbolList *previousLevel, TMemoryPoolFactory &memoryPoolFactory):
  TSymbolList (previousLevel, memoryPoolFactory, previousLevel ? previousLevel->getLevel () + 1 : 0) {
}

TSymbolList::TSymbolList (TSymbolList *symbols, TMemoryPoolFactory &memoryPoolFactory, std::size_t level):
  previousLevel (symbols),
  memoryPoolFactory (memoryPoolFactory),
  level (level) {
}

TSymbolList::TAddSymbolResult TSymbolList::addRoutine (const std::string &name, TRoutineType *type) {
    return addSymbol (name, type, TSymbol::Routine, nullptr);
}

TSymbolList::TAddSymbolResult TSymbolList::addVariable (const std::string &name, TType *type) {
    return addSymbol (name, type, TSymbol::Variable, nullptr);
}

TSymbolList::TAddSymbolResult TSymbolList::addStaticVariable (const std::string &name, const TConstant *c) {
    TSymbolList::TAddSymbolResult result = addSymbol (name, c->getType (), static_cast<TSymbol::TFlags> (TSymbol::StaticVariable | TSymbol::Variable), nullptr);
    if (!result.alreadyPresent)
        result.symbol->setConstant (c);
    return result;
}

TSymbolList::TAddSymbolResult TSymbolList::addParameter (const std::string &name, TType *type) {
    return addSymbol (name, type, TSymbol::Parameter, nullptr);
}

TSymbolList::TAddSymbolResult TSymbolList::addAlias (const std::string &name, TType *type, TSymbol *alias) {
    alias->setAliased ();
    return addSymbol (name, type, TSymbol::Alias, alias);
}

TSymbolList::TAddSymbolResult TSymbolList::addAbsolute (const std::string &name, TType *type, std::int64_t addr) {
    TAddSymbolResult result = addSymbol (name, type, TSymbol::Absolute, nullptr);
    if (!result.alreadyPresent)
        result.symbol->setOffset (addr);
    return result;
}

TSymbolList::TAddSymbolResult TSymbolList::addLabel (const std::string &name) {
    return addSymbol (name, nullptr, TSymbol::Label, nullptr);
}

TSymbolList::TAddSymbolResult TSymbolList::addConstant (const std::string &name, const TSimpleConstant *c) {
    TSymbolList::TAddSymbolResult result = addSymbol (name, c->getType (), TSymbol::Constant, nullptr);
    if (!result.alreadyPresent)
        result.symbol->setConstant (c);
    return result;
}

TSymbolList::TAddSymbolResult TSymbolList::addNamedType (const std::string &name, TType *type) {
    return addSymbol (name, type, TSymbol::NamedType, nullptr);
}

TSymbolList::TAddSymbolResult TSymbolList::addSymbol (const std::string &name, TType *type, TSymbol::TFlags flags, TSymbol *alias) {
    thread_local static std::uint64_t routineCount = 0;

    std::vector<TSymbol *> results;
    std::copy_if (symbols.begin (), symbols.end (), std::back_inserter (results), [&name] (TSymbol *s) { return s->getName () == name; });
    if (!results.empty ()) {
        if (type && type->isRoutine () && results.front ()->getType ()->isRoutine ()) {
            for (TSymbol *s: results)
                if (static_cast<TRoutineType *> (s->getType ())->matchesOverload (static_cast<TRoutineType *> (type)))
                    return {s, true};
        } else
            return {results.front (), true};
    }
        
    TSymbol *s = memoryPoolFactory.create<TSymbol> (name, type, level, flags, alias);
    if ((type && type->isRoutine ()) || flags & TSymbol::Label)
        s->setOverloadName (name + "_$" + std::to_string (routineCount++));
    symbols.push_back (s);
    return {s, false};
}

TSymbol *TSymbolList::searchSymbol (const std::string &name, TSymbol::TFlags flags) const {
    std::vector<TSymbol *> result = searchSymbols (name, flags);
    if (result.empty ())
        return nullptr;
    else
        return result.front ();
}

std::vector<TSymbol *> TSymbolList::searchSymbols (const std::string &name, TSymbol::TFlags flags) const {
    std::vector<TSymbol *> result; // = search (name);
    const TSymbolList *searchLevel = this;
    do {
        std::copy_if (searchLevel->symbols.begin (), searchLevel->symbols.end (), std::back_inserter (result), [&name] (TSymbol *s) { return s->getName () == name; });
        searchLevel = searchLevel->previousLevel;
    } while (result.empty () && searchLevel);
        
    result.erase (
        std::remove_if (result.begin (), result.end (), [flags] (const TSymbol *s) {return !(s->getSymbolFlags () & flags);}),
        result.end ());
    return result;
    /*
    std::vector<TSymbol *> result; // = search (name);
    std::copy_if (symbols.begin (), symbols.end (), std::back_inserter (result), [&name] (TSymbol *s) { return s->getName () == name; });
    if (result.empty () && previousLevel)
        return previousLevel->searchSymbols (name, flags);
        
    result.erase (
        std::remove_if (result.begin (), result.end (), [flags] (const TSymbol *s) {return !(s->getSymbolFlags () & flags);}),
        result.end ());
    return result;
    */
}

void TSymbolList::removeSymbol (const TSymbol *symbol) {
    symbols.erase (std::remove (symbols.begin (), symbols.end (), symbol), symbols.end ());
    
}

void TSymbolList::moveSymbols (TSymbol::TFlags flags, TSymbolList &dest) {
    for (TBaseContainer::iterator it = symbols.begin (); it != symbols.end (); )
        if ((*it)->getSymbolFlags () & flags) {
            dest.symbols.push_back (*it);
            it = symbols.erase (it);
        } else
            ++it;
}

void TSymbolList::copySymbols (TSymbol::TFlags flags, TSymbolList &dest) {
    for (TSymbol *s: symbols) {
        if (s->getSymbolFlags () & flags)
            // TODO: !!!! check for duplicates when importing units multiple times (system.sp!)
            if (std::find (dest.begin (), dest.end (), s) == dest.end ()) 
                dest.symbols.push_back (s);
    }
}

std::size_t TSymbolList::getParameterSize () const {
    return parameterSize;
}

std::size_t TSymbolList::getLocalSize () const {
    return localSize;
}

void TSymbolList::setParameterSize (std::size_t n) {
    parameterSize = n;
}

void TSymbolList::setLocalSize (std::size_t n) {
    localSize = n;
}

void TSymbolList::setPreviousLevel (TSymbolList *prev) {
    previousLevel = prev;
}

TSymbolList *TSymbolList::getPreviousLevel () const {
    return previousLevel;
}

}
