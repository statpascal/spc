#include "symboltable.hpp"
#include "datatypes.hpp"
#include "expression.hpp"

namespace statpascal {

void TConstant::acceptCodeGenerator (TCodeGenerator &) {
}


TSimpleConstant::TSimpleConstant (TType *type):
  type (type), integerValue (0), doubleValue (0.0), routineValue (nullptr) {
    values.fill (0);
}

TSimpleConstant::TSimpleConstant (std::int64_t val, TType *type):
  type (type), integerValue (val), doubleValue (val), routineValue (nullptr) {
}

TSimpleConstant::TSimpleConstant (double v, TType *type):
  type (type), integerValue (0), doubleValue (v), routineValue (nullptr) {
}

TSimpleConstant::TSimpleConstant (const std::string &s, TType *type):
  type (type), integerValue (0), doubleValue (0.0), stringValue (s), routineValue (nullptr) {
}

TSimpleConstant::TSimpleConstant (TRoutineValue *routineValue):
  type (routineValue->getSymbol ()->getType ()), integerValue (0), doubleValue (0.0), routineValue (routineValue) {
}

void TSimpleConstant::setType (TType *t) {
    type = t;
}

TType *TSimpleConstant::getType () const {
    return type;
}

/*
void TSimpleConstant::setInteger (std::int64_t n, TType *t) {
    integerValue = n;
    doubleValue = n;
    type = t;
}
*/
    
std::int64_t TSimpleConstant::getInteger () const {
    return integerValue;
}

/*
void TSimpleConstant::setDouble (double v, TType *t) {
    doubleValue = v;
    type = t;
}
*/

double TSimpleConstant::getDouble () const {
    return doubleValue;
}

/*
void TSimpleConstant::setString (const std::string &s, TType *t) {
    stringValue = s;
    type = t;
}
*/

std::string TSimpleConstant::getString () const {
    return stringValue;
}

TRoutineValue *TSimpleConstant::getRoutineValue () const{
    return routineValue;
}

unsigned char TSimpleConstant::getChar () const {
    return integerValue;
}

bool TSimpleConstant::isValid () const {
    return !!type;
}

bool TSimpleConstant::hasType (TType *intType, TType *realType) const {
    return type == intType || type == realType;
}

void TSimpleConstant::addSetValue (std::uint64_t n) {
    if (n < 64 * TConfig::setwords)
        values [n / 64] |= (1LL << (n % 64));
}

void TSimpleConstant::addSetRange (std::uint64_t n1, std::uint64_t n2)  {
    for (std::uint64_t n = n1; n <= std::min (64 * TConfig::setwords - 1, n2); ++n)
        values [n / 64] |= (1LL << (n % 64));
}

const std::array<std::uint64_t, TConfig::setwords> &TSimpleConstant::getSetValues () const {
    return values;
}

//

TArrayConstant::TArrayConstant (TArrayType *type):
  type (type) {
}

void TArrayConstant::addValue (const TConstant *c) {
    values.push_back (c);
}

const std::vector<const TConstant *> &TArrayConstant::getValues () const {
    return values;;
}

TType *TArrayConstant::getType () const {
    return type;
}

//

TRecordConstant::TRecordConstant (TRecordType *type):
  type (type) {
}

void TRecordConstant::addValue (const std::string &component, const TConstant *c) {
    values.push_back ({component, c});
}

const std::vector<TRecordConstant::TRecordValue> &TRecordConstant::getValues () const {
    return values;
}

TType *TRecordConstant::getType () const {
    return type;
}

}
